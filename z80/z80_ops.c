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
#include "rzx.h"
#include "settings.h"
#include "tape.h"
#include "trdos.h"
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

#ifdef __GNUC__

  void *cgoto[3]; size_t next = 0;

  SETUP_CHECK( rzx, rzx_playback, 0 );
  SETUP_CHECK( debugger, debugger_mode != DEBUGGER_MODE_INACTIVE, 1 );
  SETUP_CHECK( trdos, trdos_available, 2 );

  if( next != 3 ) { cgoto[ next ] = &&run_opcode; }

#endif				/* #ifdef __GNUC__ */

  while( tstates < event_next_event ) {

    libspectrum_byte opcode;

    /* If we're due an end of frame from RZX playback, generate one */
    CHECK( rzx, rzx_playback, 0 )

    if( R + rzx_instructions_offset >= rzx_instruction_count ) {
      event_add( tstates, EVENT_TYPE_FRAME );
      break;		/* And break out of the execution loop to let
			   the interrupt happen */
    }

    END_CHECK

    /* Check if the debugger should become active at this point */
    CHECK( debugger, debugger_mode != DEBUGGER_MODE_INACTIVE, 1 )

    if( debugger_check( DEBUGGER_BREAKPOINT_TYPE_EXECUTE, PC ) )
      debugger_trap();

    END_CHECK

    CHECK( trdos, trdos_available, 2 )

    if( trdos_active ) {
      if( machine_current->ram.current_rom == 1 &&
	  PC >= 16384 ) {
	trdos_active = 0;
	memory_map[0].page = &ROM[ machine_current->ram.current_rom ][0x0000];
	memory_map[1].page = &ROM[ machine_current->ram.current_rom ][0x2000];
      }
    } else if( ( PC & 0xff00 ) == 0x3d00 &&
	       machine_current->ram.current_rom == 1 ) {
      trdos_active = 1;
      memory_map[0].page = &ROM[2][0x0000];
      memory_map[1].page = &ROM[2][0x2000];
    }

    END_CHECK

  run_opcode:

    /* Do the instruction fetch; readbyte_internal used here to avoid
       triggering read breakpoints */
    contend( PC, 4 ); R++;
    opcode = readbyte_internal( PC ); PC++;

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
