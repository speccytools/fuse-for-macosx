/* spec48.c: Spectrum 48K specific routines
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

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "display.h"
#include "event.h"
#include "keyboard.h"
#include "spec48.h"
#include "spectrum.h"
#include "z80.h"

BYTE spec48_readbyte(WORD address)
{
  WORD bank;
  if(address<0x4000) return ROM[0][address];
  bank=address/0x4000; address-=(bank*0x4000);
  switch(bank) {
    case 1: return RAM[5][address]; break;
    case 2: return RAM[2][address]; break;
    case 3: return RAM[0][address]; break;
    default: abort();
  }
}

BYTE spec48_read_screen_memory(WORD offset)
{
  return RAM[5][offset];
}

void spec48_writebyte(WORD address, BYTE b)
{
  if(address>=0x4000) {		/* 0x4000 = 1st byte of RAM */
    WORD bank=address/0x4000,offset=address-(bank*0x4000);
    switch(bank) {
      case 1: RAM[5][offset]=b; break;
      case 2: RAM[2][offset]=b; break;
      case 3: RAM[0][offset]=b; break;
      default: abort();
    }
    if(address<0x5b00) {	/* 0x4000 - 0x5aff = display file */
      display_dirty(address,b);	/* Replot necessary pixels */
    }
  }
}

int spec48_init(void)
{
  FILE *f;

  f=fopen("48.rom","rb");
  if(!f) return 1;
  fread(ROM[0],0x4000,1,f);
  fclose(f);

  readbyte=spec48_readbyte;
  read_screen_memory=spec48_read_screen_memory;
  writebyte=spec48_writebyte;

  tstates=0;

  spectrum_set_timings(24,128,24,48,312,3.5e6,8936);
  machine.reset=spec48_reset;

  machine.ram.type=SPECTRUM_MACHINE_48;
  machine.ay.present=0;

  return 0;

}

int spec48_reset(void)
{
  event_reset();
  if(event_add(machine.cycles_per_frame,EVENT_TYPE_INTERRUPT)) return 1;
  if(event_add(machine.line_times[0],EVENT_TYPE_LINE)) return 1;
  z80_reset();
  return 0;
}
