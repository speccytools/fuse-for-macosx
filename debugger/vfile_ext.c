#include "config.h"

#include "vfile_ext.h"

#include "peripherals/fs/xfs.h"
#include "peripherals/fs/xfs_engines.h"
#include "peripherals/fs/xfs_worker.h"
#include "packets.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Hex encoding/decoding utilities (required by vfile.c)
static uint8_t hex_char_to_val(char c)
{
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    return 0; // fallback, input assumed valid
}

void bin_to_hex(const uint8_t* data, uint16_t len, char* result)
{
    static const char hex_digits[] = "0123456789ABCDEF";

    // Check if encoding in-place (data and result point to same memory)
    if (data == (const uint8_t*)result)
    {
        // Encode backwards to avoid overwriting unread binary data
        // We read from position i and write to positions i*2 and i*2+1
        // Since i*2 >= i, we need to go backwards
        for (uint16_t i = len; i > 0; i--)
        {
            uint16_t idx = i - 1;
            uint8_t byte = data[idx];
            result[idx * 2]     = hex_digits[(byte >> 4) & 0x0F];
            result[idx * 2 + 1] = hex_digits[byte & 0x0F];
        }
    }
    else
    {
        // Normal forward encoding when data and result are different buffers
        for (uint16_t i = 0; i < len; i++)
        {
            result[i * 2]     = hex_digits[(data[i] >> 4) & 0x0F];
            result[i * 2 + 1] = hex_digits[data[i] & 0x0F];
        }
    }
    result[len * 2] = '\0'; // null terminate
}

void hex_to_bin(const char* hex, uint16_t data_len, uint8_t* result)
{
    for (uint16_t i = 0; i < data_len; i++)
    {
        const uint8_t hi = hex_char_to_val(hex[i * 2]);
        const uint8_t lo = hex_char_to_val(hex[i * 2 + 1]);
        result[i] = (hi << 4) | lo;
    }
}

// External buffer from gdbserver.c
extern uint8_t tmpbuf[0x20000];

// Buffer management
char* vfile_ext_get_response_buf(void)
{
    return (char*)tmpbuf;
}

size_t vfile_ext_get_response_buf_size(void)
{
    return sizeof(tmpbuf);
}

// Message sending (for debugger responses)
void vfile_ext_send_message(const uint8_t* data, size_t len)
{
    // Use write_packet_bytes for binary data
    // Note: The packet will be flushed automatically by the network thread
    // when write_flush is called during normal packet processing
    extern void write_packet_bytes(const uint8_t *data, size_t num_bytes);
    write_packet_bytes(data, len);
    // Note: We don't flush here because gdbserver_client_socket is static
    // The packet will be flushed when the next packet is processed or when
    // the network thread calls write_flush
}

// Delay (platform-specific sleep)
void vfile_ext_delay_ms(uint32_t milliseconds)
{
    usleep(milliseconds * 1000);
}

// System reset - not supported in Fuse (emulator)
void vfile_ext_trigger_reset(void)
{
    // In Fuse, we can't actually reset the emulator from here
    // This would need to be handled differently
}

// Configuration management - not implemented in Fuse
int vfile_ext_config_set_string(uint8_t section, uint8_t item, const char* value)
{
    (void)section;
    (void)item;
    (void)value;
    // Configuration not implemented in Fuse
    return -1;
}

int vfile_ext_config_set_byte(uint8_t section, uint8_t item, uint8_t value)
{
    (void)section;
    (void)item;
    (void)value;
    // Configuration not implemented in Fuse
    return -1;
}

// Reboot monitor - not implemented in Fuse
void vfile_ext_reboot_monitor_enable(int enable)
{
    (void)enable;
    // Not implemented in Fuse
}

// XFS reset
void vfile_ext_xfs_reset(void)
{
    xfs_reset();
}

// Filesystem mount - mount RAM engine
int vfile_ext_fs_mount_ram(void)
{
    // Mount is handled by ensure_mounted, which calls the engine's mount function
    // The mount function will be called when needed
    return 0;  // Success (mount happens lazily)
}

void* vfile_ext_get_filesystem_ram(void)
{
    // In Fuse, the mount_data is a struct xfs_fs_mount_data_t
    // We need to mount the engine first to get the mount_data
    // This will be handled by ensure_mounted
    return NULL;  // Will be set by ensure_mounted
}

// Ensure filesystem is mounted (platform-specific implementation)
int vfile_ext_ensure_mounted(struct xfs_engine_mount_t* mount)
{
    if (mount->mount_data == NULL)
    {
        // Mount the RAM engine with hostname "ram" and path "/"
        const int16_t mount_result = xfs_ram_engine.mount(&xfs_ram_engine, "ram", "/", mount);
        if (mount_result != XFS_ERR_OK)
        {
            return -1;
        }
    }
    return 0;
}
