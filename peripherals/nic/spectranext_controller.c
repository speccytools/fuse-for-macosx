#include "config.h"

#include "spectranext_controller.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>

#include "engines/engine.h"

#include "libspectrum.h"
#include "memory_pages.h"

spectranext_enginecall_args_t spectranext_enginecall_args;

spectranext_state_t spectranext_state = {
    .controller_status = WIFI_CONTROLLER_STATUS_OPERATIONAL,
    .connection_status = WIFI_CONNECT_CONNECT_IP_OBTAINED,
    .ipv4_host = 0x7f000001u,
};

static char scan_ap_names[SPECTRANEXT_SCAN_AP_MAX][64];
static uint8_t scan_ap_count;

volatile struct spectranext_controller_t spectranext_controller = {
    .command = SPECTRANEXT_CMD_REG_IDLE,
    .status = SPECTRANEXT_STATUS_SUCCESS,
};

int spectranext_enginecall_dispatch(const char *input_file, const char *output_file, const char *operation)
{
    char opbuf[256];
    strncpy(opbuf, operation, sizeof(opbuf) - 1u);
    opbuf[sizeof(opbuf) - 1u] = '\0';

    char *argv[ENGINE_MAX_ARGS];
    const int argc = engine_argv_parse(opbuf, argv, ENGINE_MAX_ARGS);
    if (argc < 1)
        return -6;

    if (strcmp(argv[0], "jsonpath") == 0 || strcmp(argv[0], "json") == 0)
        return engine_json_call(input_file, output_file, argc, argv);
    if (strcmp(argv[0], "xpath") == 0)
        return engine_xpath_call(input_file, output_file, argc, argv);
    return -1;
}

static void spectranext_set_status(uint8_t status)
{
    spectranext_controller.status = status;
}

static void spectranext_controller_process_command(void)
{
    const uint8_t cmd = spectranext_controller.command;
    spectranext_controller.command = SPECTRANEXT_CMD_REG_IDLE;

    switch (cmd)
    {
        case SPECTRANEXT_CMD_GET_CONTROLLER_STATUS:
            spectranext_controller.workspace.get_controller_status.out.controller_status =
                spectranext_state.controller_status;
            spectranext_controller.workspace.get_controller_status.out.wifi_connection =
                spectranext_state.connection_status;
            spectranext_controller.workspace.get_controller_status.out.ipv4 = spectranext_state.ipv4_host;
            spectranext_set_status(SPECTRANEXT_STATUS_SUCCESS);
            break;

        case SPECTRANEXT_CMD_WIFI_SCAN_ACCESS_POINTS:
            scan_ap_count = 1;
            strncpy(scan_ap_names[0], "spectranext", sizeof(scan_ap_names[0]) - 1u);
            scan_ap_names[0][sizeof(scan_ap_names[0]) - 1u] = '\0';
            spectranext_controller.workspace.wifi_scan.io.out.scan_count =
                (uint8_t)(scan_ap_count > 255u ? 255u : scan_ap_count);
            spectranext_set_status(SPECTRANEXT_STATUS_SUCCESS);
            break;

        case SPECTRANEXT_CMD_WIFI_GET_ACCESS_POINT:
        {
            const uint8_t idx = spectranext_controller.workspace.wifi_get_ap.io.in.ap_index;
            if ((uint16_t)idx >= (uint16_t)scan_ap_count)
            {
                spectranext_set_status(SPECTRANEXT_STATUS_ERROR);
                break;
            }
            strncpy((char *)spectranext_controller.workspace.wifi_get_ap.io.out.ap_name, scan_ap_names[idx],
                    sizeof(spectranext_controller.workspace.wifi_get_ap.io.out.ap_name) - 1u);
            spectranext_controller.workspace.wifi_get_ap.io.out.ap_name
                [sizeof(spectranext_controller.workspace.wifi_get_ap.io.out.ap_name) - 1u] = '\0';
            spectranext_set_status(SPECTRANEXT_STATUS_SUCCESS);
            break;
        }

        case SPECTRANEXT_CMD_WIFI_CONNECT_ACCESS_POINT:
            spectranext_state.connection_status = WIFI_CONNECT_CONNECT_SUCCESS;
            spectranext_set_status(SPECTRANEXT_STATUS_SUCCESS);
            break;

        case SPECTRANEXT_CMD_WIFI_DISCONNECT:
            spectranext_state.connection_status = WIFI_CONNECT_DISCONNECTED;
            spectranext_set_status(SPECTRANEXT_STATUS_SUCCESS);
            break;

        case SPECTRANEXT_CMD_DNS_GETHOSTBYNAME:
        {
            char host[64];
            memcpy(host, (const void *)spectranext_controller.workspace.dns.io.in.host, 63);
            host[63] = '\0';

            if (host[0] == '\0')
            {
                spectranext_controller.workspace.dns.io.out.ipv4 = 0;
                spectranext_set_status(SPECTRANEXT_STATUS_ERROR);
                break;
            }

            struct addrinfo hints;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            struct addrinfo *res = NULL;
            const int gai_err = getaddrinfo(host, NULL, &hints, &res);
            if (gai_err != 0 || res == NULL)
            {
                spectranext_controller.workspace.dns.io.out.ipv4 = 0;
                spectranext_set_status(SPECTRANEXT_STATUS_ERROR);
                if (res)
                    freeaddrinfo(res);
                break;
            }

            uint32_t ipv4_host = 0;
            int found = 0;
            for (struct addrinfo *rp = res; rp != NULL; rp = rp->ai_next)
            {
                if (rp->ai_family == AF_INET && rp->ai_addr != NULL)
                {
                    const struct sockaddr_in *sin = (const struct sockaddr_in *)rp->ai_addr;
                    ipv4_host = (uint32_t)sin->sin_addr.s_addr;
                    found = 1;
                    break;
                }
            }
            freeaddrinfo(res);

            if (!found)
            {
                spectranext_controller.workspace.dns.io.out.ipv4 = 0;
                spectranext_set_status(SPECTRANEXT_STATUS_ERROR);
                break;
            }

            spectranext_controller.workspace.dns.io.out.ipv4 = ipv4_host;
            spectranext_state.ipv4_host = ipv4_host;
            spectranext_set_status(SPECTRANEXT_STATUS_SUCCESS);
            break;
        }

        case SPECTRANEXT_CMD_ENGINECALL:
            memcpy(spectranext_enginecall_args.input_file,
                   (const void *)spectranext_controller.workspace.enginecall.io.input_file,
                   sizeof(spectranext_enginecall_args.input_file) - 1u);
            spectranext_enginecall_args.input_file[sizeof(spectranext_enginecall_args.input_file) - 1u] = '\0';

            memcpy(spectranext_enginecall_args.output_file,
                   (const void *)spectranext_controller.workspace.enginecall.io.output_file,
                   sizeof(spectranext_enginecall_args.output_file) - 1u);
            spectranext_enginecall_args.output_file[sizeof(spectranext_enginecall_args.output_file) - 1u] = '\0';

            memcpy(spectranext_enginecall_args.operation,
                   (const void *)spectranext_controller.workspace.enginecall.io.operation,
                   sizeof(spectranext_enginecall_args.operation) - 1u);
            spectranext_enginecall_args.operation[sizeof(spectranext_enginecall_args.operation) - 1u] = '\0';

            spectranext_enginecall_args.result = spectranext_enginecall_dispatch(
                spectranext_enginecall_args.input_file,
                spectranext_enginecall_args.output_file,
                spectranext_enginecall_args.operation);

            spectranext_set_status((uint8_t)(int8_t)spectranext_enginecall_args.result);
            break;

        default:
            spectranext_set_status(SPECTRANEXT_STATUS_ERROR);
            break;
    }
}

void spectranext_controller_init(void)
{
    spectranext_controller.command = SPECTRANEXT_CMD_REG_IDLE;
    spectranext_controller.status = SPECTRANEXT_STATUS_SUCCESS;
    spectranext_state.controller_status = WIFI_CONTROLLER_STATUS_OPERATIONAL;
    spectranext_state.connection_status = WIFI_CONNECT_CONNECT_IP_OBTAINED;
    spectranext_state.ipv4_host = 0x7f000001u;
    scan_ap_count = 0;
}

libspectrum_byte spectranext_controller_read(memory_page *page, libspectrum_word address)
{
    libspectrum_word offset = address & 0xfff;
    uint8_t *registers = (uint8_t *)&spectranext_controller;
    if (offset >= sizeof(spectranext_controller))
        return 0xff;
    return registers[offset];
}

void spectranext_controller_write(memory_page *page, libspectrum_word address, libspectrum_byte b)
{
    libspectrum_word offset = address & 0xfff;
    if (offset >= sizeof(spectranext_controller))
        return;

    uint8_t *registers = (uint8_t *)&spectranext_controller;
    const uint8_t old_command = registers[0];
    registers[offset] = b;

    if (offset == 0 && old_command == SPECTRANEXT_CMD_REG_IDLE && b != SPECTRANEXT_CMD_REG_IDLE)
    {
        spectranext_controller_process_command();
    }
}
