#include "xfs.h"
#include "xfs_engines.h"
#include <stdio.h>
#include <string.h>

// Mounted engines array - shared between task and emulator
struct xfs_engine_mount_t xfs_mounted_engines[4] = {0};

// Handles array - shared between task and emulator  
struct xfs_handle_t xfs_handles[XFS_MAX_FDS] = {0};

#define XFS_DEBUG(...) do { if (xfs_debug_is_enabled()) printf(__VA_ARGS__); } while(0)

// Helper functions
static inline struct xfs_handle_t* get_handle(uint8_t handle)
{
    if (handle < 1 || handle > XFS_MAX_FDS)
        return NULL;
    return &xfs_handles[handle - 1];
}

static inline uint8_t allocate_handle(enum xfs_handle_type_t type)
{
    for (uint8_t i = 0; i < XFS_MAX_FDS; i++)
    {
        if (xfs_handles[i].type == XFS_HANDLE_TYPE_NONE)
        {
            xfs_handles[i].type = type;
            return i + 1;
        }
    }
    return 0;
}

static inline void free_handle(const uint8_t handle, const uint8_t mount_point)
{
    struct xfs_handle_t* h = get_handle(handle);
    if (h)
    {
        const struct xfs_engine_t* current_engine = xfs_mounted_engines[mount_point].engine;
        if (current_engine && current_engine->free_handle)
        {
            current_engine->free_handle(&xfs_mounted_engines[mount_point], h);
        }
        h->type = XFS_HANDLE_TYPE_NONE;
        h->data = NULL;
    }
}

static inline bool ensure_mounted(const uint8_t mount_point)
{
    return (xfs_mounted_engines[mount_point].engine != NULL);
}

/**
 * Handle XFS mount command
 */
void xfs_handle_mount(struct xfs_registers_t* registers)
{
    const char* protocol = (const char*)registers->arguments.mount.protocol;
    const char* hostname = (const char*)registers->arguments.mount.hostname;
    const char* path = (const char*)registers->arguments.mount.path;
    const int mount_point = registers->mount_point;

    if (mount_point < 0 || mount_point >= 4)
    {
        XFS_DEBUG("xfs: mount failed: invalid mount_point=%d\n", mount_point);
        registers->result = XFS_ERR_INVAL;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    if (xfs_mounted_engines[mount_point].engine != NULL)
    {
        XFS_DEBUG("xfs: mount failed: mount_point=%d already mounted\n", mount_point);
        registers->result = XFS_ERR_EXIST;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    if (!protocol || !hostname || !path)
    {
        XFS_DEBUG("xfs: mount failed: null pointer\n");
        registers->result = XFS_ERR_INVAL;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    struct xfs_engine_t* engine = NULL;

    if (strcmp(protocol, "xfs") == 0)
    {
        XFS_DEBUG("xfs: mount hostname='%s' path='%s' mount_point=%d\n", hostname, path, mount_point);

        if (strcmp(hostname, "ram") == 0)
        {
            engine = &xfs_ram_engine;
        }
        else
        {
            XFS_DEBUG("xfs: mount failed: invalid hostname\n");
            registers->result = XFS_ERR_INVAL;
            registers->status = XFS_STATUS_ERROR;
            return;
        }
    }
    else if (strcmp(protocol, "https") == 0)
    {
        XFS_DEBUG("xfs: mount https hostname='%s' path='%s' mount_point=%d\n", hostname, path, mount_point);
        engine = &https_engine;
    }
    else
    {
        XFS_DEBUG("xfs: mount failed: unknown protocol '%s' mount_point=%d\n", protocol, mount_point);
        registers->result = XFS_ERR_INVAL;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    const int16_t mount_result = engine->mount(engine, hostname, path, &xfs_mounted_engines[mount_point]);
    if (mount_result != XFS_ERR_OK)
    {
        XFS_DEBUG("xfs: mount failed: result=%d mount_point=%d\n", mount_result, mount_point);
        registers->result = mount_result;
        registers->status = XFS_STATUS_ERROR;
    }
    else
    {
        xfs_mounted_engines[mount_point].engine = engine;
        XFS_DEBUG("xfs: mount success mount_point=%d\n", mount_point);
        registers->result = 0;
        registers->status = XFS_STATUS_COMPLETE;
    }
}

void xfs_handle_open(struct xfs_registers_t* registers)
{
    const char* path = (const char*)registers->arguments.open.path;
    const uint16_t flags = registers->arguments.open.flags;
    const uint16_t mode = registers->arguments.open.mode;
    const uint8_t mount_point = registers->mount_point;
    XFS_DEBUG("xfs: open path=%s flags=0x%04x mode=0x%04x mount_point=%d\n", path, flags, mode, mount_point);
    
    if (!ensure_mounted(mount_point))
    {
        XFS_DEBUG("xfs: open failed: not mounted mount_point=%d\n", mount_point);
        registers->result = XFS_ERR_IO;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    const uint8_t handle = allocate_handle(XFS_HANDLE_TYPE_FILE);
    if (handle == 0)
    {
        XFS_DEBUG("xfs: open failed: too many open files\n");
        registers->result = XFS_ERR_NOMEM;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    int xfs_flags = 0;
    const uint8_t accmode = flags & 0x0003;
    if (accmode == 0x0001)
        xfs_flags = XFS_O_RDONLY;
    else if (accmode == 0x0002)
        xfs_flags = XFS_O_WRONLY;
    else if (accmode == 0x0003)
        xfs_flags = XFS_O_RDWR;
    else
        xfs_flags = XFS_O_RDONLY;

    if (flags & 0x0008)
        xfs_flags |= XFS_O_APPEND;
    if (flags & 0x0100)
        xfs_flags |= XFS_O_CREAT;
    if (flags & 0x0200)
        xfs_flags |= XFS_O_CREAT | XFS_O_TRUNC;

    struct xfs_handle_t* h = get_handle(handle);
    const struct xfs_engine_mount_t* mounted_engine = &xfs_mounted_engines[mount_point];
    
    const int16_t err = mounted_engine->engine->open(mounted_engine, h, path, xfs_flags);

    if (err)
    {
        XFS_DEBUG("xfs: open failed: result=%d\n", err);
        free_handle(handle, mount_point);
        registers->result = err;
        registers->status = XFS_STATUS_ERROR;
    }
    else
    {
        XFS_DEBUG("xfs: open success handle=%d\n", handle);
        registers->file_handle = handle;
        registers->result = 0;
        registers->status = XFS_STATUS_COMPLETE;
    }
}
void xfs_handle_read(struct xfs_registers_t* registers)
{
    const uint8_t handle = registers->file_handle;
    const uint16_t size = registers->arguments.read.size;
    const uint8_t mount_point = registers->mount_point;
    const struct xfs_engine_mount_t* mounted_engine = &xfs_mounted_engines[mount_point];
    XFS_DEBUG("xfs: read handle=%d size=%d remaining=%d total=%d\n", handle, size,
        registers->fops.remaining, registers->fops.total);
    
    struct xfs_handle_t* h = get_handle(handle);
    if (h == NULL || h->type != XFS_HANDLE_TYPE_FILE)
    {
        XFS_DEBUG("xfs: read failed: invalid handle\n");
        registers->result = XFS_ERR_BADF;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    const int16_t bytes_read = mounted_engine->engine->read(mounted_engine, h,
        (uint8_t*)registers->workspace, size);
    
    if (bytes_read < 0)
    {
        XFS_DEBUG("xfs: read failed: result=%d\n", (int)bytes_read);
        registers->result = bytes_read;
        registers->status = XFS_STATUS_ERROR;
    }
    else
    {
        XFS_DEBUG("xfs: read success bytes=%d\n", (int)bytes_read);
        registers->result = bytes_read;
        registers->status = XFS_STATUS_COMPLETE;
    }
}

void xfs_handle_write(struct xfs_registers_t* registers)
{
    const uint8_t handle = registers->file_handle;
    const uint16_t size = registers->arguments.write.size;
    const uint8_t mount_point = registers->mount_point;
    const struct xfs_engine_mount_t* mounted_engine = &xfs_mounted_engines[mount_point];

    XFS_DEBUG("xfs: write handle=%d size=%d remaining=%d total=%d\n", handle, size,
        registers->fops.remaining, registers->fops.total);

    struct xfs_handle_t* h = get_handle(handle);
    if (h == NULL || h->type != XFS_HANDLE_TYPE_FILE)
    {
        XFS_DEBUG("xfs: write failed: invalid handle\n");
        registers->result = XFS_ERR_BADF;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    const int16_t bytes_written = mounted_engine->engine->write(mounted_engine, h,
        (uint8_t*)registers->workspace, size);
    
    if (bytes_written < 0)
    {
        XFS_DEBUG("xfs: write failed: result=%d\n", (int)bytes_written);
        registers->result = bytes_written;
        registers->status = XFS_STATUS_ERROR;
    }
    else
    {
        XFS_DEBUG("xfs: write success bytes=%d\n", (int)bytes_written);
        registers->result = bytes_written;
        registers->status = XFS_STATUS_COMPLETE;
    }
}

void xfs_handle_close(struct xfs_registers_t* registers)
{
    const uint8_t handle = registers->file_handle;
    const uint8_t mount_point = registers->mount_point;
    const struct xfs_engine_mount_t* mounted_engine = &xfs_mounted_engines[mount_point];
    XFS_DEBUG("xfs: close handle=%d mount_point=%d\n", handle, mount_point);
    struct xfs_handle_t* h = get_handle(handle);
    
    if (h == NULL)
    {
        XFS_DEBUG("xfs: close failed: invalid handle\n");
        registers->result = XFS_ERR_BADF;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    int16_t err = XFS_ERR_OK;
    if (h->type == XFS_HANDLE_TYPE_FILE)
    {
        err = mounted_engine->engine->close(mounted_engine, h);
    }
    else if (h->type == XFS_HANDLE_TYPE_DIR)
    {
        err = mounted_engine->engine->closedir(mounted_engine, h);
    }

    free_handle(handle, mount_point);
    
    if (err)
    {
        XFS_DEBUG("xfs: close failed: result=%d\n", err);
        registers->result = err;
        registers->status = XFS_STATUS_ERROR;
    }
    else
    {
        XFS_DEBUG("xfs: close success\n");
        registers->result = 0;
        registers->status = XFS_STATUS_COMPLETE;
    }
}

void xfs_handle_opendir(struct xfs_registers_t* registers)
{
    const char* path = (const char*)registers->arguments.opendir.path;
    const uint8_t mount_point = registers->mount_point;
    const struct xfs_engine_mount_t* mounted_engine = &xfs_mounted_engines[mount_point];
    XFS_DEBUG("xfs: opendir path=%s mount_point=%d\n", path, mount_point);
    
    if (!ensure_mounted(mount_point))
    {
        XFS_DEBUG("xfs: opendir failed: not mounted mount_point=%d\n", mount_point);
        registers->result = XFS_ERR_IO;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    const uint8_t handle = allocate_handle(XFS_HANDLE_TYPE_DIR);
    if (handle == 0)
    {
        XFS_DEBUG("xfs: opendir failed: too many open files mount_point=%d\n", mount_point);
        registers->result = XFS_ERR_NOMEM;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    struct xfs_handle_t* h = get_handle(handle);
    const int16_t err = mounted_engine->engine->opendir(mounted_engine, h, path);

    if (err)
    {
        XFS_DEBUG("xfs: opendir %s failed: result=%d mount_point=%d\n", path, err, mount_point);
        free_handle(handle, mount_point);
        registers->result = err;
        registers->status = XFS_STATUS_ERROR;
    }
    else
    {
        XFS_DEBUG("xfs: opendir %s success handle=%d mount_point=%d\n", path, handle, mount_point);
        registers->result = 0;
        registers->file_handle = handle;
        registers->status = XFS_STATUS_COMPLETE;
    }
}

void xfs_handle_readdir(struct xfs_registers_t* registers)
{
    const uint8_t handle = registers->file_handle;
    const uint8_t mount_point = registers->mount_point;
    const struct xfs_engine_mount_t* mounted_engine = &xfs_mounted_engines[mount_point];
    XFS_DEBUG("xfs: readdir handle=%d\n", handle);
    struct xfs_handle_t* h = get_handle(handle);
    
    if (h == NULL || h->type != XFS_HANDLE_TYPE_DIR)
    {
        XFS_DEBUG("xfs: readdir failed: invalid handle\n");
        registers->result = XFS_ERR_BADF;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    struct xfs_stat_info info;
    const int16_t err = mounted_engine->engine->readdir(mounted_engine, h, &info);
    
    if (err < 0)
    {
        XFS_DEBUG("xfs: readdir failed: result=%d\n", err);
        registers->result = err;
        registers->status = XFS_STATUS_ERROR;
    }
    else
    {
        if (err == 0)
        {
            XFS_DEBUG("xfs: readdir eod\n");
            registers->result = 1;
        }
        else
        {
            XFS_DEBUG("xfs: readdir success name=%s\n", info.name);
            strcpy((char*)registers->workspace, info.name);
            registers->result = 0;
        }
        registers->status = XFS_STATUS_COMPLETE;
    }
}

void xfs_handle_closedir(struct xfs_registers_t* registers)
{
    uint8_t handle = registers->file_handle;
    const uint8_t mount_point = registers->mount_point;
    const struct xfs_engine_mount_t* mounted_engine = &xfs_mounted_engines[mount_point];
    XFS_DEBUG("xfs: closedir handle=%d\n", handle);
    struct xfs_handle_t* h = get_handle(handle);
    
    if (h == NULL || h->type != XFS_HANDLE_TYPE_DIR)
    {
        XFS_DEBUG("xfs: closedir failed: invalid handle\n");
        registers->result = XFS_ERR_BADF;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    const int16_t err = mounted_engine->engine->closedir(mounted_engine, h);
    free_handle(handle, mount_point);
    
    if (err)
    {
        XFS_DEBUG("xfs: closedir failed: result=%d\n", err);
        registers->result = err;
        registers->status = XFS_STATUS_ERROR;
    }
    else
    {
        XFS_DEBUG("xfs: closedir success\n");
        registers->result = 0;
        registers->status = XFS_STATUS_COMPLETE;
    }
}

void xfs_handle_stat(struct xfs_registers_t* registers)
{
    const char* path = (const char*)registers->arguments.stat.path;
    const uint8_t mount_point = registers->mount_point;
    const struct xfs_engine_mount_t* mounted_engine = &xfs_mounted_engines[mount_point];
    XFS_DEBUG("xfs: stat path=%s\n", path);
    
    if (!ensure_mounted(mount_point))
    {
        XFS_DEBUG("xfs: stat failed: not mounted\n");
        registers->result = XFS_ERR_IO;
        registers->status = XFS_STATUS_ERROR;
        return;
    }
    
    struct xfs_stat_info info;
    const int16_t err = mounted_engine->engine->stat(mounted_engine, path, &info);
    
    if (err)
    {
        XFS_DEBUG("xfs: stat failed: result=%d\n", err);
        registers->result = err;
        registers->status = XFS_STATUS_ERROR;
    }
    else
    {
        struct xfs_stat_t* stat = (struct xfs_stat_t*)registers->workspace;
        stat->mode = 0x0644;
        if (info.type == XFS_TYPE_DIR)
        {
            stat->mode |= 0x4000;
        }
        else
        {
            stat->mode |= 0x8000;
        }
        stat->uid = 0;
        stat->gid = 0;
        stat->size = (uint32_t)info.size;
        stat->atime = 0;
        stat->mtime = 0;
        stat->ctime = 0;
        uint8_t* strings = (uint8_t*)registers->workspace + 22;
        strings[0] = 0;
        strings[1] = 0;
        XFS_DEBUG("xfs: stat success size=%lu mode=0x%04x\n", info.size, stat->mode);
        registers->result = 0;
        registers->status = XFS_STATUS_COMPLETE;
    }
}

void xfs_handle_unlink(struct xfs_registers_t* registers)
{
    const char* path = (const char*)registers->arguments.unlink.path;
    const uint8_t mount_point = registers->mount_point;
    const struct xfs_engine_mount_t* mounted_engine = &xfs_mounted_engines[mount_point];
    XFS_DEBUG("xfs: unlink path=%s\n", path);
    
    if (!ensure_mounted(mount_point))
    {
        XFS_DEBUG("xfs: unlink failed: not mounted\n");
        registers->result = XFS_ERR_IO;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    const int16_t err = mounted_engine->engine->unlink(mounted_engine, path);
    
    if (err)
    {
        XFS_DEBUG("xfs: unlink failed: result=%d\n", err);
        registers->result = err;
        registers->status = XFS_STATUS_ERROR;
    }
    else
    {
        XFS_DEBUG("xfs: unlink success\n");
        registers->result = 0;
        registers->status = XFS_STATUS_COMPLETE;
    }
}

void xfs_handle_mkdir(struct xfs_registers_t* registers)
{
    const char* path = (const char*)registers->arguments.mkdir.path;
    const uint8_t mount_point = registers->mount_point;
    const struct xfs_engine_mount_t* mounted_engine = &xfs_mounted_engines[mount_point];
    XFS_DEBUG("xfs: mkdir path=%s\n", path);
    
    if (!ensure_mounted(mount_point))
    {
        XFS_DEBUG("xfs: mkdir failed: not mounted\n");
        registers->result = XFS_ERR_IO;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    const int16_t err = mounted_engine->engine->mkdir(mounted_engine, path);
    
    if (err)
    {
        XFS_DEBUG("xfs: mkdir failed: result=%d\n", err);
        registers->result = err;
        registers->status = XFS_STATUS_ERROR;
    }
    else
    {
        XFS_DEBUG("xfs: mkdir success\n");
        registers->result = 0;
        registers->status = XFS_STATUS_COMPLETE;
    }
}

void xfs_handle_rmdir(struct xfs_registers_t* registers)
{
    const char* path = (const char*)registers->arguments.rmdir.path;
    const uint8_t mount_point = registers->mount_point;
    const struct xfs_engine_mount_t* mounted_engine = &xfs_mounted_engines[mount_point];
    XFS_DEBUG("xfs: rmdir path=%s\n", path);
    
    if (!ensure_mounted(mount_point))
    {
        XFS_DEBUG("xfs: rmdir failed: not mounted\n");
        registers->result = XFS_ERR_IO;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    const int16_t err = mounted_engine->engine->rmdir(mounted_engine, path);
    
    if (err)
    {
        XFS_DEBUG("xfs: rmdir failed: result=%d\n", err);
        registers->result = err;
        registers->status = XFS_STATUS_ERROR;
    }
    else
    {
        XFS_DEBUG("xfs: rmdir success\n");
        registers->result = 0;
        registers->status = XFS_STATUS_COMPLETE;
    }
}

void xfs_handle_chdir(struct xfs_registers_t* registers)
{
    const char* path = (const char*)registers->arguments.chdir.path;
    const uint8_t mount_point = registers->mount_point;
    const struct xfs_engine_mount_t* mounted_engine = &xfs_mounted_engines[mount_point];
    XFS_DEBUG("xfs: chdir path=%s (not supported by littlefs)\n", path);
    
    if (!ensure_mounted(mount_point))
    {
        XFS_DEBUG("xfs: chdir failed: not mounted\n");
        registers->result = XFS_ERR_IO;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    struct xfs_stat_info info;
    const int err = mounted_engine->engine->stat(mounted_engine, path, &info);
    
    if (err || info.type != XFS_TYPE_DIR)
    {
        XFS_DEBUG("xfs: chdir failed: path not found or not a directory\n");
        registers->result = XFS_ERR_NOENT;
        registers->status = XFS_STATUS_ERROR;
    }
    else
    {
        XFS_DEBUG("xfs: chdir success (path verified)\n");
        registers->result = 0;
        registers->status = XFS_STATUS_COMPLETE;
    }
}

void xfs_handle_getcwd(struct xfs_registers_t* registers)
{
    const uint8_t mount_point = registers->mount_point;
    const struct xfs_engine_mount_t* mounted_engine = &xfs_mounted_engines[mount_point];

    XFS_DEBUG("xfs: getcwd (littlefs always returns root)\n");
    if (!ensure_mounted(mount_point))
    {
        XFS_DEBUG("xfs: getcwd failed: not mounted\n");
        registers->result = XFS_ERR_IO;
        registers->status = XFS_STATUS_ERROR;
        return;
    }
    
    char* path = (char*)registers->workspace;
    uint16_t len = registers->arguments.getcwd.buffer_size;
    
    int16_t err = mounted_engine->engine->getcwd(mounted_engine, path, len);
    if (err)
    {
        registers->result = err;
        registers->status = XFS_STATUS_ERROR;
        return;
    }
    
    XFS_DEBUG("xfs: getcwd success path=%s\n", path);
    registers->result = 0;
    registers->status = XFS_STATUS_COMPLETE;
}

void xfs_handle_rename(struct xfs_registers_t* registers)
{
    const char* old_path = (const char*)registers->arguments.rename.old_path;
    const char* new_path = (const char*)registers->arguments.rename.new_path;
    const uint8_t mount_point = registers->mount_point;
    const struct xfs_engine_mount_t* mounted_engine = &xfs_mounted_engines[mount_point];

    XFS_DEBUG("xfs: rename old=%s new=%s\n", old_path, new_path);
    
    if (!ensure_mounted(mount_point))
    {
        XFS_DEBUG("xfs: rename failed: not mounted\n");
        registers->result = XFS_ERR_IO;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    const int16_t err = mounted_engine->engine->rename(mounted_engine, old_path, new_path);
    
    if (err)
    {
        XFS_DEBUG("xfs: rename failed: result=%d\n", err);
        registers->result = err;
        registers->status = XFS_STATUS_ERROR;
    }
    else
    {
        XFS_DEBUG("xfs: rename success\n");
        registers->result = 0;
        registers->status = XFS_STATUS_COMPLETE;
    }
}

void xfs_handle_lseek(struct xfs_registers_t* registers)
{
    uint8_t handle = registers->file_handle;
    uint32_t offset = registers->arguments.lseek.offset;
    uint8_t whence = registers->arguments.lseek.whence;
    const uint8_t mount_point = registers->mount_point;
    const struct xfs_engine_mount_t* mounted_engine = &xfs_mounted_engines[mount_point];

    XFS_DEBUG("xfs: lseek handle=%d offset=%lu whence=%d\n", handle, (unsigned long)offset, whence);
    
    struct xfs_handle_t* h = get_handle(handle);
    if (h == NULL || h->type != XFS_HANDLE_TYPE_FILE)
    {
        XFS_DEBUG("xfs: lseek failed: invalid handle\n");
        registers->result = XFS_ERR_BADF;
        registers->status = XFS_STATUS_ERROR;
        return;
    }

    int16_t new_pos = mounted_engine->engine->lseek(mounted_engine, h, offset, whence);

    if (new_pos < 0)
    {
        XFS_DEBUG("xfs: lseek failed: result=%ld\n", (long)new_pos);
        registers->result = new_pos;
        registers->status = XFS_STATUS_ERROR;
    }
    else
    {
        XFS_DEBUG("xfs: lseek success new_pos=%lu\n", (unsigned long)new_pos);
        *(uint32_t*)&registers->workspace[0] = (uint32_t)new_pos;
        registers->result = 0;
        registers->status = XFS_STATUS_COMPLETE;
    }
}

/**
 * Handle XFS command dispatch
 * This function processes a command from the registers and dispatches to the appropriate handler
 * FreeRTOS-independent, usable in emulator
 */
void xfs_handle_command(struct xfs_registers_t* registers)
{
    const uint8_t command = registers->command;

    if (command)
    {
        registers->command = 0;
        registers->result = 0;
        registers->status = XFS_STATUS_BUSY;

        switch (command)
        {
            case XFS_CMD_MOUNT:
            {
                xfs_handle_mount(registers);
                break;
            }
            case XFS_CMD_OPEN:
            {
                xfs_handle_open(registers);
                break;
            }
            case XFS_CMD_READ:
            {
                xfs_handle_read(registers);
                break;
            }
            case XFS_CMD_WRITE:
            {
                xfs_handle_write(registers);
                break;
            }
            case XFS_CMD_CLOSE:
            {
                xfs_handle_close(registers);
                break;
            }
            case XFS_CMD_OPENDIR:
            {
                xfs_handle_opendir(registers);
                break;
            }
            case XFS_CMD_READDIR:
            {
                xfs_handle_readdir(registers);
                break;
            }
            case XFS_CMD_CLOSEDIR:
            {
                xfs_handle_closedir(registers);
                break;
            }
            case XFS_CMD_STAT:
            {
                xfs_handle_stat(registers);
                break;
            }
            case XFS_CMD_UNLINK:
            {
                xfs_handle_unlink(registers);
                break;
            }
            case XFS_CMD_MKDIR:
            {
                xfs_handle_mkdir(registers);
                break;
            }
            case XFS_CMD_RMDIR:
            {
                xfs_handle_rmdir(registers);
                break;
            }
            case XFS_CMD_CHDIR:
            {
                xfs_handle_chdir(registers);
                break;
            }
            case XFS_CMD_GETCWD:
            {
                xfs_handle_getcwd(registers);
                break;
            }
            case XFS_CMD_RENAME:
            {
                xfs_handle_rename(registers);
                break;
            }
            case XFS_CMD_LSEEK:
            {
                xfs_handle_lseek(registers);
                break;
            }
            default:
            {
                XFS_DEBUG("xfs: unknown command=%d\n", command);
                registers->result = XFS_ERR_INVAL;
                registers->status = XFS_STATUS_ERROR;
                break;
            }
        }
    }
}

/**
 * Free all XFS resources (handles and mounts)
 * This function is FreeRTOS-independent and can be used in an emulator
 */
void xfs_free(void)
{
    XFS_DEBUG("xfs: free - cleaning up all mounts and handles\n");
    
    // First, free all open handles before unmounting
    // We need to do this before unmounting because free_handle needs the engine
    for (uint8_t i = 0; i < XFS_MAX_FDS; i++)
    {
        if (xfs_handles[i].type != XFS_HANDLE_TYPE_NONE)
        {
            XFS_DEBUG("xfs: free - freeing handle %d (type=%d)\n", i + 1, xfs_handles[i].type);
            
            struct xfs_handle_t* h = &xfs_handles[i];
            
            // Try to free engine-specific resources
            // Since we don't know the mount_point, we'll try each mounted engine
            for (uint8_t mp = 0; mp < 4; mp++)
            {
                if (xfs_mounted_engines[mp].engine != NULL && xfs_mounted_engines[mp].engine->free_handle)
                {
                    xfs_mounted_engines[mp].engine->free_handle(&xfs_mounted_engines[mp], h);
                    break; // Only need to call once per handle
                }
            }
            
            // Clear handle state
            h->type = XFS_HANDLE_TYPE_NONE;
            h->data = NULL;
        }
    }
    
    // Now unmount all engines to clean up engine-specific mount resources
    for (uint8_t mount_point = 0; mount_point < 4; mount_point++)
    {
        if (xfs_mounted_engines[mount_point].engine != NULL)
        {
            XFS_DEBUG("xfs: free - unmounting engine at mount_point=%d\n", mount_point);
            
            // Call engine's unmount function to clean up engine-specific resources
            if (xfs_mounted_engines[mount_point].engine->unmount)
            {
                xfs_mounted_engines[mount_point].engine->unmount(
                    xfs_mounted_engines[mount_point].engine, &xfs_mounted_engines[mount_point]);
            }
            
            xfs_mounted_engines[mount_point].engine = NULL;
        }
    }
    
    // Clear xfs_handles array (should already be cleared, but be explicit)
    memset(&xfs_handles[0], 0, sizeof(xfs_handles));
}
