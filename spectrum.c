/* spectrum.c: Generic Spectrum routines
   Copyright (c) 1999 Philip Kendall

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

   E-mail: pak21@cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <stdio.h>
#include <unistd.h>

#include "display.h"
#include "keyboard.h"
#include "spec128.h"
#include "spec48.h"
#include "specplus2.h"
#include "specplus3.h"
#include "spectrum.h"
#include "timer.h"
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

void spectrum_set_timings(WORD cycles_per_line,WORD lines_per_frame,DWORD hz,
			  DWORD first_line)
{
  int y;

  machine.cycles_per_line=cycles_per_line;
  machine.lines_per_frame=lines_per_frame;
  machine.cycles_per_frame=cycles_per_line*(DWORD)lines_per_frame;

  machine.hz=hz;

  for(y=0;y<192;y++) {
    first_line+=cycles_per_line;
    machine.line_times[y]=first_line;
  }
  machine.line_times[192]=0xFFFFFFFF;	/* End marker */

}

void spectrum_interrupt(void)
{
  if(tstates>=machine.cycles_per_frame) {
    tstates-=machine.cycles_per_frame;
#ifdef TIMER_HOGCPU
    if(next_delay>=machine.cycles_per_frame)
      next_delay-=machine.cycles_per_frame;
#else		/* #ifdef TIMER_HOGCPU */
    sleep(1);
#endif
    display_frame();
    z80_interrupt();
  }
}

BYTE readport(WORD port)
{
  if ( ! (port & 0x01) ) {
    return read_keyboard( ( port >> 8 ) );
  } else if ( machine.ay.present && port==machine.ay.readport ) {
    return machine.ay.registers[machine.ay.current_register];
  }
  return 0xff;
}

void writeport(WORD port,BYTE b)
{
  if( ! (port & 0x01) ) display_set_border(b&0x07);
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
      if(machine.ram.current_screen!=old_screen) display_entire_screen();
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

