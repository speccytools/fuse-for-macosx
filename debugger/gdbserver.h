
#ifndef FUSE_DEBUGGER_GDBSERVER_H
#define FUSE_DEBUGGER_GDBSERVER_H

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

#endif				/* #ifndef FUSE_DEBUGGER_GDBSERVER_H */
