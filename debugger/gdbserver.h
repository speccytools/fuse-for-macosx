
#ifndef FUSE_DEBUGGER_GDBSERVER_H
#define FUSE_DEBUGGER_GDBSERVER_H

#include <stdint.h>

// Trap reasons for GDB RSP protocol
#define DEBUG_TRAP_REASON_SIGNAL_RECEIVED 0
#define DEBUG_TRAP_REASON_BREAKPOINT 1
#define DEBUG_TRAP_REASON_OTHER 2

void gdbserver_init();
int gdbserver_start( int port );
void gdbserver_stop();
int gdbserver_activate();
int gdbserver_activate_with_reason(int trap_reason);
void gdbserver_refresh_status();
void gdbserver_schedule_reset(void);
void gdbserver_schedule_autoboot(void);
void gdbserver_send_remote_console_output(const char *text);
uint8_t gdbserver_reset_via_remote_command(void);
void gdbserver_lock_network_write(void);
void gdbserver_unlock_network_write(void);

typedef uint8_t (*trapped_action_t)(const void* data, void* response);
uint8_t gdbserver_execute_on_main_thread(trapped_action_t call, const void* data, void* response);

/** Called from machine_reset() — applies one-shot Spectranext autoboot config or turns autoboot off. */
void gdbserver_on_machine_reset(void);

#endif				/* #ifndef FUSE_DEBUGGER_GDBSERVER_H */
