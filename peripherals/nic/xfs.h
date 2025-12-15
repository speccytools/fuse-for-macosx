#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "libspectrum.h"
#include "memory_pages.h"

#define XFS_SPECTRANET_PAGE (0x49)

#define XFS_CMD_MOUNT (1)
#define XFS_CMD_OPEN (2)
#define XFS_CMD_READ (3)
#define XFS_CMD_WRITE (4)
#define XFS_CMD_CLOSE (5)
#define XFS_CMD_OPENDIR (6)
#define XFS_CMD_READDIR (7)
#define XFS_CMD_CLOSEDIR (8)
#define XFS_CMD_STAT (9)
#define XFS_CMD_UNLINK (10)
#define XFS_CMD_MKDIR (11)
#define XFS_CMD_RMDIR (12)
#define XFS_CMD_CHDIR (13)
#define XFS_CMD_GETCWD (14)
#define XFS_CMD_RENAME (15)
#define XFS_CMD_LSEEK (16)

#define XFS_STATUS_IDLE (0)
#define XFS_STATUS_BUSY (1)
#define XFS_STATUS_COMPLETE (2)
#define XFS_STATUS_ERROR (3)

#define XFS_MAX_FDS (4)

#pragma pack(push, 1)

// Argument structures for each command type
struct xfs_args_mount_t
{
    char hostname[256]; // e.g., "ram"
};

struct xfs_args_open_t
{
    char path[128];
    uint16_t flags;  // POSIX flags (O_RDONLY, O_WRONLY, O_RDWR, etc.)
    uint16_t mode;   // POSIX mode (file permissions)
};

struct xfs_args_read_t
{
    uint16_t size;
    uint8_t reserved[254];
};

struct xfs_args_write_t
{
    uint16_t size;
    uint8_t reserved[254];
};

struct xfs_args_opendir_t
{
    char path[256];
};

struct xfs_args_stat_t
{
    char path[256];
};

struct xfs_args_unlink_t
{
    char path[256];
};

struct xfs_args_mkdir_t
{
    char path[256];
};

struct xfs_args_rmdir_t
{
    char path[256];
};

struct xfs_args_chdir_t
{
    char path[256];
};

struct xfs_args_getcwd_t
{
    uint16_t buffer_size;
    uint8_t reserved[254];
};

struct xfs_args_rename_t
{
    char old_path[128];
    char new_path[128];
};

struct xfs_args_lseek_t
{
    uint32_t offset; // FSIZE_t is uint32_t
    uint8_t whence;
    uint8_t reserved[251];
};

// Spectranet stat structure (matches stat.inc format)
struct xfs_stat_t
{
    uint16_t mode;   // File mode (offset 0x00)
    uint16_t uid;    // User ID (offset 0x02)
    uint16_t gid;    // Group ID (offset 0x04)
    uint32_t size;   // File size (offset 0x06)
    uint32_t atime;  // Access time (offset 0x0A)
    uint32_t mtime;  // Modification time (offset 0x0E)
    uint32_t ctime;  // Change time (offset 0x12)
    // Strings follow at offset 0x16 (uidstring, gidstring, null-terminated)
};

// Union of all argument types
union xfs_arguments_t
{
    uint8_t raw[256];
    struct xfs_args_mount_t mount;
    struct xfs_args_open_t open;
    struct xfs_args_read_t read;
    struct xfs_args_write_t write;
    struct xfs_args_opendir_t opendir;
    struct xfs_args_stat_t stat;
    struct xfs_args_unlink_t unlink;
    struct xfs_args_mkdir_t mkdir;
    struct xfs_args_rmdir_t rmdir;
    struct xfs_args_chdir_t chdir;
    struct xfs_args_getcwd_t getcwd;
    struct xfs_args_rename_t rename;
    struct xfs_args_lseek_t lseek;
};

struct xfs_registers_t
{
    // Command register - see XFS_CMD_*
    uint8_t command;
    // Status register - see XFS_STATUS_*
    uint8_t status;
    // Result code (error code or success indicator)
    int16_t result;
    // File handle for operations requiring handle
    uint8_t file_handle;
    // Saved page A (stored by F_fetchpage, restored by F_restorepage)
    uint8_t saved_page_a;
    // Reserved for alignment
    uint8_t reserved[2];
    // Arguments section (256 bytes) - use union to access typed structs
    union xfs_arguments_t arguments;
    // 0x1108: temporary space - used for process variables
    union
    {
        uint8_t tmp[248];

        struct
        {
            uint16_t remaining;
            uint16_t total;
        } fops;
    };
    // 0x1200: Workspace section (1024+ bytes for data transfer) - shifted by 8 bytes
    uint8_t workspace[1024];

    // 0x1600
    uint8_t module_space[2560];
};

#pragma pack(pop)

_Static_assert(4096 == sizeof(struct xfs_registers_t), "xfs_registers_t is not 4096");
_Static_assert(0x200 == offsetof(struct xfs_registers_t, workspace), "workspace is not at 0x200");
_Static_assert(0x008 == offsetof(struct xfs_registers_t, arguments), "arguments is not at 0x008");

extern volatile struct xfs_registers_t xfs_registers;

extern void xfs_init();

// XFS register access functions
libspectrum_byte xfs_read( memory_page *page, libspectrum_word address );
void xfs_write( memory_page *page, libspectrum_word address, libspectrum_byte b );
