/* spectrum.c: Generic Spectrum routines
   Copyright (c) 1999-2001 Philip Kendall

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

#include "display.h"
#include "event.h"
#include "fuse.h"
#include "keyboard.h"
#include "sound.h"
#include "spec128.h"
#include "spec48.h"
#include "specplus2.h"
#include "specplus3.h"
#include "spectrum.h"
#include "timer.h"
#include "z80/z80.h"

BYTE **ROM;
BYTE RAM[8][0x4000];
DWORD tstates;

int spectrum_interrupt(void)
{
  tstates-=machine_current->timings.cycles_per_frame;
  if( event_interrupt(machine_current->timings.cycles_per_frame) ) return 1;

  if(sound_enabled) {
    sound_frame();
  } else {
    timer_sleep();
    timer_count--;
  }

  if(display_frame()) return 1;
  z80_interrupt();

  if(event_add(machine_current->timings.cycles_per_frame,
	       EVENT_TYPE_INTERRUPT) ) return 1;

  return 0;
}

BYTE readport(WORD port)
{
  spectrum_port_info *ptr;

  BYTE return_value = 0xff;

  for( ptr = machine_current->peripherals; ptr->mask; ptr++ ) {
    if( ( port & ptr->mask ) == ptr->data ) {
      return_value &= ptr->read(port);
    }
  }

  return return_value;

}

void writeport(WORD port, BYTE b)
{
  
  spectrum_port_info *ptr;

  for( ptr = machine_current->peripherals; ptr->mask; ptr++ ) {
    if( ( port & ptr->mask ) == ptr->data ) {
      ptr->write(port, b);
    }
  }

}

/* A dummy function for non-readable ports */
BYTE spectrum_port_noread(WORD port)
{
  return 0xff;
}

/* What do we get if we read from the ULA? */
BYTE spectrum_ula_read(WORD port)
{
  return keyboard_read( port >> 8 );
}

/* What happens when we write to the ULA? */
void spectrum_ula_write(WORD port, BYTE b)
{
  display_set_border( b & 0x07 );
  sound_beeper( b & 0x10 );

#ifdef ISSUE2
  if( b & 0x18 ) {
    keyboard_default_value=0xff;
  } else {
    keyboard_default_value=0xbf;
  }
#else				/* #ifdef ISSUE2 */
  if( b & 0x10 ) {
    keyboard_default_value=0xff;
  } else {
    keyboard_default_value=0xbf;
  }
#endif				/* #ifdef ISSUE2 */
}
