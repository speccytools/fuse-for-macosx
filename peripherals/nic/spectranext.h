#ifndef _SPECTRANEXT_H_
#define _SPECTRANEXT_H_

#include <stdint.h>

// Spectranext controller is mounted onto page 0x48
#define SPECTRANEXT_CONTROLLER_PAGE (0x48)

#define SPECTRANEXT_COMMAND_SCAN_ACCESS_POINTS (1)
#define SPECTRANEXT_COMMAND_CONNECT_ACCESS_POINT (2)
#define SPECTRANEXT_COMMAND_DISCONNECT (3)
#define SPECTRANEXT_COMMAND_GETHOSTBYNAME (4)

#define WIFI_SCAN_NONE (0)
#define WIFI_SCAN_SCANNING (1)
#define WIFI_SCAN_COMPLETE (2)
#define WIFI_SCAN_FAILURE (-1)

#define WIFI_CONNECT_DISCONNECTED (0)
#define WIFI_CONNECT_CONNECTING (1)
#define WIFI_CONNECT_CONNECT_SUCCESS (2)
#define WIFI_CONNECT_CONNECT_IP_OBTAINED (3)
#define WIFI_CONNECT_CONNECT_FAILURE (-1)

#define WIFI_CONTROLLER_STATUS_OFFLINE (0)
#define WIFI_CONTROLLER_STATUS_BUSY_UPDATING (1)
#define WIFI_CONTROLLER_STATUS_OPERATIONAL (2)

#define GETHOSTBYNAME_STATUS_NONE (0)
#define GETHOSTBYNAME_STATUS_SUCCESS (1)
#define GETHOSTBYNAME_STATUS_HOST_NOT_FOUND (-1)
#define GETHOSTBYNAME_STATUS_TIMEOUT (-2)
#define GETHOSTBYNAME_STATUS_SYSTEM_FAILURE (-3)

#pragma pack(push, 1)

struct spectranext_controller_registers_t
{
    // command registers - see WIFI_COMMAND_*
    uint8_t command;
    // scan result feedback - see WIFI_SCAN_*
    int8_t scan_status;
    // connect result feedback - see WIFI_CONNECT_*
    int8_t connection_status;
    // amount of found access points for WIFI_COMMAND_SCAN_ACCESS_POINTS
    uint8_t scan_access_point_count;

    // SSID for WIFI_COMMAND_CONNECT_ACCESS_POINT
    char access_point_name[64];
    // PASSWORD for WIFI_COMMAND_CONNECT_ACCESS_POINT
    char access_point_password[64];

    // controller status - see WIFI_CONTROLLER_STATUS_*
    uint8_t controller_status;

    // gethostbyname status result
    int8_t gethostbyname_status;
    // ipv4 result of a successful gethostbyname query
    uint32_t gethostbyname_ipv4_result;
    // input hostname to gethostbyname query
    char gethostbyname_hostname[96];
    // reserved
    char reserved[22];

    // list of found access points
    struct
    {
        char name[64];
    } access_points[32];
};

#pragma pack(pop)

#endif
