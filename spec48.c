/* spec48.c: Spectrum 48K specific routines
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
#include "fuse.h"
#include "keyboard.h"
#include "machine.h"
#include "sound.h"
#include "spec48.h"
#include "spectrum.h"
#include "z80/z80.h"

spectrum_port_info spec48_peripherals[] = {
  { 0x0001, 0x0000, spectrum_ula_read, spectrum_ula_write },
  { 0, 0, NULL, NULL } /* End marker. DO NOT REMOVE */
};

BYTE spec48_readbyte(WORD address)
{
  WORD bank;

  if(address<0x4000) return ROM[0][address];
  bank=address/0x4000; address-=(bank*0x4000);
  switch(bank) {
    case 1: return RAM[5][address]; break;
    case 2: return RAM[2][address]; break;
    case 3: return RAM[0][address]; break;
    default:
      fprintf(stderr, "%s: access to impossible bank %d at %s:%d\n",
	      fuse_progname, bank, __FILE__, __LINE__);
      abort();
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
    default:
      fprintf(stderr, "%s: access to impossible bank %d at %s:%d\n",
	      fuse_progname, bank, __FILE__, __LINE__);
      abort();
    }
    if(address<0x5b00) {	/* 0x4000 - 0x5aff = display file */
      display_dirty(address,b);	/* Replot necessary pixels */
    }
  }
}

int spec48_init( machine_info *machine )
{
  int error;

  machine->machine = SPECTRUM_MACHINE_48;

  machine->reset = spec48_reset;

  machine_set_timings( machine, 3.5e6, 24, 128, 24, 48, 312, 8936 );

  machine->ram.read_memory  = spec48_readbyte;
  machine->ram.read_screen  = spec48_read_screen_memory;
  machine->ram.write_memory = spec48_writebyte;

  error = machine_allocate_roms( machine, 1 );
  if( error ) return error;
  error = machine_read_rom( machine, 0, "48.rom" );
  if( error ) return error;

  machine->peripherals = spec48_peripherals;

  machine->ay.present = 0;

  return 0;

}

int spec48_reset(void)
{
  z80_reset();
  sound_ay_reset();	/* should happen for *all* resets */

  return 0;
}
