#ifndef _WIFI_H_
#define _WIFI_H_

#include <stdint.h>

#define SPECTRANEXT_CONFIG_PAGE (0x48)

#define SPECTRANEXT_COMMAND_SCAN_ACCESS_POINTS (1)
#define SPECTRANEXT_COMMAND_CONNECT_ACCESS_POINT (2)
#define SPECTRANEXT_COMMAND_DISCONNECT (3)

#define WIFI_SCAN_NONE (0)
#define WIFI_SCAN_SCANNING (1)
#define WIFI_SCAN_COMPLETE (2)
#define WIFI_SCAN_FAILURE (-1)

#define WIFI_CONNECT_DISCONNECTED (0)
#define WIFI_CONNECT_CONNECTING (1)
#define WIFI_CONNECT_CONNECT_SUCCESS (2)
#define WIFI_CONNECT_CONNECT_IP_OBTAINED (3)
#define WIFI_CONNECT_CONNECT_FAILURE (-1)

#define SPECTRANEXT_CONTROLLER_STATUS_OFFLINE (0)
#define SPECTRANEXT_CONTROLLER_STATUS_BUSY_UPDATING (1)
#define SPECTRANEXT_CONTROLLER_STATUS_OPERATIONAL (2)

#pragma pack(push, 1)

struct wifi_config_registers_t
{
    // command registers - see WIFI_COMMAND_*
    uint8_t command;
    // scan result feedback - see WIFI_SCAN_*
    int8_t scan_status;
    // connect result feedback - see WIFI_CONNECT_*
    int8_t connection_status;
    // amount of found access points for SPECTRANEXT_COMMAND_SCAN_ACCESS_POINTS
    uint8_t scan_access_point_count;

    // SSID for WIFI_COMMAND_CONNECT_ACCESS_POINT
    char access_point_name[64];
    // PASSWORD for WIFI_COMMAND_CONNECT_ACCESS_POINT
    char access_point_password[64];

    // controller status - see WIFI_CONTROLLER_STATUS_*
    uint8_t controller_status;
    // reserved
    char reserved[123];

    // list of found access points
    struct
    {
        char name[64];
    } access_points[32];
};

#pragma pack(pop)

#endif
