/* z80_ops.c: Process the next opcode
   Copyright (c) 1999-2003 Philip Kendall, Witold Filipczyk

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
#include "rzx.h"
#include "settings.h"
#include "spectrum.h"
#include "tape.h"
#include "trdos.h"
#include "z80.h"

#include "z80_macros.h"

#ifndef HAVE_ENOUGH_MEMORY
static void z80_cbxx(BYTE opcode2);
static void z80_ddxx(BYTE opcode2);
static void z80_edxx(BYTE opcode2);
static void z80_fdxx(BYTE opcode2);
static void z80_ddfdcbxx(BYTE opcode3, WORD tempaddr);
#endif				/* #ifndef HAVE_ENOUGH_MEMORY */

/* Execute Z80 opcodes until the next event */
void z80_do_opcodes()
{

  while(tstates < event_next_event ) {

    BYTE opcode;

    /* If we're due an interrupt from RZX playback, generate one */
    if( rzx_playback &&
	R + rzx_instructions_offset >= rzx_instruction_count
      ) {
      event_add( tstates, EVENT_TYPE_INTERRUPT );
      break;		/* And break out of the execution loop to let
			   the interrupt happen */
    }

    /* Check if the debugger should become active at this point;
       special case DEBUGGER_MODE_INACTIVE for alleged performance
       reasons */
    if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
	debugger_check( DEBUGGER_BREAKPOINT_TYPE_EXECUTE, PC ) )
      debugger_trap();

    if( machine_current->ram.current_rom == 1 && PC >= 16384 ) {
      trdos_active = 0;
    } else if ( machine_current->ram.current_rom == 1 && 
		( PC & 0xff00 ) == 0x3d00 ) {
      trdos_active = 1;
    }

    /* Do the instruction fetch; readbyte_internal used here to avoid
       triggering read breakpoints */
    contend( PC, 4 ); R++;
    opcode = readbyte_internal( PC++ );

    switch(opcode) {
#include "opcodes_base.c"
    }

  }

}

#ifndef HAVE_ENOUGH_MEMORY

static void z80_cbxx(BYTE opcode2)
{
  switch(opcode2) {
#include "z80_cb.c"
  }
}

static void z80_ddxx(BYTE opcode2)
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

static void z80_edxx(BYTE opcode2)
{
  switch(opcode2) {
#include "z80_ed.c"
  }
}

static void z80_fdxx(BYTE opcode2)
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

static void z80_ddfdcbxx(BYTE opcode3, WORD tempaddr)
{
  switch(opcode3) {
#include "z80_ddfdcb.c"
  }
}

#endif			/* #ifndef HAVE_ENOUGH_MEMORY */
