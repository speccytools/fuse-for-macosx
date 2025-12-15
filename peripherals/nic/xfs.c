#include "config.h"

#include "xfs.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>

#include "libspectrum.h"
#include "memory_pages.h"
#include "ui/ui.h"
#include "compat.h"

volatile struct xfs_registers_t xfs_registers = {};

// XFS debug logging control
static bool xfs_debug_enabled = false;

// Macro for XFS debug output (only prints if enabled)
#define XFS_DEBUG(...) printf(__VA_ARGS__);

// XFS base directory path
static char *xfs_base_path = NULL;

// File/directory handle management
typedef enum
{
    XFS_HANDLE_TYPE_NONE = 0,
    XFS_HANDLE_TYPE_FILE = 1,
    XFS_HANDLE_TYPE_DIR = 2,
} xfs_handle_type_t;

typedef struct
{
    xfs_handle_type_t type;
    union
    {
        int fd;         // File descriptor
        DIR *dir;       // Directory handle
    };
} xfs_handle_t;

static xfs_handle_t xfs_handles[XFS_MAX_FDS] = {};

static inline xfs_handle_t* get_handle(uint8_t handle)
{
    if (handle < 1 || handle > XFS_MAX_FDS)
        return NULL;
    return &xfs_handles[handle - 1];
}

static inline uint8_t allocate_handle(xfs_handle_type_t type)
{
    for (uint8_t i = 0; i < XFS_MAX_FDS; i++)
    {
        if (xfs_handles[i].type == XFS_HANDLE_TYPE_NONE)
        {
            xfs_handles[i].type = type;
            return i + 1; // Return 1-based handle
        }
    }
    return 0; // No free handle
}

static inline void free_handle(uint8_t handle)
{
    xfs_handle_t* h = get_handle(handle);
    if (h)
    {
        h->type = XFS_HANDLE_TYPE_NONE;
    }
}

// Helper function to build a full path
// Returns pointer to static buffer
static char* build_path(const char* path)
{
    static char full_path[PATH_MAX];
    if (xfs_base_path)
    {
        snprintf(full_path, sizeof(full_path), "%s/%s", xfs_base_path, path);
    }
    else
    {
        strncpy(full_path, path, sizeof(full_path) - 1);
        full_path[sizeof(full_path) - 1] = '\0';
    }
    return full_path;
}

// Convert POSIX errno to XFS error code (use negative errno)
static int16_t xfs_error_from_errno(int err)
{
    return (err != 0) ? -err : 0;
}

// Process a single XFS command
static void xfs_process_command(void)
{
    const uint8_t command = xfs_registers.command;
    
    if (command == 0)
        return;
    
    xfs_registers.command = 0;
    xfs_registers.result = 0;
    xfs_registers.status = XFS_STATUS_BUSY;
    
    switch (command)
    {
        case XFS_CMD_MOUNT:
        {
            const char* hostname = (const char*)xfs_registers.arguments.mount.hostname;
            XFS_DEBUG("xfs: mount hostname='%s'\n", hostname);
          
            if (strcmp(hostname, "ram") != 0)
            {
                XFS_DEBUG("xfs: mount failed: invalid hostname\n");
                xfs_registers.result = 6; // FR_INVALID_NAME
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
        
            XFS_DEBUG("xfs: mount successly\n");
            xfs_registers.result = 0;
            xfs_registers.status = XFS_STATUS_COMPLETE;
          
            break;
        }
        case XFS_CMD_OPEN:
        {
            const char* path = (const char*)xfs_registers.arguments.open.path;
            uint16_t flags = xfs_registers.arguments.open.flags;
            uint16_t mode = xfs_registers.arguments.open.mode;
            XFS_DEBUG("xfs: open path=%s flags=0x%04x mode=0x%04x\n", path, flags, mode);
            
            uint8_t handle = allocate_handle(XFS_HANDLE_TYPE_FILE);
            if (handle == 0)
            {
                XFS_DEBUG("xfs: open failed: too many open files\n");
                xfs_registers.result = -EMFILE;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
            // Convert POSIX flags
            int open_flags = 0;
            uint8_t access = flags & 0x0003;
            if (access == 0x0001 || access == 0) // O_RDONLY
                open_flags = O_RDONLY;
            else if (access == 0x0002) // O_WRONLY
                open_flags = O_WRONLY;
            else if (access == 0x0003) // O_RDWR
                open_flags = O_RDWR;
            else
                open_flags = O_RDONLY;
            
            if (flags & 0x0100) // O_CREAT
                open_flags |= (flags & 0x0400) ? O_CREAT | O_EXCL : O_CREAT;
            if (flags & 0x0200) // O_TRUNC
                open_flags |= O_TRUNC;
            
            xfs_handle_t* h = get_handle(handle);
            char* full_path = build_path(path);
            h->fd = open(full_path, open_flags, mode);
            
            if (h->fd < 0)
            {
                XFS_DEBUG("xfs: open failed: %s\n", strerror(errno));
                free_handle(handle);
                xfs_registers.result = xfs_error_from_errno(errno);
                xfs_registers.status = XFS_STATUS_ERROR;
            }
            else
            {
                XFS_DEBUG("xfs: open success handle=%d\n", handle);
                xfs_registers.file_handle = handle;
                xfs_registers.result = 0;
                xfs_registers.status = XFS_STATUS_COMPLETE;
            }
            break;
        }
        case XFS_CMD_READ:
        {
            uint8_t handle = xfs_registers.file_handle;
            uint16_t size = xfs_registers.arguments.read.size;
            XFS_DEBUG("xfs: read handle=%d size=%d\n", handle, size);
            
            xfs_handle_t* h = get_handle(handle);
            if (h == NULL || h->type != XFS_HANDLE_TYPE_FILE)
            {
                XFS_DEBUG("xfs: read failed: invalid handle\n");
                xfs_registers.result = -EBADF;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
            ssize_t bytes_read = read(h->fd, xfs_registers.workspace, size);
            
            if (bytes_read < 0)
            {
                XFS_DEBUG("xfs: read failed: %s\n", strerror(errno));
                xfs_registers.result = xfs_error_from_errno(errno);
                xfs_registers.status = XFS_STATUS_ERROR;
            }
            else
            {
                XFS_DEBUG("xfs: read success bytes=%zd\n", bytes_read);
                xfs_registers.result = (int16_t)bytes_read;
                xfs_registers.status = XFS_STATUS_COMPLETE;
            }
            break;
        }
        case XFS_CMD_WRITE:
        {
            uint8_t handle = xfs_registers.file_handle;
            uint16_t size = xfs_registers.arguments.write.size;
            XFS_DEBUG("xfs: write handle=%d size=%d\n", handle, size);
            
            xfs_handle_t* h = get_handle(handle);
            if (h == NULL || h->type != XFS_HANDLE_TYPE_FILE)
            {
                XFS_DEBUG("xfs: write failed: invalid handle\n");
                xfs_registers.result = -EBADF;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
            ssize_t bytes_written = write(h->fd, xfs_registers.workspace, size);
            
            if (bytes_written < 0)
            {
                XFS_DEBUG("xfs: write failed: %s\n", strerror(errno));
                xfs_registers.result = xfs_error_from_errno(errno);
                xfs_registers.status = XFS_STATUS_ERROR;
            }
            else
            {
                XFS_DEBUG("xfs: write success bytes=%zd\n", bytes_written);
                xfs_registers.result = (int16_t)bytes_written;
                xfs_registers.status = XFS_STATUS_COMPLETE;
            }
            break;
        }
        case XFS_CMD_CLOSE:
        {
            uint8_t handle = xfs_registers.file_handle;
            XFS_DEBUG("xfs: close handle=%d\n", handle);
            xfs_handle_t* h = get_handle(handle);
            
            if (h == NULL)
            {
                XFS_DEBUG("xfs: close failed: invalid handle\n");
                xfs_registers.result = -EBADF;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
            int ret = 0;
            if (h->type == XFS_HANDLE_TYPE_FILE)
            {
                ret = close(h->fd);
            }
            else if (h->type == XFS_HANDLE_TYPE_DIR)
            {
                ret = closedir(h->dir);
            }
            
            free_handle(handle);
            
            if (ret != 0)
            {
                XFS_DEBUG("xfs: close failed: %s\n", strerror(errno));
                xfs_registers.result = xfs_error_from_errno(errno);
                xfs_registers.status = XFS_STATUS_ERROR;
            }
            else
            {
                XFS_DEBUG("xfs: close success\n");
                xfs_registers.result = 0;
                xfs_registers.status = XFS_STATUS_COMPLETE;
            }
            break;
        }
        case XFS_CMD_OPENDIR:
        {
            const char* path = (const char*)xfs_registers.arguments.opendir.path;
            XFS_DEBUG("xfs: opendir path=%s\n", path);
            
            uint8_t handle = allocate_handle(XFS_HANDLE_TYPE_DIR);
            if (handle == 0)
            {
                XFS_DEBUG("xfs: opendir failed: too many open files\n");
                xfs_registers.result = -EMFILE;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
            xfs_handle_t* h = get_handle(handle);
            char* full_path = build_path(path);
            h->dir = opendir(full_path);
            
            if (h->dir == NULL)
            {
                XFS_DEBUG("xfs: opendir %s failed: %s\n", full_path, strerror(errno));
                free_handle(handle);
                xfs_registers.result = xfs_error_from_errno(errno);
                xfs_registers.status = XFS_STATUS_ERROR;
            }
            else
            {
                XFS_DEBUG("xfs: opendir %s success handle=%d\n", full_path, handle);
                xfs_registers.result = 0;
                xfs_registers.file_handle = handle;
                xfs_registers.status = XFS_STATUS_COMPLETE;
            }
            break;
        }
        case XFS_CMD_READDIR:
        {
            uint8_t handle = xfs_registers.file_handle;
            XFS_DEBUG("xfs: readdir handle=%d\n", handle);
            xfs_handle_t* h = get_handle(handle);
            
            if (h == NULL || h->type != XFS_HANDLE_TYPE_DIR)
            {
                XFS_DEBUG("xfs: readdir failed: invalid handle\n");
                xfs_registers.result = -EBADF;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
            // Read directory entries, skipping "." and ".."
            // Clear errno before calling readdir to avoid false errors from previous operations
            errno = 0;
            struct dirent *entry;
            while ((entry = readdir(h->dir)) != NULL)
            {
                // Skip "." and ".." entries
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                    continue;
                
                // Found a valid entry
                XFS_DEBUG("xfs: readdir success name=%s\n", entry->d_name);
                strncpy((char*)xfs_registers.workspace, entry->d_name, 1023);
                xfs_registers.workspace[1023] = '\0';
                xfs_registers.result = 0; // Not end of directory
                xfs_registers.status = XFS_STATUS_COMPLETE;
                break;
            }
            
            // If we exited the loop, either end of directory or error
            if (entry == NULL)
            {
                if (errno != 0)
                {
                    XFS_DEBUG("xfs: readdir failed: %s\n", strerror(errno));
                    xfs_registers.result = xfs_error_from_errno(errno);
                    xfs_registers.status = XFS_STATUS_ERROR;
                }
                else
                {
                    // End of directory
                    XFS_DEBUG("xfs: readdir end of directory\n");
                    xfs_registers.result = 1; // End of directory
                    xfs_registers.status = XFS_STATUS_COMPLETE;
                }
            }
            break;
        }
        case XFS_CMD_CLOSEDIR:
        {
            uint8_t handle = xfs_registers.file_handle;
            XFS_DEBUG("xfs: closedir handle=%d\n", handle);
            xfs_handle_t* h = get_handle(handle);
            
            if (h == NULL || h->type != XFS_HANDLE_TYPE_DIR)
            {
                XFS_DEBUG("xfs: closedir failed: invalid handle\n");
                xfs_registers.result = -EBADF;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
            int ret = closedir(h->dir);
            free_handle(handle);
            
            if (ret != 0)
            {
                XFS_DEBUG("xfs: closedir failed: %s\n", strerror(errno));
                xfs_registers.result = xfs_error_from_errno(errno);
                xfs_registers.status = XFS_STATUS_ERROR;
            }
            else
            {
                XFS_DEBUG("xfs: closedir success\n");
                xfs_registers.result = 0;
                xfs_registers.status = XFS_STATUS_COMPLETE;
            }
            break;
        }
        case XFS_CMD_STAT:
        {
            const char* path = (const char*)xfs_registers.arguments.stat.path;
            XFS_DEBUG("xfs: stat path=%s\n", path);
            
            char* full_path = build_path(path);
            struct stat st;
            int ret = stat(full_path, &st);
            
            if (ret != 0)
            {
                XFS_DEBUG("xfs: stat failed: %s\n", strerror(errno));
                xfs_registers.result = xfs_error_from_errno(errno);
                xfs_registers.status = XFS_STATUS_ERROR;
            }
            else
            {
                // Convert struct stat to Spectranet stat structure format
                struct xfs_stat_t* stat_out = (struct xfs_stat_t*)xfs_registers.workspace;
                
                // mode: S_IFDIR = 0x4000, S_IFREG = 0x8000
                stat_out->mode = st.st_mode & 0777; // Permissions
                if (S_ISDIR(st.st_mode))
                    stat_out->mode |= 0x4000; // S_IFDIR
                else if (S_ISREG(st.st_mode))
                    stat_out->mode |= 0x8000; // S_IFREG
                
                stat_out->uid = (uint16_t)st.st_uid;
                stat_out->gid = (uint16_t)st.st_gid;
                stat_out->size = (uint32_t)st.st_size;
                stat_out->atime = (uint32_t)st.st_atime;
                stat_out->mtime = (uint32_t)st.st_mtime;
                stat_out->ctime = (uint32_t)st.st_ctime;
                
                // strings: null-terminated uid and gid strings (empty for now)
                uint8_t* strings = (uint8_t*)xfs_registers.workspace + 22;
                strings[0] = 0; // uid string (empty)
                strings[1] = 0; // gid string (empty)
                
                XFS_DEBUG("xfs: stat success size=%lu mode=0x%04x\n", st.st_size, stat_out->mode);
                xfs_registers.result = 0;
                xfs_registers.status = XFS_STATUS_COMPLETE;
            }
            break;
        }
        case XFS_CMD_UNLINK:
        {
            const char* path = (const char*)xfs_registers.arguments.unlink.path;
            XFS_DEBUG("xfs: unlink path=%s\n", path);
            
            char* full_path = build_path(path);
            int ret = unlink(full_path);
            
            if (ret != 0)
            {
                XFS_DEBUG("xfs: unlink failed: %s\n", strerror(errno));
                xfs_registers.result = xfs_error_from_errno(errno);
                xfs_registers.status = XFS_STATUS_ERROR;
            }
            else
            {
                XFS_DEBUG("xfs: unlink success\n");
                xfs_registers.result = 0;
                xfs_registers.status = XFS_STATUS_COMPLETE;
            }
            break;
        }
        case XFS_CMD_MKDIR:
        {
            const char* path = (const char*)xfs_registers.arguments.mkdir.path;
            XFS_DEBUG("xfs: mkdir path=%s\n", path);
            
            char* full_path = build_path(path);
            int ret = mkdir(full_path, 0755);
            
            if (ret != 0)
            {
                XFS_DEBUG("xfs: mkdir failed: %s\n", strerror(errno));
                xfs_registers.result = xfs_error_from_errno(errno);
                xfs_registers.status = XFS_STATUS_ERROR;
            }
            else
            {
                XFS_DEBUG("xfs: mkdir success\n");
                xfs_registers.result = 0;
                xfs_registers.status = XFS_STATUS_COMPLETE;
            }
            break;
        }
        case XFS_CMD_RMDIR:
        {
            const char* path = (const char*)xfs_registers.arguments.rmdir.path;
            XFS_DEBUG("xfs: rmdir path=%s\n", path);
            
            char* full_path = build_path(path);
            int ret = rmdir(full_path);
            
            if (ret != 0)
            {
                XFS_DEBUG("xfs: rmdir failed: %s\n", strerror(errno));
                xfs_registers.result = xfs_error_from_errno(errno);
                xfs_registers.status = XFS_STATUS_ERROR;
            }
            else
            {
                XFS_DEBUG("xfs: rmdir success\n");
                xfs_registers.result = 0;
                xfs_registers.status = XFS_STATUS_COMPLETE;
            }
            break;
        }
        case XFS_CMD_CHDIR:
        {
            const char* path = (const char*)xfs_registers.arguments.chdir.path;
            XFS_DEBUG("xfs: chdir path=%s\n", path);
            
            char* full_path = build_path(path);
            int ret = chdir(full_path);
            
            if (ret != 0)
            {
                XFS_DEBUG("xfs: chdir failed: %s\n", strerror(errno));
                xfs_registers.result = xfs_error_from_errno(errno);
                xfs_registers.status = XFS_STATUS_ERROR;
            }
            else
            {
                XFS_DEBUG("xfs: chdir success\n");
                xfs_registers.result = 0;
                xfs_registers.status = XFS_STATUS_COMPLETE;
            }
            break;
        }
        case XFS_CMD_GETCWD:
        {
            XFS_DEBUG("xfs: getcwd\n");
            
            char* path = (char*)xfs_registers.workspace;
            uint16_t len = xfs_registers.arguments.getcwd.buffer_size;
            
            if (getcwd(path, len) == NULL)
            {
                XFS_DEBUG("xfs: getcwd failed: %s\n", strerror(errno));
                xfs_registers.result = xfs_error_from_errno(errno);
                xfs_registers.status = XFS_STATUS_ERROR;
            }
            else
            {
                XFS_DEBUG("xfs: getcwd success path=%s\n", path);
                xfs_registers.result = 0;
                xfs_registers.status = XFS_STATUS_COMPLETE;
            }
            break;
        }
        case XFS_CMD_RENAME:
        {
            const char* old_path = (const char*)xfs_registers.arguments.rename.old_path;
            const char* new_path = (const char*)xfs_registers.arguments.rename.new_path;
            XFS_DEBUG("xfs: rename old=%s new=%s\n", old_path, new_path);
            
            char* full_old_path = build_path(old_path);
            char* full_new_path = build_path(new_path);
            int ret = rename(full_old_path, full_new_path);
            
            if (ret != 0)
            {
                XFS_DEBUG("xfs: rename failed: %s\n", strerror(errno));
                xfs_registers.result = xfs_error_from_errno(errno);
                xfs_registers.status = XFS_STATUS_ERROR;
            }
            else
            {
                XFS_DEBUG("xfs: rename success\n");
                xfs_registers.result = 0;
                xfs_registers.status = XFS_STATUS_COMPLETE;
            }
            break;
        }
        case XFS_CMD_LSEEK:
        {
            uint8_t handle = xfs_registers.file_handle;
            off_t offset = (off_t)xfs_registers.arguments.lseek.offset;
            uint8_t whence = xfs_registers.arguments.lseek.whence;
            XFS_DEBUG("xfs: lseek handle=%d offset=%ld whence=%d\n", handle, (long)offset, whence);
            
            xfs_handle_t* h = get_handle(handle);
            if (h == NULL || h->type != XFS_HANDLE_TYPE_FILE)
            {
                XFS_DEBUG("xfs: lseek failed: invalid handle\n");
                xfs_registers.result = -EBADF;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
            int posix_whence;
            if (whence == 0) // SEEK_SET
                posix_whence = SEEK_SET;
            else if (whence == 1) // SEEK_CUR
                posix_whence = SEEK_CUR;
            else if (whence == 2) // SEEK_END
                posix_whence = SEEK_END;
            else
            {
                XFS_DEBUG("xfs: lseek failed: invalid whence\n");
                xfs_registers.result = -EINVAL;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
            off_t new_pos = lseek(h->fd, offset, posix_whence);
            
            if (new_pos == (off_t)-1)
            {
                XFS_DEBUG("xfs: lseek failed: %s\n", strerror(errno));
                xfs_registers.result = xfs_error_from_errno(errno);
                xfs_registers.status = XFS_STATUS_ERROR;
            }
            else
            {
                XFS_DEBUG("xfs: lseek success new_pos=%ld\n", (long)new_pos);
                *(uint32_t*)&xfs_registers.workspace[0] = (uint32_t)new_pos;
                xfs_registers.result = 0;
                xfs_registers.status = XFS_STATUS_COMPLETE;
            }
            break;
        }
        default:
        {
            XFS_DEBUG("xfs: unknown command=%d\n", command);
            xfs_registers.result = -EINVAL;
            xfs_registers.status = XFS_STATUS_ERROR;
            break;
        }
    }
}

void xfs_init()
{
    const char *home_dir = compat_get_config_path();
    size_t home_len = strlen(home_dir);
    size_t xfs_path_len = home_len + strlen("/xfs") + 1;
    
    // Allocate memory for full path: home_dir + "/xfs"
    xfs_base_path = libspectrum_malloc( xfs_path_len );
    if (xfs_base_path)
    {
        snprintf(xfs_base_path, xfs_path_len, "%s/xfs", home_dir);
        
        // Create "xfs" directory in user's home directory
        if (mkdir(xfs_base_path, 0755) != 0 && errno != EEXIST)
        {
            ui_error( UI_ERROR_WARNING, "xfs: failed to create xfs directory: %s\n", strerror(errno) );
        }
    }
    else
    {
        ui_error( UI_ERROR_WARNING, "xfs: failed to allocate memory for base path\n" );
    }
    
    XFS_DEBUG("xfs: initialized with base path: %s\n", xfs_base_path ? xfs_base_path : "(null)");
}

libspectrum_byte xfs_read( memory_page *page GCC_UNUSED, libspectrum_word address )
{
    libspectrum_word offset = address & 0xfff;  // XFS is single page (0x49)
    uint8_t *registers = (uint8_t*)&xfs_registers;
    return registers[offset];
}

void xfs_write( memory_page *page GCC_UNUSED, libspectrum_word address, libspectrum_byte b )
{
    libspectrum_word offset = address & 0xfff;
    uint8_t *registers = (uint8_t*)&xfs_registers;
    registers[offset] = b;
    
    if (offset == 0)
    {
        xfs_process_command();
    }
}
