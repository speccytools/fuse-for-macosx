/* spec128.c: Spectrum 128K specific routines
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

#include "ay.h"
#include "config.h"
#include "display.h"
#include "event.h"
#include "keyboard.h"
#include "spec128.h"
#include "spectrum.h"
#include "z80.h"

BYTE spec128_readbyte(WORD address)
{
  WORD bank;
  if(address<0x4000) return ROM[machine.ram.current_rom][address];
  bank=address/0x4000; address-=(bank*0x4000);
  switch(bank) {
    case 1: return RAM[                       5][address]; break;
    case 2: return RAM[                       2][address]; break;
    case 3: return RAM[machine.ram.current_page][address]; break;
    default: abort();
  }
}

BYTE spec128_read_screen_memory(WORD offset)
{
  return RAM[machine.ram.current_screen][offset];
}

void spec128_writebyte(WORD address, BYTE b)
{
  int bank;

  if(address>=0x4000) {		/* 0x4000 = 1st byte of RAM */
    bank=address/0x4000; address-=(bank*0x4000);
    switch(bank) {
      case 1: bank=5;                        break;
      case 2: bank=2;                        break;
      case 3: bank=machine.ram.current_page; break;
    }
    RAM[bank][address]=b;
    if(bank==machine.ram.current_screen && address < 0x1b00) {
      display_dirty(address+0x4000,b); /* Replot necessary pixels */
    }
  }
}

int spec128_init(void)
{
  FILE *f;

  f=fopen("128-0.rom","rb");
  if(!f) return 1;
  fread(ROM[0],0x4000,1,f);

  f=freopen("128-1.rom","rb",f);
  if(!f) return 2;
  fread(ROM[1],0x4000,1,f);

  fclose(f);

  readbyte=spec128_readbyte;
  read_screen_memory=spec128_read_screen_memory;
  writebyte=spec128_writebyte;

  tstates=0;

  spectrum_set_timings(24,128,24,52,311,3.54690e6,8865);
  machine.reset=spec128_reset;

  machine.ram.type=SPECTRUM_MACHINE_128;
  machine.ram.port=0x7ffd;

  machine.ay.present=1;
  machine.ay.readport=0xfffd;
  machine.ay.writeport=0xbffd;

  return 0;

}

int spec128_reset(void)
{
  machine.ram.current_page=0; machine.ram.current_rom=0;
  machine.ram.current_screen=5;
  machine.ram.locked=0;
  event_reset();
  if(event_add(machine.cycles_per_frame,EVENT_TYPE_INTERRUPT)) return 1;
  if(event_add(machine.line_times[0],EVENT_TYPE_LINE)) return 1;
  z80_reset();
  return 0;
}
