
#ifndef FUSE_DEBUGGER_GDBSERVER_H
#define FUSE_DEBUGGER_GDBSERVER_H

void gdbserver_init();
int gdbserver_start( int port );
void gdbserver_stop();
int gdbserver_activate();
void gdbserver_refresh_status();

#endif				/* #ifndef FUSE_DEBUGGER_GDBSERVER_H */
