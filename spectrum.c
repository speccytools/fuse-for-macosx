/* spectrum.c: Generic Spectrum routines
   Copyright (c) 1999-2002 Philip Kendall, Darren Salt

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
#include "printer.h"
#include "settings.h"
#include "sound.h"
#include "spec128.h"
#include "spec48.h"
#include "specplus2.h"
#include "specplus3.h"
#include "spectrum.h"
#include "tape.h"
#include "timer.h"
#include "z80/z80.h"

BYTE **ROM;
BYTE RAM[8][0x4000];
DWORD tstates;

/* Level data from .slt files */

BYTE *slt[256];
size_t slt_length[256];

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
  printer_frame();
  z80_interrupt();

  if(event_add(machine_current->timings.cycles_per_frame,
	       EVENT_TYPE_INTERRUPT) ) return 1;

  return 0;
}

BYTE readport(WORD port)
{
  spectrum_port_info *ptr;

  BYTE return_value = 0xff;
  int attached = 0;		/* Is this port attached to anything? */

  for( ptr = machine_current->peripherals; ptr->mask; ptr++ ) {
    if( ( port & ptr->mask ) == ptr->data ) {
      return_value &= ptr->read(port); attached = 1;
    }
  }

  if( !attached ) {
    return machine_current->unattached_port();
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
  return ( keyboard_read( port >> 8 ) ^ ( tape_microphone ? 0x40 : 0x00 ) );
}

/* What happens when we write to the ULA? */
void spectrum_ula_write(WORD port, BYTE b)
{
  display_set_border( b & 0x07 );
  sound_beeper( 0, b & 0x10 );

  if( settings_current.issue2 ) {
    if( b & 0x18 ) {
      keyboard_default_value=0xff;
    } else {
      keyboard_default_value=0xbf;
    }
  } else {
    if( b & 0x10 ) {
      keyboard_default_value=0xff;
    } else {
      keyboard_default_value=0xbf;
    }
  }
}
