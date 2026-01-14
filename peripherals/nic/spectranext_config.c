#include "config.h"

#include "spectranext_config.h"

#include <string.h>

#include "libspectrum.h"
#include "memory_pages.h"

volatile struct spectranext_controller_registers_t spectranext_config = {
    .controller_status = WIFI_CONTROLLER_STATUS_OPERATIONAL,
    .connection_status = WIFI_CONNECT_CONNECT_IP_OBTAINED
};

// Process a single Spectranext command
static void spectranext_config_process_command(void)
{
    const uint8_t command = spectranext_config.command;
    
    if (command == 0)
        return;
    
    spectranext_config.command = 0;
    
    switch (command)
    {
        case SPECTRANEXT_COMMAND_SCAN_ACCESS_POINTS:
        {
            // Return single access point "spectranext"
            spectranext_config.scan_status = WIFI_SCAN_SCANNING;
            spectranext_config.scan_access_point_count = 1;
            strcpy(spectranext_config.access_points[0].name, "spectranext");
            spectranext_config.scan_status = WIFI_SCAN_COMPLETE;
            break;
        }
        case SPECTRANEXT_COMMAND_CONNECT_ACCESS_POINT:
        {
            spectranext_config.connection_status = WIFI_CONNECT_CONNECT_SUCCESS;
            break;
        }
        case SPECTRANEXT_COMMAND_DISCONNECT:
        {
            spectranext_config.connection_status = WIFI_CONNECT_CONNECT_FAILURE;
            break;
        }
        default:
        {
            break;
        }
    }
}

void spectranext_config_init()
{
    // Initialize with default values
    spectranext_config.controller_status = WIFI_CONTROLLER_STATUS_OPERATIONAL;
    spectranext_config.connection_status = WIFI_CONNECT_CONNECT_IP_OBTAINED;
    spectranext_config.scan_status = WIFI_SCAN_NONE;
    spectranext_config.scan_access_point_count = 0;
}

libspectrum_byte spectranext_config_read( memory_page *page, libspectrum_word address )
{
    libspectrum_word offset = address & 0xfff;  // Spectranext config is single page (0x48)
    uint8_t *registers = (uint8_t*)&spectranext_config;
    if (offset >= sizeof(spectranext_config))
        return 0xff;
    return registers[offset];
}

void spectranext_config_write( memory_page *page, libspectrum_word address, libspectrum_byte b )
{
    libspectrum_word offset = address & 0xfff;
    if (offset >= sizeof(spectranext_config))
        return;
    
    uint8_t *registers = (uint8_t*)&spectranext_config;
    uint8_t old_command = registers[0];
    registers[offset] = b;
    
    // If writing to command register (offset 0) and command changed from 0 to non-zero, process command
    if (offset == 0 && old_command == 0 && b != 0)
    {
        spectranext_config_process_command();
    }
}
