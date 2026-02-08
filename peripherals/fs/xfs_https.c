#include "config.h"

#include "xfs.h"
#include "xfs_worker.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

#include "libspectrum.h"
#include "../http/httpc.h"
#include "../http/http_sck.h"

// Macro for XFS debug output (only prints if enabled)
#define XFS_DEBUG(...) do { if (xfs_debug_is_enabled()) printf(__VA_ARGS__); } while(0)

struct xfs_handle_https_file_t
{
    void* data;                          // Pointer to file data buffer
    size_t size;                         // Size of file data buffer
    size_t pos;                          // Current read position in buffer
};

struct xfs_handle_https_dir_t
{
    // parsed dir entries are stored there
    struct xfs_handle_https_dir_entry_t** dir_entries;
    uint8_t dir_entries_count;           // Number of entries allocated
    uint8_t dir_entries_pos;             // Current position for readdir
};

static struct xfs_handle_https_file_t* get_https_file_handle(const struct xfs_handle_t* handle)
{
    return (struct xfs_handle_https_file_t*)handle->data;
}

static struct xfs_handle_https_dir_t* get_https_dir_handle(const struct xfs_handle_t* handle)
{
    return (struct xfs_handle_https_dir_t*)handle->data;
}

// Maximum hostname length
#define HTTPS_MAX_HOSTNAME 256
// Maximum cached directories
#define HTTPS_DIR_CACHE_SIZE 4
// Maximum path length for cache
#define HTTPS_CACHE_PATH_MAX 256

struct https_dir_cache_entry_t
{
    char path[HTTPS_CACHE_PATH_MAX];
    struct xfs_handle_https_dir_entry_t** dir_entries;
    uint8_t dir_entries_count;
    uint8_t in_use;  // Non-zero if this cache slot is in use
};

struct https_engine_mount_data_t
{
    char url[HTTPS_MAX_HOSTNAME];
    struct https_dir_cache_entry_t https_dir_cache[HTTPS_DIR_CACHE_SIZE];
    uint8_t next_cache;
};

static inline struct https_engine_mount_data_t* get_mount_data(const struct xfs_engine_mount_t* engine_mount)
{
    return (struct https_engine_mount_data_t*)engine_mount->mount_data;
}

// Structure to hold buffer information during download
struct https_buffer_info_t
{
    void* data;                          // Buffer pointer
    size_t size;                         // Current buffer size
    size_t capacity;                     // Current buffer capacity
};

// Helper function to build full URL from base URL + path
// base_url should be the mount URL (e.g., "https://hostname/path/")
// path is the relative path to append
static int build_https_url(const char* base_url, const char* path, char* url_buf, size_t buf_size)
{
    // Skip "." and "./" at the start of path
    const char* path_ptr = path;
    while (*path_ptr == '.' && (path_ptr[1] == '/' || path_ptr[1] == '\0'))
    {
        if (path_ptr[1] == '/')
        {
            path_ptr += 2; // Skip "./"
        }
        else
        {
            path_ptr += 1; // Skip "."
            break;
        }
    }
    
    // If path is empty after skipping ".", use base_url as-is
    if (*path_ptr == '\0')
    {
        size_t base_len = strlen(base_url);
        if (base_len >= buf_size)
        {
            return -1;
        }
        memcpy(url_buf, base_url, base_len + 1);
        return 0;
    }
    
    // base_url already ends with '/', so we just need to append path
    // If path starts with '/', skip it (it's redundant)
    if (path_ptr[0] == '/')
    {
        path_ptr++;
    }
    
    // Simple append: base_url (ends with /) + path (without leading /)
    int written = snprintf(url_buf, buf_size, "%s%s", base_url, path_ptr);
    
    // Check if snprintf truncated (return value >= buf_size means truncation occurred)
    if (written < 0 || written >= buf_size)
    {
        return -1;
    }
    
    return 0;
}

// Normalize directory path for cache lookup (same logic used in stat)
static void https_normalize_dir_path(const char* path, char normalized[HTTPS_CACHE_PATH_MAX])
{
    if (!path || strcmp(path, ".") == 0 || strcmp(path, "./") == 0)
    {
        strncpy(normalized, "/", HTTPS_CACHE_PATH_MAX);
        return;
    }

    strncpy(normalized, path, HTTPS_CACHE_PATH_MAX);

    const uint16_t path_len = strlen(path);
    if (path_len > 1 && normalized[path_len - 1] == '/')
    {
        normalized[path_len - 1] = '\0';
    }
}

static struct xfs_handle_https_dir_entry_t* https_cache_find_entry(
    const struct xfs_engine_mount_t* engine, const char* dir_path, const char* entry_name)
{
    char normalized_path[HTTPS_CACHE_PATH_MAX];
    https_normalize_dir_path(dir_path, normalized_path);

    const struct https_engine_mount_data_t* mount_data = get_mount_data(engine);
    
    // Find cache entry for this directory
    for (int i = 0; i < HTTPS_DIR_CACHE_SIZE; i++)
    {
        if (mount_data->https_dir_cache[i].in_use && strcmp(mount_data->https_dir_cache[i].path, normalized_path) == 0)
        {
            // Search entries in this cached directory
            for (uint8_t j = 0; j < mount_data->https_dir_cache[i].dir_entries_count; j++)
            {
                if (mount_data->https_dir_cache[i].dir_entries[j] &&
                    strcmp(mount_data->https_dir_cache[i].dir_entries[j]->name, entry_name) == 0)
                {
                    return mount_data->https_dir_cache[i].dir_entries[j];
                }
            }
            break;
        }
    }
    
    return NULL;
}

// Free directory entries from a cache entry
static void https_cache_free_entries(struct https_dir_cache_entry_t* cache_entry)
{
    if (!cache_entry || !cache_entry->dir_entries)
    {
        return;
    }
    
    for (uint8_t i = 0; i < cache_entry->dir_entries_count; i++)
    {
        if (cache_entry->dir_entries[i])
        {
            libspectrum_free(cache_entry->dir_entries[i]);
        }
    }
    libspectrum_free(cache_entry->dir_entries);
    cache_entry->dir_entries = NULL;
    cache_entry->dir_entries_count = 0;
}

static struct xfs_handle_https_dir_entry_t* https_cache_copy_entry(const struct xfs_handle_https_dir_entry_t* src)
{
    if (!src)
    {
        return NULL;
    }

    struct xfs_handle_https_dir_entry_t* dst = libspectrum_malloc(sizeof(struct xfs_handle_https_dir_entry_t));

    if (!dst)
    {
        return NULL;
    }

    memcpy(dst, src, sizeof(struct xfs_handle_https_dir_entry_t));
    return dst;
}

// Put directory entries into cache
static void https_cache_put_entries(const struct xfs_engine_mount_t* engine, const char* dir_path,
    struct xfs_handle_https_dir_entry_t** entries, uint8_t count)
{
    if (!entries || count == 0)
    {
        return;
    }

    struct https_engine_mount_data_t* mount_data = get_mount_data(engine);

    int cache_idx = -1;

    {
      for (int i = 0; i < HTTPS_DIR_CACHE_SIZE; i++)
      {
          if (!mount_data->https_dir_cache[i].in_use)
          {
              cache_idx = i;
              break;
          }
      }
    }

    // If cache is full, use next_cache slot and cycle to next
    if (cache_idx == -1)
    {
        cache_idx = mount_data->next_cache;
        mount_data->next_cache = (mount_data->next_cache + 1) % HTTPS_DIR_CACHE_SIZE;
    }

    // If cache slot was in use, free old entries
    if (mount_data->https_dir_cache[cache_idx].in_use)
    {
        if (mount_data->https_dir_cache[cache_idx].dir_entries)
        {
            https_cache_free_entries(&mount_data->https_dir_cache[cache_idx]);
        }
    }

    // Copy path
    size_t path_len = strlen(dir_path);
    if (path_len >= HTTPS_CACHE_PATH_MAX)
    {
        path_len = HTTPS_CACHE_PATH_MAX - 1;
    }
    memcpy(mount_data->https_dir_cache[cache_idx].path, dir_path, path_len);
    mount_data->https_dir_cache[cache_idx].path[path_len] = '\0';

    // Allocate array of pointers
    mount_data->https_dir_cache[cache_idx].dir_entries = (struct xfs_handle_https_dir_entry_t**)libspectrum_malloc(
        count * sizeof(struct xfs_handle_https_dir_entry_t*));

    if (!mount_data->https_dir_cache[cache_idx].dir_entries)
    {
        mount_data->https_dir_cache[cache_idx].in_use = 0;
        return;
    }

    // Copy all entries
    int dir_entries_count = 0;
    for (int i = 0; i < count; i++)
    {
        if (entries[i])
        {
            struct xfs_handle_https_dir_entry_t* cached_entry = https_cache_copy_entry(entries[i]);
            if (cached_entry)
            {
                mount_data->https_dir_cache[cache_idx].dir_entries[dir_entries_count] = cached_entry;
                dir_entries_count++;
            }
        }
    }

    mount_data->https_dir_cache[cache_idx].dir_entries_count = dir_entries_count;
    mount_data->https_dir_cache[cache_idx].in_use = 1;

    XFS_DEBUG("https: cache_put cached %d entries for path '%s' at slot %d\n",
        dir_entries_count, dir_path, cache_idx);
}

// Put directory entries into cache (stores pointer directly, no copying)
static void https_cache_store_entries(const struct xfs_engine_mount_t* engine,
    const char* dir_path, struct xfs_handle_https_dir_entry_t** entries, const uint8_t count)
{
    if (!entries || count == 0)
    {
        return;
    }

    struct https_engine_mount_data_t* mount_data = get_mount_data(engine);
    
    // Find first unused cache slot
    int cache_idx = -1;

    for (int i = 0; i < HTTPS_DIR_CACHE_SIZE; i++)
    {
        if (!mount_data->https_dir_cache[i].in_use)
        {
            cache_idx = i;
            break;
        }
    }
    
    // If cache is full, use next_cache slot and cycle to next
    if (cache_idx == -1)
    {
        cache_idx = mount_data->next_cache;
        mount_data->next_cache = (mount_data->next_cache + 1) % HTTPS_DIR_CACHE_SIZE;
    }
    
    // If cache slot was in use, free old entries
    if (mount_data->https_dir_cache[cache_idx].in_use)
    {
        https_cache_free_entries(&mount_data->https_dir_cache[cache_idx]);
    }
    
    // Copy path
    strncpy(mount_data->https_dir_cache[cache_idx].path, dir_path, HTTPS_CACHE_PATH_MAX);
    
    // Store entries pointer directly (no copying)
    mount_data->https_dir_cache[cache_idx].dir_entries = entries;
    mount_data->https_dir_cache[cache_idx].dir_entries_count = count;
    mount_data->https_dir_cache[cache_idx].in_use = 1;
    
    XFS_DEBUG("https: cache_store stored %d entries for path '%s' at slot %d\n", 
              count, dir_path, cache_idx);
}

// Callback function for httpc_get to write downloaded data to buffer
static int write_buffer_callback(void *param, unsigned char *buf, size_t length, size_t position, size_t content_length)
{
    XFS_DEBUG("https: write_buffer_callback ENTRY param=%p buf=%p length=%zu position=%zu content_length=%zu\n",
              param, (void*)buf, length, position, content_length);
    
    struct https_buffer_info_t* buffer_info = (struct https_buffer_info_t*)param;
    
    // If we know the content length, pre-allocate buffer
    if (content_length > 0 && buffer_info->capacity == 0)
    {
        buffer_info->capacity = content_length;
        buffer_info->data = libspectrum_malloc(content_length);
        if (!buffer_info->data)
        {
            XFS_DEBUG("https: write_buffer_callback [ERROR] failed to allocate buffer of size %zu\n", content_length);
            return -1;
        }
        buffer_info->size = 0;
    }
    
    // Grow buffer if needed (if content_length was unknown)
    if (position + length > buffer_info->capacity)
    {
        size_t new_capacity = buffer_info->capacity;
        if (new_capacity == 0)
        {
            new_capacity = 4096; // Start with 4KB
        }
        
        // Double capacity until it's large enough
        while (new_capacity < position + length)
        {
            new_capacity *= 2;
        }
        
        void* new_data = libspectrum_realloc(buffer_info->data, new_capacity);
        if (!new_data)
        {
            XFS_DEBUG("https: write_buffer_callback [ERROR] failed to reallocate buffer to size %zu\n", new_capacity);
            return -1;
        }
        
        buffer_info->data = new_data;
        buffer_info->capacity = new_capacity;
    }
    
    // Copy data to buffer
    memcpy((unsigned char*)buffer_info->data + position, buf, length);
    
    // Update size if this extends the buffer
    if (position + length > buffer_info->size)
    {
        buffer_info->size = position + length;
    }
    
    return (int)length;
}

static int parse_index_line_to_entry(const char* line, struct xfs_handle_https_dir_entry_t* entry);
static int https_parse_index_txt(const char* buffer, const size_t buffer_len,
    struct xfs_handle_https_dir_entry_t** entries,
    const uint8_t max_entries);

// Fetch and parse index.txt for a directory
// Fetches all entries, allocates and returns entries array in out_entries, returns entry count
static int16_t https_fetch_and_parse_index(const struct xfs_engine_mount_t* engine, const char* dir_path,
    struct xfs_handle_https_dir_entry_t*** out_entries, uint8_t* out_entry_count)
{
    struct https_engine_mount_data_t* mount_data = get_mount_data(engine);
    if (!mount_data || mount_data->url[0] == '\0')
    {
        XFS_DEBUG("https: fetch_index failed: not mounted\n");
        return XFS_ERR_IO;
    }

    // Build index.txt path
    char index_path[256];
    if (dir_path)
    {
        if (dir_path[strlen(dir_path) - 1] == '/')
        {
            snprintf(index_path, sizeof(index_path), "%sindex.txt", dir_path);
        }
        else
        {
            snprintf(index_path, sizeof(index_path), "%s/index.txt", dir_path);
        }
    }
    else
    {
        strcpy(index_path, "/");
    }

    // Build full URL
    char url[512];
    if (build_https_url(mount_data->url, index_path, url, sizeof(url)) != 0)
    {
        XFS_DEBUG("https: fetch_index failed: invalid URL\n");
        return XFS_ERR_INVAL;
    }

    XFS_DEBUG("https: fetch_index fetching from '%s'\n", url);

    // Allocate buffer and fetch index.txt
    const size_t buffer_size = 2048;
    char* buffer = (char*)libspectrum_malloc(buffer_size);
    if (!buffer)
    {
        return XFS_ERR_NOMEM;
    }

    size_t length = buffer_size;
    if (httpc_get_buffer(&tls_sck, url, buffer, &length) != HTTPC_OK)
    {
        XFS_DEBUG("https: fetch_index failed: httpc_get_buffer error\n");
        libspectrum_free(buffer);
        return XFS_ERR_NOENT;
    }

    XFS_DEBUG("https: fetch_index received %zu bytes\n", length);

    // First pass: count entries (max 128)
    int entry_count = https_parse_index_txt(buffer, length, NULL, 0);
    if (entry_count < 0)
    {
        XFS_DEBUG("https: fetch_index failed: parse error\n");
        libspectrum_free(buffer);
        return XFS_ERR_IO;
    }

    // Limit to 128 entries
    if (entry_count > 128)
    {
        entry_count = 128;
    }

    XFS_DEBUG("https: fetch_index found %d entries\n", entry_count);

    if (entry_count == 0)
    {
        XFS_DEBUG("https: fetch_index empty directory\n");
        libspectrum_free(buffer);
        *out_entries = NULL;
        *out_entry_count = 0;
        return XFS_ERR_OK; // Empty directory
    }

    // Allocate array of pointers
    struct xfs_handle_https_dir_entry_t** entries = (struct xfs_handle_https_dir_entry_t**)libspectrum_malloc(entry_count *
    sizeof(struct xfs_handle_https_dir_entry_t*));

    if (!entries)
    {
        XFS_DEBUG("https: fetch_index failed: no memory for entries array\n");
        libspectrum_free(buffer);
        return XFS_ERR_NOMEM;
    }

    // Second pass: parse entries
    const int parsed_count = https_parse_index_txt(buffer, length, entries, entry_count);

    if (parsed_count < 0)
    {
        // Free already allocated entries
        for (int i = 0; i < entry_count; i++)
        {
            if (entries[i])
            {
                libspectrum_free(entries[i]);
            }
        }
        libspectrum_free(entries);
        libspectrum_free(buffer);
        return XFS_ERR_NOMEM;
    }

    // Free buffer now that parsing is complete
    libspectrum_free(buffer);

    *out_entries = entries;
    *out_entry_count = (uint8_t)parsed_count;

    XFS_DEBUG("https: fetch_index parsed %d entries successfully\n", parsed_count);
    return XFS_ERR_OK;
}


// Mount HTTPS filesystem
static int16_t https_mount(const struct xfs_engine_t* engine, const char* hostname, const char* path, struct xfs_engine_mount_t* out_mount)
{
    XFS_DEBUG("https: mount hostname='%s' path='%s'\n", hostname ? hostname : "(null)", path ? path : "(null)");
    
    (void)engine;
    
    if (!hostname || !path)
    {
        XFS_DEBUG("https: mount failed: invalid parameters\n");
        return XFS_ERR_INVAL;
    }

    // Check for empty hostname
    if (strlen(hostname) == 0)
    {
        XFS_DEBUG("https: mount failed: empty hostname\n");
        return XFS_ERR_INVAL;
    }

    struct https_engine_mount_data_t* mount_data = libspectrum_malloc(sizeof(struct https_engine_mount_data_t));
    if (mount_data == NULL)
    {
        XFS_DEBUG("https: mount failed: no mount data\n");
        return XFS_ERR_NOMEM;
    }
    memset(mount_data, 0, sizeof(struct https_engine_mount_data_t));

    // Initialize cache rotation index
    mount_data->next_cache = 0;

    // Handle empty path by treating it as root "/"
    const char* normalized_path = path;
    const size_t path_len = strlen(path);
    
    // Normalize empty path to "/"
    if (path_len == 0)
    {
        normalized_path = "/";
    }
    
    // Safe to access last character now (either path has length > 0, or normalized_path is "/")
    const size_t normalized_len = strlen(normalized_path);
    uint8_t path_ends_with_slash = normalized_path[normalized_len - 1] == '/';
    uint8_t path_starts_with_slash = normalized_path[0] == '/';

    // Calculate maximum URL length: "https://" (8) + hostname (max 64) + "/" (1) + path (max 160) + "/" (1) + null (1) = 235
    // mount_data->url is HTTPS_MAX_HOSTNAME (256) bytes, so we have enough space
    const size_t hostname_len = strlen(hostname);
    const size_t max_url_len = 8 + hostname_len + normalized_len + 2; // "https://" + hostname + path + optional "/" + null
    if (max_url_len >= sizeof(mount_data->url))
    {
        XFS_DEBUG("https: mount failed: URL too long (hostname=%zu, path=%zu)\n", hostname_len, normalized_len);
        libspectrum_free(mount_data);
        return XFS_ERR_INVAL;
    }

    int url_written;
    if (path_starts_with_slash)
    {
        // Path already starts with '/', so don't add another one after hostname
        if (path_ends_with_slash)
        {
            url_written = snprintf(mount_data->url, sizeof(mount_data->url), "https://%s%s", hostname, normalized_path);
        }
        else
        {
            url_written = snprintf(mount_data->url, sizeof(mount_data->url), "https://%s%s/", hostname, normalized_path);
        }
    }
    else
    {
        // Path doesn't start with '/', so add one after hostname
        if (path_ends_with_slash)
        {
            url_written = snprintf(mount_data->url, sizeof(mount_data->url), "https://%s/%s", hostname, normalized_path);
        }
        else
        {
            url_written = snprintf(mount_data->url, sizeof(mount_data->url), "https://%s/%s/", hostname, normalized_path);
        }
    }
    
    // Verify snprintf succeeded (check for truncation or error)
    if (url_written < 0 || url_written >= sizeof(mount_data->url))
    {
        XFS_DEBUG("https: mount failed: URL buffer overflow or snprintf error (written=%d, max=%zu)\n", 
                  url_written, sizeof(mount_data->url));
        mount_data->url[0] = '\0'; // Clear invalid URL
        libspectrum_free(mount_data);
        return XFS_ERR_INVAL;
    }

    XFS_DEBUG("https: mount hostname set to '%s'\n", mount_data->url);

    // Temporarily set mount_data so we can use fetch functions
    out_mount->mount_data = mount_data;

    // Fetch and cache root index.txt to verify mount is valid
    struct xfs_handle_https_dir_entry_t** root_entries = NULL;
    uint8_t root_entry_count = 0;
    
    XFS_DEBUG("https: mount fetching root index.txt\n");
    const int16_t fetch_result = https_fetch_and_parse_index(out_mount, "/", &root_entries, &root_entry_count);
    
    if (fetch_result != XFS_ERR_OK)
    {
        XFS_DEBUG("https: mount failed: root index.txt not found or error (result=%d)\n", fetch_result);
        out_mount->mount_data = NULL;
        libspectrum_free(mount_data);
        return fetch_result;
    }
    
    // Cache the root directory entries
    char cache_path[HTTPS_CACHE_PATH_MAX];
    https_normalize_dir_path("/", cache_path);
    https_cache_store_entries(out_mount, cache_path, root_entries, root_entry_count);
    // Ownership transferred to cache, root_entries will be freed when cache is evicted
    
    XFS_DEBUG("https: mount success, root index.txt cached with %d entries\n", root_entry_count);
    return XFS_ERR_OK;
}

// Unmount HTTPS filesystem
static void https_unmount(const struct xfs_engine_t* engine, struct xfs_engine_mount_t* mount)
{
    XFS_DEBUG("https: unmount\n");
    
    (void)engine;
    
    struct https_engine_mount_data_t* mount_data = get_mount_data(mount);
    if (!mount_data)
    {
        XFS_DEBUG("https: unmount: no mount data\n");
        return;
    }
    
    // Free all cache entries
    for (int i = 0; i < HTTPS_DIR_CACHE_SIZE; i++)
    {
        if (mount_data->https_dir_cache[i].in_use)
        {
            https_cache_free_entries(&mount_data->https_dir_cache[i]);
            mount_data->https_dir_cache[i].in_use = 0;
        }
    }
    
    // Clear hostname
    mount_data->url[0] = '\0';
    XFS_DEBUG("https: unmount complete\n");

    mount->mount_data = NULL;
    libspectrum_free(mount_data);
}

// Open file - download to memory buffer
static int16_t https_open(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle, const char* path, int flags)
{
    XFS_DEBUG("https: open path='%s' flags=0x%04x\n", path ? path : "(null)", flags);
    
    (void)flags; // HTTPS is read-only, ignore write flags
    
    struct https_engine_mount_data_t* mount_data = get_mount_data(engine);
    if (!mount_data || mount_data->url[0] == '\0')
    {
        XFS_DEBUG("https: open failed: not mounted\n");
        return XFS_ERR_IO;
    }
    
    // Build full URL
    char url[256];
    if (build_https_url(mount_data->url, path, url, sizeof(url)) != 0)
    {
        XFS_DEBUG("https: open failed: invalid URL\n");
        return XFS_ERR_INVAL;
    }
    
    XFS_DEBUG("https: open URL='%s'\n", url);

    struct xfs_handle_https_file_t* https_handle = libspectrum_malloc(sizeof(struct xfs_handle_https_file_t));
    if (!https_handle)
    {
        return XFS_ERR_NOMEM;
    }
    memset(https_handle, 0, sizeof(struct xfs_handle_https_file_t));

    // Initialize buffer info for download callback
    struct https_buffer_info_t buffer_info = {0};
    
    // Download file using httpc_get
    XFS_DEBUG("https: open downloading from URL...\n");
    if (httpc_get(&tls_sck, url, write_buffer_callback, &buffer_info) != HTTPC_OK)
    {
        XFS_DEBUG("https: open failed: httpc_get error\n");
        if (buffer_info.data)
        {
            libspectrum_free(buffer_info.data);
        }
        libspectrum_free(https_handle);
        return XFS_ERR_IO;
    }
    
    XFS_DEBUG("https: open download complete, size=%zu\n", buffer_info.size);
    
    // Store buffer pointer and size
    https_handle->data = buffer_info.data;
    https_handle->size = buffer_info.size;
    https_handle->pos = 0;

    handle->type = XFS_HANDLE_TYPE_FILE;
    handle->data = https_handle;

    XFS_DEBUG("https: open success\n");
    return XFS_ERR_OK;
}

// Read from memory buffer
static int16_t https_read(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle, void* buffer, uint16_t size)
{
    (void)engine;
    struct xfs_handle_https_file_t* https_handle = get_https_file_handle(handle);

    // Check if we're at end of file
    if (https_handle->pos >= https_handle->size)
    {
        XFS_DEBUG("https: read EOF\n");
        return 0;
    }
    
    // Calculate how much we can read
    size_t remaining = https_handle->size - https_handle->pos;
    size_t to_read = (size < remaining) ? size : remaining;
    
    // Copy data from buffer
    memcpy(buffer, (unsigned char*)https_handle->data + https_handle->pos, to_read);
    https_handle->pos += to_read;
    
    XFS_DEBUG("https: read size=%d bytes_read=%zu\n", size, to_read);
    return (int16_t)to_read;
}

// Write not supported
static int16_t https_write(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle, const void* buffer, uint16_t size)
{
    XFS_DEBUG("https: write called (not supported)\n");
    (void)engine;
    (void)handle;
    (void)buffer;
    (void)size;
    return XFS_ERR_INVAL;
}

// Close file and free memory buffer
static int16_t https_close(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle)
{
    XFS_DEBUG("https: close\n");
    (void)engine;

    struct xfs_handle_https_file_t* https_handle = get_https_file_handle(handle);
    
    // Free buffer
    if (https_handle->data)
    {
        libspectrum_free(https_handle->data);
        https_handle->data = NULL;
    }
    
    // Free file handle
    libspectrum_free(https_handle);
    handle->data = NULL;
    
    XFS_DEBUG("https: close success\n");
    return XFS_ERR_OK;
}

// Seek in memory buffer
static int16_t https_lseek(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle, uint32_t offset, uint8_t whence)
{
    XFS_DEBUG("https: lseek offset=%lu whence=%d\n", (unsigned long)offset, whence);
    (void)engine;

    struct xfs_handle_https_file_t* https_handle = get_https_file_handle(handle);
    
    size_t new_pos;
    if (whence == 0) // SEEK_SET
    {
        new_pos = offset;
    }
    else if (whence == 1) // SEEK_CUR
    {
        new_pos = https_handle->pos + offset;
    }
    else if (whence == 2) // SEEK_END
    {
        new_pos = https_handle->size + offset;
    }
    else
    {
        XFS_DEBUG("https: lseek failed: invalid whence\n");
        return XFS_ERR_INVAL;
    }
    
    // Clamp to valid range
    if (new_pos > https_handle->size)
    {
        new_pos = https_handle->size;
    }
    
    https_handle->pos = new_pos;
    
    XFS_DEBUG("https: lseek new_pos=%zu\n", new_pos);
    return (int16_t)new_pos;
}

// Parse index.txt buffer and populate entries array
// Returns number of entries parsed, or -1 on error
// If entries is NULL and max_entries is 0, only counts entries (doesn't allocate)
// If search_name is non-NULL, stops at first matching entry and returns 1 if found, -1 if not found
// If found_entry is provided and search_name is set, fills found_entry with the matching entry
static int https_parse_index_txt(const char* buffer, const size_t buffer_len,
    struct xfs_handle_https_dir_entry_t** entries,
    const uint8_t max_entries)
{
    size_t pos = 0;
    uint8_t entry_idx = 0;
    
    while (pos < buffer_len && (max_entries == 0 || entry_idx < max_entries))
    {
        // Skip whitespace and empty lines
        while (pos < buffer_len && (buffer[pos] == ' ' || buffer[pos] == '\t' || buffer[pos] == '\n' || buffer[pos] == '\r'))
        {
            pos++;
        }
        
        if (pos >= buffer_len)
            break;
        
        // Find end of line
        size_t line_start = pos;
        size_t line_end = pos;
        while (line_end < buffer_len && buffer[line_end] != '\n' && buffer[line_end] != '\r')
        {
            line_end++;
        }
        
        if (line_end > line_start)
        {
            // Extract line
            size_t line_len = line_end - line_start;
            char line[256];
            if (line_len >= sizeof(line))
            {
                line_len = sizeof(line) - 1;
            }
            memcpy(line, buffer + line_start, line_len);
            line[line_len] = '\0';
            
            // Determine which entry buffer to use
            struct xfs_handle_https_dir_entry_t* entry = NULL;
            struct xfs_handle_https_dir_entry_t temp_entry;
            int should_allocate = 0;
            
            if (max_entries > 0 && entries)
            {
                // Parsing all entries, allocate new entry
                should_allocate = 1;
            }
            else
            {
                // Just counting, use temp buffer
                entry = &temp_entry;
            }
            
            if (should_allocate)
            {
                entry = (struct xfs_handle_https_dir_entry_t*)libspectrum_malloc(
                    sizeof(struct xfs_handle_https_dir_entry_t));
            }

            if (entry == NULL)
            {
                return -1; // Out of memory?
            }

            // Parse entry
            if (parse_index_line_to_entry(line, entry) == 0)
            {
                // If searching for specific name, check if it matches
                if (max_entries > 0 && entries)
                {
                    // Store in entries array
                    entries[entry_idx] = entry;
                    entry_idx++;
                }
                else
                {
                    // Just counting or searching (but not found yet)
                    entry_idx++;
                    if (should_allocate)
                    {
                        // We allocated but didn't store, free it
                        libspectrum_free(entry);
                    }
                }
            }
            else
            {
                // Invalid entry
                if (should_allocate)
                {
                    libspectrum_free(entry);
                }
            }
        }
        
        // Skip line ending
        pos = line_end;
        if (pos < buffer_len && buffer[pos] == '\r')
            pos++;
        if (pos < buffer_len && buffer[pos] == '\n')
            pos++;
    }

    return entry_idx;
}

// Parse index.txt file line into xfs_handle_https_dir_entry_t
static int parse_index_line_to_entry(const char* line, struct xfs_handle_https_dir_entry_t* entry)
{
    memset(entry, 0, sizeof(*entry));
    
    // Parse key=value pairs
    const char* p = line;
    char type[16] = {0};
    char name[256] = {0};
    uint32_t size = 0;
    
    while (*p)
    {
        // Skip whitespace
        while (*p == ' ' || *p == '\t')
            p++;
        
        if (*p == '\0' || *p == '\n' || *p == '\r')
            break;
        
        // Find key
        const char* key_start = p;
        while (*p && *p != '=' && *p != ' ' && *p != '\t')
            p++;
        
        if (*p != '=')
            break;
        
        size_t key_len = p - key_start;
        p++; // Skip '='
        
        // Find value
        const char* value_start = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
            p++;
        
        size_t value_len = p - value_start;
        
        // Process key-value pair
        if (key_len == 4 && memcmp(key_start, "type", 4) == 0)
        {
            if (value_len < sizeof(type))
            {
                memcpy(type, value_start, value_len);
                type[value_len] = '\0';
            }
        }
        else if (key_len == 4 && memcmp(key_start, "name", 4) == 0)
        {
            if (value_len < sizeof(name))
            {
                memcpy(name, value_start, value_len);
                name[value_len] = '\0';
            }
        }
        else if (key_len == 4 && memcmp(key_start, "size", 4) == 0)
        {
            size = (uint32_t)strtoul(value_start, NULL, 10);
        }
    }
    
    // Set entry structure
    if (strcmp(type, "dir") == 0)
    {
        entry->is_dir = 1;
    }
    else if (strcmp(type, "file") == 0)
    {
        entry->is_dir = 0;
        entry->size = size;
    }
    else
    {
        return -1;
    }
    
    // Copy name (ensure it fits)
    size_t name_len = strlen(name);
    if (name_len >= sizeof(entry->name))
    {
        name_len = sizeof(entry->name) - 1;
    }
    memcpy(entry->name, name, name_len);
    entry->name[name_len] = '\0';
    
    return 0;
}

// Open directory - fetch index.txt file
static int16_t https_opendir(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle, const char* path)
{
    XFS_DEBUG("https: opendir path='%s'\n", path ? path : "(null)");

    uint8_t entry_count = 0;

    struct xfs_handle_https_dir_t* https_handle = libspectrum_malloc(sizeof(struct xfs_handle_https_dir_t));
    if (!https_handle)
    {
        return XFS_ERR_NOMEM;
    }
    memset(https_handle, 0, sizeof(struct xfs_handle_https_dir_t));

    const int16_t result = https_fetch_and_parse_index(engine, path, &https_handle->dir_entries, &entry_count);
    
    if (result != XFS_ERR_OK)
    {
        libspectrum_free(https_handle);
        return result;
    }
    
    if (entry_count == 0)
    {
        // Empty directory
        libspectrum_free(https_handle);
        return XFS_ERR_OK;
    }

    https_handle->dir_entries_count = entry_count;
    https_handle->dir_entries_pos = 0;
    
    // Cache the directory entries using normalized directory path (same as stat() will use)
    char cache_path[HTTPS_CACHE_PATH_MAX];
    https_normalize_dir_path(path, cache_path);
    https_cache_put_entries(engine, cache_path, https_handle->dir_entries, entry_count);
    
    handle->data = https_handle;
    handle->type = XFS_HANDLE_TYPE_DIR;
    
    return XFS_ERR_OK;
}

// Read directory entry from parsed entries array
static int16_t https_readdir(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle, struct xfs_stat_info* info)
{
    (void)engine;
    
    struct xfs_handle_https_dir_t* https_handle = get_https_dir_handle(handle);

    if (!https_handle->dir_entries || https_handle->dir_entries_pos >= https_handle->dir_entries_count)
    {
        XFS_DEBUG("https: readdir end of directory\n");
        return 0; // No more entries
    }
    
    struct xfs_handle_https_dir_entry_t* entry = https_handle->dir_entries[https_handle->dir_entries_pos];
    if (!entry)
    {
        XFS_DEBUG("https: readdir null entry\n");
        return 0;
    }
    
    // Fill xfs_stat_info structure
    memset(info, 0, sizeof(*info));
    
    if (entry->is_dir)
    {
        info->type = XFS_TYPE_DIR;
    }
    else
    {
        info->type = XFS_TYPE_REG;
        info->size = entry->size;
    }
    
    // Copy name
    strncpy(info->name, entry->name, sizeof(info->name));
    
    XFS_DEBUG("https: readdir name='%s' type=%d size=%lu\n", 
               info->name, info->type, (unsigned long)info->size);
    
    https_handle->dir_entries_pos++;
    
    return 1; // Entry found
}

// Close directory
static int16_t https_closedir(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle)
{
    (void)engine;
    XFS_DEBUG("https: closedir success\n");
    return XFS_ERR_OK;
}

// Stat - check if file/directory exists
static int16_t https_stat(const struct xfs_engine_mount_t* engine, const char* path, struct xfs_stat_info* stat_info)
{
    XFS_DEBUG("https: stat path='%s'\n", path ? path : "(null)");
    
    struct https_engine_mount_data_t* mount_data = get_mount_data(engine);
    if (!mount_data || mount_data->url[0] == '\0')
    {
        XFS_DEBUG("https: stat failed: not mounted\n");
        return XFS_ERR_IO;
    }
    
    if (!path)
    {
        XFS_DEBUG("https: stat failed: null path\n");
        return XFS_ERR_INVAL;
    }
    
    // Extract directory path and filename
    char dir_path[HTTPS_CACHE_PATH_MAX];
    char entry_name[64];
    const char* last_slash = strrchr(path, '/');
    
    if (last_slash)
    {
        // Copy directory path
        size_t dir_len = last_slash - path;
        if (dir_len >= HTTPS_CACHE_PATH_MAX)
        {
            dir_len = HTTPS_CACHE_PATH_MAX - 1;
        }
        memcpy(dir_path, path, dir_len);
        dir_path[dir_len] = '\0';
        if (dir_len == 0)
        {
            strcpy(dir_path, "/");
        }
        
        // Copy entry name
        const char* name_start = last_slash + 1;
        if (*name_start == '\0')
        {
            // Path ends with '/' - entry name is empty, so we're checking if the directory itself exists
            // Extract the directory name from dir_path
            if (strcmp(dir_path, "/") == 0)
            {
                // Root directory
                strcpy(entry_name, "");
            }
            else
            {
                // Find the last component of dir_path
                const char* dir_name_slash = strrchr(dir_path, '/');
                if (dir_name_slash)
                {
                    strncpy(entry_name, dir_name_slash + 1, sizeof(entry_name) - 1);
                    entry_name[sizeof(entry_name) - 1] = '\0';
                    
                    // Update dir_path to be the parent directory
                    const size_t parent_len = dir_name_slash - dir_path;
                    if (parent_len == 0)
                    {
                        strcpy(dir_path, "/");
                    }
                    else
                    {
                        dir_path[parent_len] = '\0';
                    }
                }
                else
                {
                    // dir_path has no slashes, so it's the entry name in root
                    strncpy(entry_name, dir_path, sizeof(entry_name) - 1);
                    entry_name[sizeof(entry_name) - 1] = '\0';
                    strcpy(dir_path, "/");
                }
            }
        }
        else
        {
            // Normal entry name
            strncpy(entry_name, name_start, sizeof(entry_name) - 1);
            entry_name[sizeof(entry_name) - 1] = '\0';
        }
    }
    else
    {
        // No slash, entry is in root directory
        strcpy(dir_path, "/");
        strncpy(entry_name, path, sizeof(entry_name) - 1);
        entry_name[sizeof(entry_name) - 1] = '\0';
    }
    
    // Try to find entry in cache first
    const struct xfs_handle_https_dir_entry_t* cached_entry =
        https_cache_find_entry(engine, dir_path, entry_name);

    if (cached_entry)
    {
        XFS_DEBUG("https: stat found in cache: '%s' in '%s'\n", entry_name, dir_path);
        memset(stat_info, 0, sizeof(*stat_info));
        
        if (cached_entry->is_dir)
        {
            stat_info->type = XFS_TYPE_DIR;
            stat_info->size = 0;
        }
        else
        {
            stat_info->type = XFS_TYPE_REG;
            stat_info->size = cached_entry->size;
        }
        
        // Copy name
        size_t name_len = strlen(cached_entry->name);
        if (name_len >= sizeof(stat_info->name))
        {
            name_len = sizeof(stat_info->name) - 1;
        }
        memcpy(stat_info->name, cached_entry->name, name_len);
        stat_info->name[name_len] = '\0';
        
        XFS_DEBUG("https: stat success from cache: type=%d size=%lu\n", 
                  stat_info->type, (unsigned long)stat_info->size);

        return XFS_ERR_OK;
    }
    
    // Not in cache, fetch index.txt for the directory
    struct xfs_handle_https_dir_entry_t** entries = NULL;
    uint8_t entry_count = 0;
    
    const int16_t fetch_result = https_fetch_and_parse_index(engine, dir_path, &entries, &entry_count);
    
    if (fetch_result != XFS_ERR_OK)
    {
        return fetch_result;
    }
    
    // Search for the entry in the fetched entries
    const struct xfs_handle_https_dir_entry_t* found_entry = NULL;
    for (uint8_t i = 0; i < entry_count; i++)
    {
        if (entries[i] && strcmp(entries[i]->name, entry_name) == 0)
        {
            found_entry = entries[i];
            break;
        }
    }
    
    // Cache the directory entries for future use (transfer ownership to cache)
    char cache_path[HTTPS_CACHE_PATH_MAX];
    https_normalize_dir_path(dir_path, cache_path);
    https_cache_store_entries(engine, cache_path, entries, entry_count);
    // Ownership transferred to cache, entries will be freed when cache is evicted
    entries = NULL;
    entry_count = 0;
    
    if (!found_entry)
    {
        XFS_DEBUG("https: stat failed: entry '%s' not found in directory '%s'\n", entry_name, dir_path);
        return XFS_ERR_NOENT;
    }
    
    // Found the entry, fill stat structure
    memset(stat_info, 0, sizeof(*stat_info));
    
    if (found_entry->is_dir)
    {
        stat_info->type = XFS_TYPE_DIR;
        stat_info->size = 0;
    }
    else
    {
        stat_info->type = XFS_TYPE_REG;
        stat_info->size = found_entry->size;
    }
    
    // Copy name
    size_t name_len = strlen(found_entry->name);
    if (name_len >= sizeof(stat_info->name))
    {
        name_len = sizeof(stat_info->name) - 1;
    }
    memcpy(stat_info->name, found_entry->name, name_len);
    stat_info->name[name_len] = '\0';
    
    XFS_DEBUG("https: stat success: type=%d size=%lu\n", 
              stat_info->type, (unsigned long)stat_info->size);
    return XFS_ERR_OK;
}

// Unlink not supported
static int16_t https_unlink(const struct xfs_engine_mount_t* engine, const char* path)
{
    (void)engine;
    (void)path;
    return XFS_ERR_INVAL;
}

// Mkdir not supported
static int16_t https_mkdir(const struct xfs_engine_mount_t* engine, const char* path)
{
    (void)engine;
    (void)path;
    return XFS_ERR_INVAL;
}

// Rmdir not supported
static int16_t https_rmdir(const struct xfs_engine_mount_t* engine, const char* path)
{
    (void)engine;
    (void)path;
    return XFS_ERR_INVAL;
}

// Chdir - always succeeds (virtual directories)
static int16_t https_chdir(const struct xfs_engine_mount_t* engine, const char* path)
{
    (void)engine;
    (void)path;
    return XFS_ERR_OK;
}

// Getcwd - always return root
static int16_t https_getcwd(const struct xfs_engine_mount_t* engine, char* buffer, uint16_t size)
{
    (void)engine;
    if (size < 2)
    {
        return XFS_ERR_INVAL;
    }
    strncpy(buffer, "/", size - 1);
    buffer[size - 1] = '\0';
    return XFS_ERR_OK;
}

// Rename not supported
static int16_t https_rename(const struct xfs_engine_mount_t* engine, const char* old_path, const char* new_path)
{
    (void)engine;
    (void)old_path;
    (void)new_path;
    return XFS_ERR_INVAL;
}

// Free handle resources
static void https_free_handle(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle)
{
    (void)engine;

    if (handle->type == XFS_HANDLE_TYPE_FILE)
    {
        struct xfs_handle_https_file_t* https_handle = get_https_file_handle(handle);

        // Free buffer if exists
        if (https_handle)
        {
          if (https_handle->data)
          {
              libspectrum_free(https_handle->data);
              https_handle->data = NULL;
          }

          libspectrum_free(https_handle);
        }
    }
    else if (handle->type == XFS_HANDLE_TYPE_DIR)
    {
        struct xfs_handle_https_dir_t* https_handle = get_https_dir_handle(handle);

        // Free directory entries
        if (https_handle && https_handle->dir_entries)
        {
            for (uint8_t i = 0; i < https_handle->dir_entries_count; i++)
            {
                if (https_handle->dir_entries[i])
                {
                    libspectrum_free(https_handle->dir_entries[i]);
                }
            }
            libspectrum_free(https_handle->dir_entries);
            https_handle->dir_entries = NULL;
            https_handle->dir_entries_count = 0;
            libspectrum_free(https_handle);
        }

    }

    handle->data = NULL;
}

// HTTPS engine instance
struct xfs_engine_t https_engine = {
    .user = NULL,
    .mount = https_mount,
    .unmount = https_unmount,
    .open = https_open,
    .read = https_read,
    .write = https_write,
    .close = https_close,
    .lseek = https_lseek,
    .opendir = https_opendir,
    .readdir = https_readdir,
    .closedir = https_closedir,
    .stat = https_stat,
    .unlink = https_unlink,
    .mkdir = https_mkdir,
    .rmdir = https_rmdir,
    .chdir = https_chdir,
    .getcwd = https_getcwd,
    .rename = https_rename,
    .free_handle = https_free_handle,
};
