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
static bool xfs_debug_enabled = true;

// Macro for XFS debug output (only prints if enabled)
#define XFS_DEBUG(...) do { if (xfs_debug_enabled) printf(__VA_ARGS__); } while(0)

// Function to enable/disable XFS debug logging
void xfs_debug_enable(bool enable)
{
    xfs_debug_enabled = enable;
}

bool xfs_debug_is_enabled(void)
{
    return xfs_debug_enabled;
}

// XFS base directory path
static char *xfs_base_path = NULL;

// Track mount state
static bool xfs_mounted = false;

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
    int fd;         // File descriptor
    DIR *dir;       // Directory handle
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
            xfs_handles[i].fd = -1;
            xfs_handles[i].dir = NULL;
            return i + 1;
        }
    }
    return 0; // No free handle
}

static inline void free_handle(uint8_t handle)
{
    xfs_handle_t* h = get_handle(handle);
    if (h)
    {
        h->fd = -1;
        h->dir = NULL;
        h->type = XFS_HANDLE_TYPE_NONE;
    }
}

// Helper function to ensure filesystem is mounted before operations
static bool ensure_mounted(void)
{
    if (!xfs_mounted)
    {
        return false;
    }
    return true;
}

// Helper function to build a full path
// Returns pointer to static buffer (not thread-safe, but we're single-threaded)
static char* build_path(const char* path)
{
    static char full_path[PATH_MAX];
    // Ensure path starts with / (like RP2350 version)
    if (path[0] != '/')
    {
        // Prepend / if missing
        if (xfs_base_path)
        {
            snprintf(full_path, sizeof(full_path), "%s/%s", xfs_base_path, path);
        }
        else
        {
            full_path[0] = '/';
            strncpy(full_path + 1, path, sizeof(full_path) - 2);
            full_path[sizeof(full_path) - 1] = '\0';
        }
    }
    else
    {
        if (xfs_base_path)
        {
            snprintf(full_path, sizeof(full_path), "%s%s", xfs_base_path, path);
        }
        else
        {
            strncpy(full_path, path, sizeof(full_path) - 1);
            full_path[sizeof(full_path) - 1] = '\0';
        }
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
                xfs_registers.result = -EINVAL;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
        
            xfs_mounted = true;
            XFS_DEBUG("xfs: mount success\n");
            xfs_registers.result = 0;
            xfs_registers.status = XFS_STATUS_COMPLETE;
          
            break;
        }
        case XFS_CMD_OPEN:
        {
            const char* path = (const char*)xfs_registers.arguments.open.path;
            uint16_t flags = xfs_registers.arguments.open.flags;  // 16-bit POSIX flags
            uint16_t mode = xfs_registers.arguments.open.mode;    // 16-bit POSIX mode
            XFS_DEBUG("xfs: open path=%s flags=0x%04x mode=0x%04x\n", path, flags, mode);
            
            if (!ensure_mounted())
            {
                XFS_DEBUG("xfs: open failed: not mounted\n");
                xfs_registers.result = -EIO;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
            uint8_t handle = allocate_handle(XFS_HANDLE_TYPE_FILE);
            if (handle == 0)
            {
                XFS_DEBUG("xfs: open failed: too many open files\n");
                xfs_registers.result = -EMFILE;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
            // Convert POSIX flags to open flags (following RP2350 pattern)
            // flags contains POSIX open flags: O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC, etc.
            // mode contains file permissions (not used for open flags)
            int open_flags = 0;
            
            // Determine access mode from flags (POSIX O_RDONLY=0, O_WRONLY=1, O_RDWR=2)
            uint8_t accmode = flags & 0x0003;
            if (accmode == 0x0001) // O_RDONLY
                open_flags = O_RDONLY;
            else if (accmode == 0x0002) // O_WRONLY
                open_flags = O_WRONLY;
            else if (accmode == 0x0003) // O_RDWR
                open_flags = O_RDWR;
            else
            {
                // Invalid access mode, default to read-only
                open_flags = O_RDONLY;
            }
            
            // Add other flags individually (like RP2350 version)
            if (flags & 0x0008) // O_APPEND
                open_flags |= O_APPEND;
            if (flags & 0x0100) // O_CREAT
            {
                open_flags |= O_CREAT;
                // If O_CREAT is set and access mode is write-only, upgrade to read-write
                // This allows reading from newly created files
                if (accmode == 0x0001) // O_WRONLY
                {
                    XFS_DEBUG("xfs: open O_CREAT with O_WRONLY, upgrading to O_RDWR\n");
                    open_flags = (open_flags & ~O_ACCMODE) | O_RDWR;
                }
            }
            if (flags & 0x0200) // O_TRUNC
            {
                open_flags |= O_CREAT | O_TRUNC;
                // If O_TRUNC is set, also upgrade write-only to read-write
                if (accmode == 0x0001) // O_WRONLY
                {
                    XFS_DEBUG("xfs: open O_TRUNC with O_WRONLY, upgrading to O_RDWR\n");
                    open_flags = (open_flags & ~O_ACCMODE) | O_RDWR;
                }
            }
            
            XFS_DEBUG("xfs: open decoded flags: accmode=0x%02x open_flags=0x%x (O_RDONLY=%d O_WRONLY=%d O_RDWR=%d)\n", 
                accmode, open_flags, O_RDONLY, O_WRONLY, O_RDWR);
            
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
                XFS_DEBUG("xfs: open success handle=%d fd=%d\n", handle, h->fd);
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
            XFS_DEBUG("xfs: read handle=%d size=%d remaining=%d total=%d\n", handle, size,
                xfs_registers.fops.remaining, xfs_registers.fops.total);
            
            xfs_handle_t* h = get_handle(handle);
            if (h == NULL || h->type != XFS_HANDLE_TYPE_FILE)
            {
                XFS_DEBUG("xfs: read failed: invalid handle (h=%p type=%d)\n", (void*)h, h ? h->type : -1);
                xfs_registers.result = -EBADF;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
            ssize_t bytes_read = read(h->fd, xfs_registers.workspace, size);
            
            if (bytes_read < 0)
            {
                XFS_DEBUG("xfs: read failed: fd=%d errno=%d (%s)\n", h->fd, errno, strerror(errno));
                xfs_registers.result = xfs_error_from_errno(errno);
                xfs_registers.status = XFS_STATUS_ERROR;
            }
            else
            {
                XFS_DEBUG("xfs: read success bytes=%zd\n", bytes_read);
                xfs_registers.result = (int16_t)bytes_read; // Return bytes read
                xfs_registers.status = XFS_STATUS_COMPLETE;
            }
            break;
        }
        case XFS_CMD_WRITE:
        {
            uint8_t handle = xfs_registers.file_handle;
            uint16_t size = xfs_registers.arguments.write.size;

            XFS_DEBUG("xfs: write handle=%d size=%d remaining=%d total=%d\n", handle, size,
                xfs_registers.fops.remaining, xfs_registers.fops.total);

            // Data is in workspace section
            
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
                xfs_registers.result = (int16_t)bytes_written; // Return bytes written
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
            
            if (!ensure_mounted())
            {
                XFS_DEBUG("xfs: opendir failed: not mounted\n");
                xfs_registers.result = -EIO;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }

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
            
            struct dirent *entry;
            int err = 0;
            
            // Read directory entries, skipping "." and ".."
            // Clear errno before calling readdir to avoid false errors from previous operations
            errno = 0;
            do
            {
                entry = readdir(h->dir);
                if (entry == NULL)
                {
                    if (errno != 0)
                    {
                        err = errno;
                    }
                    break; // End of directory or error
                }
                // Skip "." and ".." entries
            } while (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0);
            
            if (err != 0)
            {
                XFS_DEBUG("xfs: readdir failed: %s\n", strerror(err));
                xfs_registers.result = xfs_error_from_errno(err);
                xfs_registers.status = XFS_STATUS_ERROR;
            }
            else if (entry == NULL)
            {
                uint8_t eod = 1; // 1 means end of directory
                XFS_DEBUG("xfs: readdir end of directory\n");
                xfs_registers.result = eod;
                xfs_registers.status = XFS_STATUS_COMPLETE;
            }
            else
            {
                uint8_t eod = 0; // 0 means not end of directory
                XFS_DEBUG("xfs: readdir success eod=%d name=%s\n", eod, entry->d_name);
                
                // Copy just the filename string to workspace
                strcpy((char*)xfs_registers.workspace, entry->d_name);
                
                xfs_registers.result = eod; // 0 = not end of directory
                xfs_registers.status = XFS_STATUS_COMPLETE;
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
            
            if (!ensure_mounted())
            {
                XFS_DEBUG("xfs: stat failed: not mounted\n");
                xfs_registers.result = -EIO;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
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
                // Default permissions: 0x0644 (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
                stat_out->mode = 0x0644;
                if (S_ISDIR(st.st_mode))
                {
                    stat_out->mode |= 0x4000; // S_IFDIR
                }
                else if (S_ISREG(st.st_mode))
                {
                    stat_out->mode |= 0x8000; // S_IFREG
                }
                
                // uid, gid: from stat
                stat_out->uid = (uint16_t)st.st_uid;
                stat_out->gid = (uint16_t)st.st_gid;
                
                // size: file size from stat
                stat_out->size = (uint32_t)st.st_size;
                
                // atime, mtime, ctime: from stat
                stat_out->atime = (uint32_t)st.st_atime;
                stat_out->mtime = (uint32_t)st.st_mtime;
                stat_out->ctime = (uint32_t)st.st_ctime;
                
                // strings: null-terminated uid and gid strings (empty for now)
                // These follow the struct at offset 0x16 (22 bytes)
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
            
            if (!ensure_mounted())
            {
                XFS_DEBUG("xfs: unlink failed: not mounted\n");
                xfs_registers.result = -EIO;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
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
            
            if (!ensure_mounted())
            {
                XFS_DEBUG("xfs: mkdir failed: not mounted\n");
                xfs_registers.result = -EIO;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
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
            
            if (!ensure_mounted())
            {
                XFS_DEBUG("xfs: rmdir failed: not mounted\n");
                xfs_registers.result = -EIO;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
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
            
            if (!ensure_mounted())
            {
                XFS_DEBUG("xfs: chdir failed: not mounted\n");
                xfs_registers.result = -EIO;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
            // Verify path exists (as a workaround, similar to RP2350)
            char* full_path = build_path(path);
            struct stat st;
            int err = stat(full_path, &st);
            
            if (err != 0 || !S_ISDIR(st.st_mode))
            {
                XFS_DEBUG("xfs: chdir failed: path not found or not a directory\n");
                xfs_registers.result = -ENOENT;
                xfs_registers.status = XFS_STATUS_ERROR;
            }
            else
            {
                // For FUSE, we can actually change directory
                if (chdir(full_path) != 0)
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
            }
            break;
        }
        case XFS_CMD_GETCWD:
        {
            XFS_DEBUG("xfs: getcwd\n");
            if (!ensure_mounted())
            {
                XFS_DEBUG("xfs: getcwd failed: not mounted\n");
                xfs_registers.result = -EIO;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
            char* path = (char*)xfs_registers.workspace;
            uint16_t len = xfs_registers.arguments.getcwd.buffer_size;
            
            if (len < 2)
            {
                xfs_registers.result = -EINVAL;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
            // For FUSE, get actual current directory, but fallback to "/" if needed
            if (getcwd(path, len) == NULL)
            {
                // Fallback to root if getcwd fails
                strncpy(path, "/", len - 1);
                path[len - 1] = '\0';
            }
            
            XFS_DEBUG("xfs: getcwd success path=%s\n", path);
            xfs_registers.result = 0;
            xfs_registers.status = XFS_STATUS_COMPLETE;
            break;
        }
        case XFS_CMD_RENAME:
        {
            const char* old_path = (const char*)xfs_registers.arguments.rename.old_path;
            const char* new_path = (const char*)xfs_registers.arguments.rename.new_path;
            XFS_DEBUG("xfs: rename old=%s new=%s\n", old_path, new_path);
            
            if (!ensure_mounted())
            {
                XFS_DEBUG("xfs: rename failed: not mounted\n");
                xfs_registers.result = -EIO;
                xfs_registers.status = XFS_STATUS_ERROR;
                break;
            }
            
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
            {
                posix_whence = SEEK_SET;
            }
            else if (whence == 1) // SEEK_CUR
            {
                posix_whence = SEEK_CUR;
            }
            else if (whence == 2) // SEEK_END
            {
                posix_whence = SEEK_END;
            }
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
                *(uint32_t*)&xfs_registers.workspace[0] = (uint32_t)new_pos; // Return new position
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

// Unmount the filesystem and close all handles
void xfs_unmount(void)
{
    XFS_DEBUG("xfs: unmount - closing all handles\n");
    
    // Close all open handles
    for (uint8_t i = 0; i < XFS_MAX_FDS; i++)
    {
        if (xfs_handles[i].type != XFS_HANDLE_TYPE_NONE)
        {
            uint8_t handle = i + 1; // Convert to 1-based handle
            xfs_handle_t* h = &xfs_handles[i];
            
            if (h->type == XFS_HANDLE_TYPE_FILE)
            {
                close(h->fd);
            }
            else if (h->type == XFS_HANDLE_TYPE_DIR)
            {
                closedir(h->dir);
            }
            
            free_handle(handle);
        }
    }
    
    // Clear mount state
    xfs_mounted = false;
    
    XFS_DEBUG("xfs: unmount complete\n");
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
