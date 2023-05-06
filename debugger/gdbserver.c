
#include "config.h"

#include "debugger.h"
#include "gdbserver.h"
#include "packets.h"
#include "arch.h"
#include "gdbserver_utils.h"

#include "debugger_internals.h"
#include "event.h"
#include "fuse.h"
#include "infrastructure/startup_manager.h"
#include "memory.h"
#include "mempool.h"
#include "periph.h"
#include "settings.h"
#include "utils.h"
#include "ui/ui.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include "compat.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <semaphore.h>

typedef uint8_t (*trapped_action_t)(const void* data, void* response);

static int gdbserver_socket;
static int gdbserver_client_socket = -1;
static uint8_t tmpbuf[0x20000];
static volatile char gdbserver_trapped = 0;
static volatile char gdbserver_do_not_report_trap = 0;
static int gdbserver_port = 0;

static pthread_t network_thread_id;
static trapped_action_t scheduled_action = NULL;
static const void* scheduled_action_data = NULL;
static void* scheduled_action_response = NULL;
static uint8_t scheduled_action_response_delivered = 0;

static pthread_cond_t trapped_cond;
static pthread_cond_t response_cond;
static pthread_mutex_t trap_process_mutex;
static pthread_mutex_t network_mutex;

static libspectrum_word* registers[] = {
    &AF,
    &BC,
    &DE,
    &HL,
    &SP,
    &PC,
    &IX,
    &IY,
    &AF_,
    &BC_,
    &DE_,
    &HL_,
    &CLOCKL,
    &CLOCKH
};

static uint8_t gdbserver_detrap();
static uint8_t gdbserver_execute_on_main_thread(trapped_action_t call, const void* data, void* response);

static uint8_t action_get_registers(const void* arg, void* response);
static uint8_t action_set_registers(const void* arg, void* response);
static uint8_t action_get_mem(const void* arg, void* response);
static uint8_t action_set_mem(const void* arg, void* response);
static uint8_t action_get_register(const void* arg, void* response);
static uint8_t action_set_register(const void* arg, void* response);
static uint8_t action_set_breakpoint(const void* arg, void* response);
static uint8_t action_remove_breakpoint(const void* arg, void* response);
static uint8_t action_step_instruction(const void* arg, void* response);

struct action_mem_args_t {
    size_t maddr, mlen;
    uint8_t* payload;
};

struct action_step_args_t {
    size_t addr, len;
};

struct action_register_args_t {
    int reg;
    libspectrum_word value;
};

struct action_set_registers_args_t {
    libspectrum_word regs_data[sizeof(registers) / sizeof(libspectrum_word*)];
};

struct action_breakpoint_args_t {
    size_t maddr, mlen;
};

static void process_xfer(const char *name, char *args)
{
  const char *mode = args;
  args = strchr(args, ':');
  *args++ = '\0';
  
  if (!strcmp(name, "features") && !strcmp(mode, "read")) {
      write_packet(FEATURE_STR);
  }
  if (!strcmp(name, "exec-file") && !strcmp(mode, "read")) {
      write_packet("/fuse/emulated");
  }
}

static void process_query(char *payload)
{
    const char *name;
    char *args;

    args = strchr(payload, ':');
    if (args)
        *args++ = '\0';
    name = payload;
    if (!strcmp(name, "C"))
    {
        snprintf((char*)tmpbuf, sizeof(tmpbuf), "QCp%02x.%02x", 1, 1);
        write_packet((const char*)tmpbuf);
    }
    if (!strcmp(name, "Attached"))
    {
        write_packet("1");
    }
    if (!strcmp(name, "Offsets"))
        write_packet("");
    if (!strcmp(name, "Supported"))
        write_packet("PacketSize=4000;qXfer:features:read+;qXfer:auxv:read+");
    if (!strcmp(name, "Symbol"))
        write_packet("OK");
    if (name == strstr(name, "ThreadExtraInfo"))
    {
        args = payload;
        args = 1 + strchr(args, ',');
        write_packet("41414141");
    }
    if (!strcmp(name, "TStatus"))
        write_packet("");
    if (!strcmp(name, "Xfer"))
    {
        name = args;
        args = strchr(args, ':');
        *args++ = '\0';
        return process_xfer(name, args);
    }
    if (!strcmp(name, "fThreadInfo"))
    {
        write_packet("mp01.01");
    }
    if (!strcmp(name, "sThreadInfo"))
        write_packet("l");
}

static void process_vpacket(char *payload)
{
    const char *name;
    char *args;
    args = strchr(payload, ';');
    if (args)
        *args++ = '\0';
    name = payload;

    if (!strcmp("Cont", name))
    {
        if (args[0] == 'c')
        {
            if (gdbserver_detrap())
            {
                write_packet("OK");
            }
            else
            {
                write_packet("E01");
            }
        }
        if (args[0] == 's')
        {
            if (gdbserver_trapped)
            {
                gdbserver_execute_on_main_thread(action_step_instruction, NULL, NULL);
                gdbserver_detrap();
            }
        }
    }
    if (!strcmp("Cont?", name))
      write_packet("vCont;c;C;s;S;");
    if (!strcmp("Kill", name))
    {
      write_packet("OK");
    }
    if (!strcmp("MustReplyEmpty", name))
      write_packet("");
}

static int set_register_value(int reg, libspectrum_word value)
{
    if (reg >= (sizeof(registers) / (sizeof(libspectrum_word*))))
    {
        return 1;
    }
    *registers[reg] = value;
    return 0;
}

static int get_register_value(int reg, libspectrum_word* result)
{
    if (reg >= (sizeof(registers) / (sizeof(libspectrum_word*))))
    {
        return 1;
    }
    *result = *registers[reg];
    return 0;
}

static uint8_t process_packet()
{
    uint8_t *inbuf = inbuf_get();
    int inbuf_size = inbuf_end();
  
    if (inbuf_size == 0)
    {
        return 0;
    }
  
    // printf("r: %.*s\n", inbuf_size, inbuf);
  
    if (inbuf_size >= 1 && *inbuf == INTERRUPT_CHAR)
    {
        inbuf_erase_head(1);
        debugger_mode = DEBUGGER_MODE_HALTED;
        return 1;
    }
  
    uint8_t *packetend_ptr = (uint8_t *)memchr(inbuf, '#', inbuf_size);
    if (packetend_ptr == NULL)
    {
        return 0;
    }
  
    int packetend = packetend_ptr - inbuf;
    assert('$' == inbuf[0]);
    inbuf[packetend] = '\0';

    uint8_t checksum = 0;
    int i;
    for (i = 1; i < packetend; i++)
        checksum += inbuf[i];
  
    if (checksum != (hex(inbuf[packetend + 1]) << 4 | hex(inbuf[packetend + 2])))
    {
        inbuf_erase_head(packetend + 3);
        return 1;
    }
  
    static char recv_data[8196];
    strcpy(recv_data, (char*)&inbuf[1]);
    inbuf_erase_head(packetend + 3);
  
    char request = recv_data[0];
    char *payload = (char *)&recv_data[1];

    switch (request)
    {
        case 'c':
        {
            gdbserver_detrap();
            break;
        }
        case 'D':
        {
            if (gdbserver_detrap())
            {
                write_packet("OK");
            }
            else
            {
                write_packet("");
            }
            break;
        }
        case 'g':
        {
            if (gdbserver_execute_on_main_thread(action_get_registers, NULL, tmpbuf))
                write_packet((const char*)tmpbuf);
            break;
        }
        case 'G':
        {
            struct action_set_registers_args_t r = {};
            if (strlen(payload) != ((sizeof(registers) / sizeof(libspectrum_word*)) * 4))
            {
                write_packet("E01");
                break;
            }
            hex2mem(payload, (void *)&r.regs_data, (sizeof(registers) / sizeof(libspectrum_word*)) * 2);
          
            if (gdbserver_execute_on_main_thread(action_set_registers, &r, tmpbuf))
                write_packet((const char*)tmpbuf);
            break;
        }
        case 'H':
        {
            write_packet("OK");
            break;
        }
        case 'm':
        {
            struct action_mem_args_t mem;
            assert(sscanf(payload, "%zx,%zx", &mem.maddr, &mem.mlen) == 2);
            if (mem.mlen * SZ * 2 > 0x20000)
            {
              puts("Buffer overflow!");
              exit(-1);
            }
          
            if (gdbserver_execute_on_main_thread(action_get_mem, &mem, tmpbuf))
                write_packet((const char*)tmpbuf);
            break;
        }
        case 'M':
        {
            struct action_mem_args_t mem;
            int offset;
            if (sscanf(payload, "%zx,%zx:%n", &mem.maddr, &mem.mlen, &offset) != 2) {
                write_packet("E01");
                break;
            }
            mem.payload = (uint8_t*)(payload + offset);
            if (gdbserver_execute_on_main_thread(action_set_mem, &mem, tmpbuf))
                write_packet((const char*)tmpbuf);
            break;
        }
        case 'p':
        {
            struct action_register_args_t r;
            r.reg = strtol(payload, NULL, 16);
            if (gdbserver_execute_on_main_thread(action_get_register, &r, tmpbuf))
                write_packet((const char*)tmpbuf);
            break;
        }
        case 'P':
        {
            struct action_register_args_t r;
            r.reg = strtol(payload, NULL, 16);
            assert('=' == *payload++);
          
            hex2mem(payload, (void *)&r.value, SZ * 2);
          
            if (gdbserver_execute_on_main_thread(action_set_register, &r, tmpbuf))
                write_packet((const char*)tmpbuf);
          
            break;
        }
        case 'q':
        {
            process_query(payload);
            break;
        }
        case 's':
        {
            gdbserver_execute_on_main_thread(action_step_instruction, NULL, NULL);
            gdbserver_detrap();
            break;
        }
        case 'i':
        {
            if (gdbserver_trapped)
            {
                size_t a, b;
              
                if (sscanf(payload, "%zx,%zx", &a, &b) == 2)
                {
                    struct action_step_args_t ar;
                    ar.addr = a;
                    ar.len = b;
                    gdbserver_execute_on_main_thread(action_step_instruction, &ar, NULL);
                    gdbserver_detrap();
                }
                else if (sscanf(payload, "%zx", &a) == 1)
                {
                    struct action_step_args_t ar;
                    ar.addr = 0;
                    ar.len = a;
                    gdbserver_execute_on_main_thread(action_step_instruction, &ar, NULL);
                    gdbserver_detrap();
                }
                else
                {
                    gdbserver_execute_on_main_thread(action_step_instruction, NULL, NULL);
                    gdbserver_detrap();
                }
            }
            else
            {
                write_packet("E01");
            }
          
            break;
        }
        case 'v':
        {
            process_vpacket(payload);
            break;
        }
        case 'X':
        {
            size_t maddr, mlen;
            int offset, new_len;
            if (sscanf(payload, "%zx,%zx:%n", &maddr, &mlen, &offset) != 2) {
                write_packet("E01");
                break;
            }
            payload += offset;
            new_len = unescape(payload, (char *)packetend_ptr - payload);
            if (new_len != mlen) {
                write_packet("E01");
                break;
            }
          
            struct action_mem_args_t mem;
            mem.payload = (uint8_t*)payload;
            mem.maddr = maddr;
            mem.mlen = mlen;
          
            if (gdbserver_execute_on_main_thread(action_set_mem, &mem, tmpbuf))
                write_packet((const char*)tmpbuf);
            break;
        }
        case 'Z':
        {
            size_t type, addr, length;
            assert(sscanf(payload, "%zx,%zx,%zx", &type, &addr, &length) == 3);
          
            struct action_breakpoint_args_t b;
            b.maddr = addr;
          
            if (gdbserver_execute_on_main_thread(action_set_breakpoint, &b, tmpbuf))
                write_packet((const char*)tmpbuf);
        
            break;
        }
        case 'z':
        {
            size_t type, addr, length;
            assert(sscanf(payload, "%zx,%zx,%zx", &type, &addr, &length) == 3);
          
            struct action_breakpoint_args_t b;
            b.maddr = addr;
          
            if (gdbserver_execute_on_main_thread(action_remove_breakpoint, &b, tmpbuf))
                write_packet((const char*)tmpbuf);
        
            break;
        }
        case '?':
        {
            if (gdbserver_trapped)
            {
                char tbuf[64];
                sprintf(tbuf, "T%02xthread:p%02x.%02x;", 5, 1, 1);
                write_packet(tbuf);
            }
            else
            {
                write_packet("OK");
            }
            break;
        }
        default:
        {
            write_packet("");
        }
    }
  
    return 0;
}


static int process_network(int socket)
{
    int ret;
    if ((ret = read_packet(socket)))
    {
        return ret;
    }

    pthread_mutex_lock(&network_mutex);
    acknowledge_packet(socket);
    while (process_packet()) ;
    write_flush(socket);
    pthread_mutex_unlock(&network_mutex);
  
    return 0;
}

static void* network_thread(void* arg)
{
    while (gdbserver_debugging_enabled)
    {
        {
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 500000;

            pthread_mutex_lock(&network_mutex);
            
            if (gdbserver_socket < 0)
            {
                pthread_mutex_unlock(&network_mutex);
                break;
            }
            
            fd_set set;
            FD_ZERO(&set);
            FD_SET(gdbserver_socket, &set);

            if (select(gdbserver_socket + 1, &set, NULL, NULL, &timeout) <= 0)
            {
                pthread_mutex_unlock(&network_mutex);
                continue;
            }
            pthread_mutex_unlock(&network_mutex);
        }

        socklen_t socklen;
        struct sockaddr_in connected_addr;
        int sock = accept(gdbserver_socket, (struct sockaddr*)&connected_addr, &socklen);
        if (sock < 0)
        {
            continue;
        }
      
        int optval = 1;
        setsockopt (sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, sizeof(optval));
#ifndef WIN32
        int interval = 1;
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(int));
        int maxpkt = 10;
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &maxpkt, sizeof(int));
#endif

        printf("Accepted new socket: %d\n", sock);

        gdbserver_client_socket = sock;
        gdbserver_do_not_report_trap = 1;
        debugger_mode = DEBUGGER_MODE_HALTED;

        // property wait for it to trap
        pthread_mutex_lock(&trap_process_mutex);
        while (!gdbserver_trapped)
        {
            pthread_cond_wait(&trapped_cond, &trap_process_mutex);
        }
        pthread_mutex_unlock(&trap_process_mutex);

    
        int ret;
        while ((ret = process_network(gdbserver_client_socket)) == 0) ;
        printf("Socket closed: %d\n", gdbserver_client_socket);
        
        debugger_breakpoint_remove_all();
        printf("Deleted all breakpoints.\n");
        
        compat_socket_close(gdbserver_client_socket);
        gdbserver_client_socket = -1;

        gdbserver_detrap();
    }
    return NULL;
}

void gdbserver_init()
{
    pthread_cond_init(&trapped_cond, NULL);
    pthread_cond_init(&response_cond, NULL);
    pthread_mutex_init(&trap_process_mutex, NULL);
    pthread_mutex_init(&network_mutex, NULL);
    
    gdbserver_refresh_status();
}

int gdbserver_start( int port )
{
    utils_networking_init();

    gdbserver_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (gdbserver_socket == -1)
    {
#ifdef WIN32
        return WSAGetLastError();
#else
        return 1;
#endif
    }

    {
        int reuseval = 1;
        setsockopt(gdbserver_socket, SOL_SOCKET, SO_REUSEADDR, (void*)&reuseval, sizeof(int));
    }

    {
        struct linger sl;
        sl.l_onoff = 1;
        sl.l_linger = 0;
        setsockopt(gdbserver_socket, SOL_SOCKET, SO_LINGER, (void*)&sl, sizeof(sl));
    }

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
  
    if ((bind(gdbserver_socket, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0)
    {
        utils_networking_end();
        return 2;
    }
  
    if ((listen(gdbserver_socket, 5)) != 0)
    {
        utils_networking_end();
        return 3;
    }

    if (pthread_create(&network_thread_id, NULL, network_thread, NULL) != 0)
    {
        utils_networking_end();
        return 4;
    }

    gdbserver_port = port;
    gdbserver_debugging_enabled = 1;
    return 0;
}

void gdbserver_stop()
{
    if (!gdbserver_debugging_enabled)
    {
        return;
    }

    pthread_mutex_lock(&network_mutex);
    if (gdbserver_socket != -1)
    {
        compat_socket_close(gdbserver_socket);
        gdbserver_socket = -1;
    }
    
    pthread_mutex_unlock(&network_mutex);

    gdbserver_debugging_enabled = 0;
    pthread_join(network_thread_id, NULL);
    utils_networking_end();
}

void gdbserver_refresh_status()
{
    if (gdbserver_debugging_enabled ^ settings_current.gdbserver_enable)
    {
        if (settings_current.gdbserver_enable)
        {
            // we've been turned off, now must turn on
            int code = gdbserver_start(settings_current.gdbserver_port);
            if (code)
            {
                ui_error(UI_ERROR_ERROR, "Cannot start gdbserver on port %d, code: %d", settings_current.gdbserver_port, code);
                settings_current.gdbserver_enable = 0;
                ui_menu_activate( UI_MENU_ITEM_MACHINE_DEBUGGER, 1 );
            }
            else
            {
                ui_menu_activate( UI_MENU_ITEM_MACHINE_DEBUGGER, 0 );
            }
        }
        else
        {
            // we've been turned on, now must turn off
            gdbserver_stop();
            ui_menu_activate( UI_MENU_ITEM_MACHINE_DEBUGGER, 1 );
        }
    }
    else
    {
        if (gdbserver_debugging_enabled && (gdbserver_port != settings_current.gdbserver_port))
        {
            // we need to restart
            gdbserver_stop();
            int code = gdbserver_start(settings_current.gdbserver_port);
            if (code)
            {
                ui_error(UI_ERROR_ERROR, "Cannot restart gdbserver on port %d, code: %d", settings_current.gdbserver_port, code);
                settings_current.gdbserver_enable = 0;
                ui_menu_activate( UI_MENU_ITEM_MACHINE_DEBUGGER, 1 );
            }
        }
    }
}

static uint8_t gdbserver_detrap()
{
    uint8_t result = 0;
    pthread_mutex_lock(&trap_process_mutex);
    if (gdbserver_trapped)
    {
        gdbserver_trapped = 0;
        pthread_cond_signal(&trapped_cond);
        result = 1;
    }
    pthread_mutex_unlock(&trap_process_mutex);
    return result;
}

// schedule a simple job (call) on the main thread, while it's trapped
// data and response are supposed to be located on the stack of the caller (network) thread,
// as it's going to be stopped until the response is there
static uint8_t gdbserver_execute_on_main_thread(trapped_action_t call, const void* data, void* response)
{
    pthread_mutex_lock(&trap_process_mutex);

    if (gdbserver_trapped != 1 && debugger_mode != DEBUGGER_MODE_HALTED)
    {
        pthread_mutex_unlock(&trap_process_mutex);
        return 0;
    }

    // prepare the action arguments
    scheduled_action = call;
    scheduled_action_data = data;
    scheduled_action_response = response;
    scheduled_action_response_delivered = 0;

    // execute
    pthread_cond_signal(&trapped_cond);
    pthread_mutex_unlock(&trap_process_mutex);
    
    pthread_yield_np();

    pthread_mutex_lock(&trap_process_mutex);
    // wait for the response
    while (scheduled_action_response_delivered == 0)
    {
        pthread_cond_wait(&response_cond, &trap_process_mutex);
    }
    pthread_mutex_unlock(&trap_process_mutex);
    return 1;
}

static uint8_t action_get_registers(const void* arg, void* response)
{
    int i;
    char* resp_buff = (char*)response;
  
    libspectrum_word regs[sizeof(registers) / (sizeof(libspectrum_word*))];
  
    for (i = 0; i < sizeof(registers) / (sizeof(libspectrum_word*)); i++)
    {
        get_register_value(i, &regs[i]);
    }
  
    mem2hex((const uint8_t *)regs, resp_buff, (sizeof(registers) / sizeof(libspectrum_word*)) * 2);
  
    return 0;
}

static uint8_t action_set_registers(const void* arg, void* response)
{
    int i;
    struct action_set_registers_args_t* regs = (struct action_set_registers_args_t*)arg;
    char* resp_buff = (char*)response;
  
    for (i = 0; i < sizeof(registers) / (sizeof(libspectrum_word*)); i++)
    {
        set_register_value(i, regs->regs_data[i]);
    }
  
    strcpy(resp_buff, "OK");
    return 0;
}

static uint8_t action_get_mem(const void* arg, void* response)
{
    int i;
    struct action_mem_args_t* mem = (struct action_mem_args_t*)arg;
    char* resp_buff = (char*)response;
  
    uint32_t t = tstates;
  
    libspectrum_word address = mem->maddr;
    for (i = 0; i < mem->mlen; i++, address++)
    {
        libspectrum_byte data = readbyte(address);
        mem2hex((void *)&data, resp_buff + i * 2, 1);
    }
  
    tstates = t;

    resp_buff[mem->mlen * 2] = '\0';
    return 0;
}

static uint8_t action_set_mem(const void* arg, void* response)
{
    uint32_t t = tstates;
  
    int i;
    struct action_mem_args_t* mem = (struct action_mem_args_t*)arg;
    char* resp_buff = (char*)response;
  
    libspectrum_byte data;
    libspectrum_word address = mem->maddr;
    for (i = 0; i < mem->mlen; i++, address++)
    {
        hex2mem((char*)mem->payload + i * 2, (uint8_t *)&data, 1);
        writebyte(address, data);
    }
  
    tstates = t;
  
    strcpy(resp_buff, "OK");
    return 0;
}

static uint8_t action_get_register(const void* arg, void* response)
{
    struct action_register_args_t* r = (struct action_register_args_t*)arg;
    char* resp_buff = (char*)response;

    libspectrum_word reg;
    if (get_register_value(r->reg, &reg))
    {
        strcpy(resp_buff, "E01");
        return 0;
    }

    mem2hex((const uint8_t *)&reg, resp_buff, SZ);
    resp_buff[SZ * 2] = '\0';
    return 0;
}

static uint8_t action_set_register(const void* arg, void* response)
{
    struct action_register_args_t* r = (struct action_register_args_t*)arg;
    char* resp_buff = (char*)response;
  
    if (set_register_value(r->reg, r->value))
    {
        strcpy(resp_buff, "E01");
        return 0;
    }

    strcpy(resp_buff, "OK");
    return 0;
}

static uint8_t action_set_breakpoint(const void* arg, void* response)
{
    struct action_breakpoint_args_t* b = (struct action_breakpoint_args_t*)arg;
    char* resp_buff = (char*)response;
  
    if (debugger_breakpoint_add_address(
        DEBUGGER_BREAKPOINT_TYPE_EXECUTE, memory_source_any, 0, b->maddr, 0,
        DEBUGGER_BREAKPOINT_LIFE_PERMANENT, NULL))
    {
        strcpy(resp_buff, "E01");
    }
    else
    {
        strcpy(resp_buff, "OK");
    }
  
    return 0;
}

static uint8_t action_remove_breakpoint(const void* arg, void* response)
{
    struct action_breakpoint_args_t* b = (struct action_breakpoint_args_t*)arg;
    char* resp_buff = (char*)response;
  
    libspectrum_word address = b->maddr;
    GSList* ptr;
    debugger_breakpoint* found = NULL;
    for(ptr = debugger_breakpoints; ptr; ptr = ptr->next)
    {
        debugger_breakpoint* p = (debugger_breakpoint*)ptr->data;
        if (p->type != DEBUGGER_BREAKPOINT_TYPE_EXECUTE)
            continue;
        if (p->value.address.source != memory_source_any)
            continue;
        if (p->value.address.offset != address)
            continue;
        found = p;
        break;
    }

    if (found)
    {
        debugger_breakpoint_remove(found->id);
        strcpy(resp_buff, "OK");
    }
    else
    {
        strcpy(resp_buff, "E01");
    }
  
    return 0;
}

static uint8_t action_step_instruction(const void* arg, void* response)
{
    struct action_step_args_t* a = (struct action_step_args_t*)arg;

    if (a == NULL)
    {
        return 1;
    }
    else
    {
        if (a->addr != 0)
        {
            PC = a->addr;
        }
      
        debugger_breakpoint_add_address(
            DEBUGGER_BREAKPOINT_TYPE_EXECUTE, memory_source_any, 0, PC + a->len, 0,
            DEBUGGER_BREAKPOINT_LIFE_ONESHOT, NULL
        );
    }
  
    return 0;
}

int gdbserver_activate()
{
    // printf("Execution stopped: trapped.\n");

    if (gdbserver_do_not_report_trap == 0)
    {
        pthread_mutex_lock(&network_mutex);
        inbuf_reset();
        // notify the gdb client that we have trapped
        char tbuf[64];
        sprintf(tbuf, "T%02xthread:p%02x.%02x;", 5, 1, 1);
        write_packet(tbuf);
        write_flush(gdbserver_client_socket);
        pthread_mutex_unlock(&network_mutex);
    }

    pthread_mutex_lock(&trap_process_mutex);
  
    gdbserver_do_not_report_trap = 0;
    gdbserver_trapped = 1;
    pthread_cond_signal(&trapped_cond);
  
    uint8_t halt = 0;

    // a simple loop that waits for someone to unlock, or postone a simple
    // action (only one at a time)
    do
    {

        if (!gdbserver_trapped)
        {
            break;
        }

        pthread_cond_wait(&trapped_cond, &trap_process_mutex);

        if (scheduled_action != NULL)
        {
            halt = scheduled_action(scheduled_action_data, scheduled_action_response);
            scheduled_action = NULL;
            scheduled_action_response_delivered = 1;
            
            // notify the waiter that we're done
            pthread_cond_signal(&response_cond);
        }
      
    } while (1);

    if (halt == 0)
    {
        debugger_mode = DEBUGGER_MODE_ACTIVE;
        // printf("Execution resumed.\n");
    }
    else
    {
        debugger_mode = DEBUGGER_MODE_HALTED;
        gdbserver_trapped = 0;
    }

    pthread_mutex_unlock(&trap_process_mutex);

    return 0;
}
