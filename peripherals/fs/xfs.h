#pragma once

#include <stdbool.h>
#include <sys/cdefs.h>
#include <stdint.h>

#define XFS_SPECTRANET_PAGE (0x49)

// XFS error codes (mapped from LittleFS error codes)
enum xfs_error {
    XFS_ERR_OK          = 0,    // No error
    XFS_ERR_IO          = -5,   // Error during device operation
    XFS_ERR_CORRUPT     = -84,  // Corrupted
    XFS_ERR_NOENT       = -2,   // No directory entry
    XFS_ERR_EXIST       = -17,  // Entry already exists
    XFS_ERR_NOTDIR      = -20,  // Entry is not a dir
    XFS_ERR_ISDIR       = -21,  // Entry is a dir
    XFS_ERR_NOTEMPTY    = -39,  // Dir is not empty
    XFS_ERR_BADF        = -9,   // Bad file number
    XFS_ERR_FBIG        = -27,  // File too large
    XFS_ERR_INVAL       = -22,  // Invalid parameter
    XFS_ERR_NOSPC       = -28,  // No space left on device
    XFS_ERR_NOMEM       = -12,  // No more memory available
    XFS_ERR_NOATTR      = -61,  // No data/attr available
    XFS_ERR_NAMETOOLONG = -36,  // File name too long
};

// File types
enum xfs_type {
    XFS_TYPE_REG = 0x001,
    XFS_TYPE_DIR = 0x002,
};

// File open flags
enum xfs_open_flags {
    XFS_O_RDONLY = 1,
    XFS_O_WRONLY = 2,
    XFS_O_RDWR   = 3,
    XFS_O_CREAT  = 0x0100,
    XFS_O_EXCL   = 0x0200,
    XFS_O_TRUNC  = 0x0400,
    XFS_O_APPEND = 0x0800,
};

// File seek flags
enum xfs_whence_flags {
    XFS_SEEK_SET = 0,
    XFS_SEEK_CUR = 1,
    XFS_SEEK_END = 2,
};

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

#define XFS_MAX_FDS (16)

struct xfs_handle_t;

struct xfs_engine_mount_t
{
    struct xfs_engine_t* engine;
    void* mount_data;
};

struct xfs_stat_info
{
    uint8_t type;      // XFS_TYPE_REG or XFS_TYPE_DIR
    uint32_t size;     // File size (for files)
    char name[64];    // File/directory name
};

struct xfs_engine_t
{
    void* user; // Engine-specific user data

    // Mount/unmount operations
    int16_t (*mount)(const struct xfs_engine_t* engine, const char* hostname, const char* path, struct xfs_engine_mount_t* out_mount);
    uint8_t (*is_mounted)(const struct xfs_engine_t* engine, struct xfs_engine_mount_t* mount);
    void (*unmount)(const struct xfs_engine_t* engine, struct xfs_engine_mount_t* mount);

    // File operations
    int16_t (*open)(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle, const char* path, int flags);
    int16_t (*read)(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle, void* buffer, uint16_t size);
    int16_t (*write)(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle, const void* buffer, uint16_t size);
    int16_t (*close)(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle);
    int16_t (*lseek)(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle, uint32_t offset, uint8_t whence);

    // Directory operations
    int16_t (*opendir)(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle, const char* path);
    int16_t (*readdir)(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle, struct xfs_stat_info* info);
    int16_t (*closedir)(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle);

    // Path operations
    int16_t (*stat)(const struct xfs_engine_mount_t* engine, const char* path, struct xfs_stat_info* stat_info);
    int16_t (*unlink)(const struct xfs_engine_mount_t* engine, const char* path);
    int16_t (*mkdir)(const struct xfs_engine_mount_t* engine, const char* path);
    int16_t (*rmdir)(const struct xfs_engine_mount_t* engine, const char* path);
    int16_t (*chdir)(const struct xfs_engine_mount_t* engine, const char* path);
    int16_t (*getcwd)(const struct xfs_engine_mount_t* engine, char* buffer, uint16_t size);
    int16_t (*rename)(const struct xfs_engine_mount_t* engine, const char* old_path, const char* new_path);

    // Handle management
    void (*free_handle)(const struct xfs_engine_mount_t* engine, struct xfs_handle_t* handle);
};

enum xfs_handle_type_t
{
    XFS_HANDLE_TYPE_NONE = 0,
    XFS_HANDLE_TYPE_FILE = 1,
    XFS_HANDLE_TYPE_DIR = 2,
};

struct xfs_handle_https_dir_entry_t
{
    uint8_t is_dir;
    char name[64];
    uint32_t size;
};

struct xfs_handle_t
{
    enum xfs_handle_type_t type;
    void* data;
};

#pragma pack(push, 1)

// Argument structures for each command type
struct xfs_args_mount_t
{
    char protocol[32]; // e.g., "xfs"
    char hostname[64]; // e.g., "ram"
    char path[160]; // e.g., "/folder/"
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
    // Mount point (0..3)
    uint8_t mount_point;
    // Reserved for alignment
    uint8_t reserved[1];
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

extern void xfs_init();
extern void xfs_reset(void);

// XFS debug logging control
extern void xfs_debug_enable(bool enable);
extern bool xfs_debug_is_enabled(void);

// Command handlers (FreeRTOS-independent, usable in emulator)
extern void xfs_handle_command(struct xfs_registers_t* registers);
extern void xfs_free(void);
extern void xfs_handle_mount(struct xfs_registers_t* registers);

// Mounted engines array (shared between task and emulator)
extern struct xfs_engine_mount_t xfs_mounted_engines[4];
