/* spectrum.c: Generic Spectrum routines
   Copyright (c) 1999-2000 Philip Kendall

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

   E-mail: pak@ast.cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <stdio.h>

#include "display.h"
#include "event.h"
#include "keyboard.h"
#include "spec128.h"
#include "spec48.h"
#include "specplus2.h"
#include "specplus3.h"
#include "spectrum.h"
#include "timer.h"
#include "x.h"
#include "z80.h"

BYTE ROM[4][0x4000];
BYTE RAM[8][0x4000];
DWORD tstates;
machine_info machine;

int spectrum_init()
{
  switch(machine.machine) {
    case SPECTRUM_MACHINE_48:
      return spec48_init();
    case SPECTRUM_MACHINE_128:
      return spec128_init();
    case SPECTRUM_MACHINE_PLUS2:
      return specplus2_init();
    case SPECTRUM_MACHINE_PLUS3:
      return specplus3_init();
    default:
      fprintf(stderr,"Unknown machine: %d\n",machine.machine);
      abort();
  }
}

void spectrum_set_timings(WORD left_border_cycles,  WORD screen_cycles,
			  WORD right_border_cycles, WORD retrace_cycles,
			  WORD lines_per_frame, DWORD hz, DWORD first_line)
{
  int y;

  machine.left_border_cycles =left_border_cycles;
  machine.screen_cycles      =screen_cycles;
  machine.right_border_cycles=right_border_cycles;
  machine.retrace_cycles     =retrace_cycles;

  machine.cycles_per_line=left_border_cycles+screen_cycles+
    right_border_cycles+retrace_cycles;

  machine.lines_per_frame=lines_per_frame;
  machine.cycles_per_frame=machine.cycles_per_line*(DWORD)lines_per_frame;

  machine.hz=hz;

  machine.line_times[0]=first_line;
  for(y=1;y<DISPLAY_SCREEN_HEIGHT+1;y++) {
    machine.line_times[y]=machine.line_times[y-1]+machine.cycles_per_line;
  }

}

int spectrum_interrupt(void)
{
  tstates-=machine.cycles_per_frame;
  if(event_interrupt(machine.cycles_per_frame)) return 1;

  timer_sleep(); timer_count--;
  if(display_frame()) return 1;
  z80_interrupt();

  if(event_add(machine.cycles_per_frame,EVENT_TYPE_INTERRUPT)) return 1;

  return 0;
}

BYTE readport(WORD port)
{
  if ( ! (port & 0x01) ) {
    return keyboard_read( ( port >> 8 ) );
  } else if ( machine.ay.present && port==machine.ay.readport ) {
    return machine.ay.registers[machine.ay.current_register];
  }
  return 0xff;
}

void writeport(WORD port,BYTE b)
{
  if( ! (port & 0x01) ) {
    display_set_border(b&0x07);

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

  if( port==machine.ram.port && 
      ( machine.ram.type==SPECTRUM_MACHINE_128 || 
	machine.ram.type==SPECTRUM_MACHINE_PLUS3 ) &&
      !(machine.ram.locked) )
    {
      int old_screen=machine.ram.current_screen;
      machine.ram.last_byte=b;
      machine.ram.current_page=b&0x07;
      machine.ram.current_screen=( b & 0x08) ? 7 : 5;
      machine.ram.current_rom=(machine.ram.current_rom & 0x02) |
	( (b & 0x10) >> 4 );
      machine.ram.locked=( b & 0x20 );
      if(machine.ram.current_screen!=old_screen) display_refresh_all();
    }
  if( port==machine.ram.port2 &&
      machine.ram.type==SPECTRUM_MACHINE_PLUS3 &&
      !(machine.ram.locked) )
    {
      machine.ram.last_byte2=b;
      if(b&0x01) {
	machine.ram.special=1;
	machine.ram.specialcfg=(b&0x06)>>1;
      } else {
	machine.ram.special=0;
	machine.ram.current_rom=(machine.ram.current_rom & 0x01) |
	  ( (b & 0x04) >> 1 );
      }
    }
  if(machine.ay.present) {
    if( port==machine.ay.readport && 0<b<15) {
      machine.ay.current_register=b;
    }
    if( port==machine.ay.writeport ) {
      machine.ay.registers[machine.ay.current_register]=b;
    }
  }
}
