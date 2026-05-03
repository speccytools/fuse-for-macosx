#include "config.h"

#include "debugger/gdbserver.h"
#include "spectranext_stdout.h"

#include <stdio.h>
#include <string.h>

#define SPECTRANEXT_STDOUT_BUFFER_SIZE 1024

static char spectranext_stdout_buffer[SPECTRANEXT_STDOUT_BUFFER_SIZE];
static size_t spectranext_stdout_buffer_length = 0;

static void
spectranext_stdout_flush_buffer( void )
{
    if( !spectranext_stdout_buffer_length ) return;

    spectranext_stdout_buffer[spectranext_stdout_buffer_length] = '\0';
    fwrite( spectranext_stdout_buffer, 1, spectranext_stdout_buffer_length, stdout );
    fflush( stdout );
    gdbserver_send_remote_console_output( spectranext_stdout_buffer );
    spectranext_stdout_buffer_length = 0;
}

/*
 * Spectranext terminal stdout: OUT to port 0x043b pushes a byte into the host
 * ring (see rp2350 critical.c address_high == 0x04). Fuse forwards each byte
 * to the process stdout, similar to console.c draining critical_term_stdout_get.
 */
void spectranext_stdout_write(libspectrum_word port, libspectrum_byte data)
{
    (void)port;

    if( spectranext_stdout_buffer_length >= SPECTRANEXT_STDOUT_BUFFER_SIZE - 1 )
        spectranext_stdout_flush_buffer();

    spectranext_stdout_buffer[spectranext_stdout_buffer_length++] = (char)data;

    if( data == '\n' )
        spectranext_stdout_flush_buffer();
}
