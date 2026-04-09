#include "config.h"

#include "spectranext_stdout.h"

#include <stdio.h>

/*
 * Spectranext terminal stdout: OUT to port 0x043b pushes a byte into the host
 * ring (see rp2350 critical.c address_high == 0x04). Fuse forwards each byte
 * to the process stdout, similar to console.c draining critical_term_stdout_get.
 */
void spectranext_stdout_write(libspectrum_word port, libspectrum_byte data)
{
    (void)port;
    putchar((int)data);
    fflush(stdout);
}
