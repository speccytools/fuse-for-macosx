#pragma once

#include <stdint.h>
#include <stddef.h>

// Forward declaration
struct xfs_engine_mount_t;

// Platform-specific extension layer for vfile.c
// This allows vfile.c to be moved to different platforms by reimplementing these functions

// Buffer management
// Get pointer to response buffer (platform-specific memory)
char* vfile_ext_get_response_buf(void);
size_t vfile_ext_get_response_buf_size(void);

// Hex encoding/decoding utilities
// These functions are provided by vfile_ext.c:
void bin_to_hex(const uint8_t* data, uint16_t len, char* out);
void hex_to_bin(const char* hex, uint16_t len, uint8_t* out);

// Message sending (for debugger responses)
void vfile_ext_send_message(const uint8_t* data, size_t len);

// Delay (platform-specific sleep)
void vfile_ext_delay_ms(uint32_t milliseconds);

// System reset
void vfile_ext_trigger_reset(void);

// Autoboot configuration (sets mount point and enables auto-boot, then resets)
void vfile_ext_autoboot(void);

// XFS reset
void vfile_ext_xfs_reset(void);

// Filesystem mount
int vfile_ext_fs_mount_ram(void);
void* vfile_ext_get_filesystem_ram(void);

// Ensure filesystem is mounted (platform-specific implementation)
// Takes the mount structure and ensures it's initialized
int vfile_ext_ensure_mounted(struct xfs_engine_mount_t* mount);
