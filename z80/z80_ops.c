/* z80_ops.c: Process the next opcode
   Copyright (c) 1999-2004 Philip Kendall, Witold Filipczyk

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <stdio.h>

#include "debugger/debugger.h"
#include "event.h"
#include "machine.h"
#include "memory.h"
#include "periph.h"
#include "profile.h"
#include "rzx.h"
#include "if1.h"
#include "settings.h"
#include "slt.h"
#include "tape.h"
#include "trdos.h"
#include "ula.h"
#include "z80.h"

#include "z80_macros.h"

#ifndef HAVE_ENOUGH_MEMORY
static void z80_cbxx( libspectrum_byte opcode2 );
static void z80_ddxx( libspectrum_byte opcode2 );
static void z80_edxx( libspectrum_byte opcode2 );
static void z80_fdxx( libspectrum_byte opcode2 );
static void z80_ddfdcbxx( libspectrum_byte opcode3,
			  libspectrum_word tempaddr );
#endif				/* #ifndef HAVE_ENOUGH_MEMORY */

/* Certain features (eg RZX playback trigged interrupts, the debugger,
   TR-DOS ROM paging) can't be handled within the normal 'events'
   framework as they don't happen at a specified tstate. In order to
   support these, we basically need to check every opcode as to
   whether they have occured or not.

   There are (fairly common) circumstances under which we know that
   the features will never occur (eg we will never get an interrupt
   from RZX playback unless we're doing RZX playback), and we would
   quite like to skip the check in this state. We can do this if we
   use gcc's computed goto feature[1]. What follows is some
   preprocessor hackery to moderately transparently do this while
   still retaining the "normal" behaviour for non-gcc compilers.

   Ensure that the same arguments are given to respective
   SETUP_CHECK() and CHECK() macros or everything will break.

   [1] see 'C Extensions', 'Labels as Values' in the gcc info page.
*/

#ifdef __GNUC__

#define SETUP_CHECK( label, condition, dest ) \
  if( condition ) { cgoto[ next ] = &&label; next = dest + 1; }
#define CHECK( label, condition, dest ) goto *cgoto[ dest ]; label:
#define END_CHECK

#else				/* #ifdef __GNUC__ */

#define CHECK( label, condition, dest ) if( condition ) {
#define END_CHECK }

#endif				/* #ifdef __GNUC__ */

/* Execute Z80 opcodes until the next event */
void
z80_do_opcodes( void )
{
  int even_m1;
  void *cgoto[6]; size_t next;
  libspectrum_byte opcode = 0x00;

  /* Avoid 'variable not used' warnings if we're not using gcc */
  cgoto[0] = cgoto[0]; next = next;

  even_m1 =
    machine_current->capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_EVEN_M1; 

#ifdef __GNUC__

  next = 0;
  SETUP_CHECK( profile, profile_active, 0 );
  SETUP_CHECK( rzx, rzx_playback, 1 );
  SETUP_CHECK( debugger, debugger_mode != DEBUGGER_MODE_INACTIVE, 2 );
  SETUP_CHECK( trdos, trdos_available, 3 );
  SETUP_CHECK( if1p, if1_available, 4 );
  if( next != 5 ) { cgoto[ next ] = &&opcode_delay; }

  next = 5;
  SETUP_CHECK( evenm1, even_m1, 5 );
  if( next != 6 ) { cgoto[ next ] = &&run_opcode; }
  next = 6;
  SETUP_CHECK( if1u, if1_available, 6 );
  if( next != 7 ) { cgoto[ next ] = &&end_opcode; }

#endif				/* #ifdef __GNUC__ */

  while( tstates < event_next_event ) {

    /* Profiler */
    CHECK( profile, profile_active, 0 )

    profile_map( PC );

    END_CHECK

    /* If we're due an end of frame from RZX playback, generate one */
    CHECK( rzx, rzx_playback, 1 )

    if( R + rzx_instructions_offset >= rzx_instruction_count ) {
      event_add( tstates, EVENT_TYPE_FRAME );
      break;		/* And break out of the execution loop to let
			   the interrupt happen */
    }

    END_CHECK

    /* Check if the debugger should become active at this point */
    CHECK( debugger, debugger_mode != DEBUGGER_MODE_INACTIVE, 2 )

    if( debugger_check( DEBUGGER_BREAKPOINT_TYPE_EXECUTE, PC ) )
      debugger_trap();

    END_CHECK

    CHECK( trdos, trdos_available, 3 )

    if( trdos_active ) {
      if( machine_current->ram.current_rom &&
	  PC >= 16384 ) {
	trdos_unpage();
      }
    } else if( ( PC & 0xff00 ) == 0x3d00 &&
	       machine_current->ram.current_rom ) {
      trdos_page();
    }

    END_CHECK

    CHECK( if1p, if1_available, 4 )

    if( PC == 0x0008 || PC == 0x1708 ) {
      if1_page();
    }

    END_CHECK

  opcode_delay:

    contend_read( PC, 4 );

    /* Check to see if M1 cycles happen on even tstates */
    CHECK( evenm1, even_m1, 5 )

    if( tstates & 1 ) tstates++;

    END_CHECK

  run_opcode:
    /* Do the instruction fetch; readbyte_internal used here to avoid
       triggering read breakpoints */
    opcode = readbyte_internal( PC );

    CHECK( if1u, if1_available, 6 )

    if( PC == 0x0700 ) {
      if1_unpage();
    }

    END_CHECK

  end_opcode:
    PC++; R++;
    switch(opcode) {
#include "opcodes_base.c"
    }

  }

}

#ifndef HAVE_ENOUGH_MEMORY

static void
z80_cbxx( libspectrum_byte opcode2 )
{
  switch(opcode2) {
#include "z80_cb.c"
  }
}

static void
z80_ddxx( libspectrum_byte opcode2 )
{
  switch(opcode2) {
#define REGISTER  IX
#define REGISTERL IXL
#define REGISTERH IXH
#include "z80_ddfd.c"
#undef REGISTERH
#undef REGISTERL
#undef REGISTER
  }
}

static void
z80_edxx( libspectrum_byte opcode2 )
{
  switch(opcode2) {
#include "z80_ed.c"
  }
}

static void
z80_fdxx( libspectrum_byte opcode2 )
{
  switch(opcode2) {
#define REGISTER  IY
#define REGISTERL IYL
#define REGISTERH IYH
#include "z80_ddfd.c"
#undef REGISTERH
#undef REGISTERL
#undef REGISTER
  }
}

static void
z80_ddfdcbxx( libspectrum_byte opcode3, libspectrum_word tempaddr )
{
  switch(opcode3) {
#include "z80_ddfdcb.c"
  }
}

#endif			/* #ifndef HAVE_ENOUGH_MEMORY */
