/* specplus3.c: Spectrum +2A/+3 specific routines
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

#include "ay.h"
#include "display.h"
#include "fuse.h"
#include "joystick.h"
#include "keyboard.h"
#include "machine.h"
#include "sound.h"
#include "spec128.h"
#include "specplus3.h"
#include "spectrum.h"
#include "z80/z80.h"

spectrum_port_info specplus3_peripherals[] = {
  { 0x0001, 0x0000, spectrum_ula_read, spectrum_ula_write },
  { 0x00e0, 0x0000, joystick_kempston_read, joystick_kempston_write },
  { 0xc002, 0xc000, ay_registerport_read, ay_registerport_write },
  { 0xc002, 0x8000, spectrum_port_noread, ay_dataport_write },
  { 0xc002, 0x4000, spectrum_port_noread, spec128_memoryport_write },
  { 0xf002, 0x1000, spectrum_port_noread, specplus3_memoryport_write },
  { 0, 0, NULL, NULL } /* End marker. DO NOT REMOVE */
};

BYTE specplus3_readbyte(WORD address)
{
  int bank;

  if(machine_current->ram.special) {
    bank=address/0x4000; address-=(bank*0x4000);
    switch(machine_current->ram.specialcfg) {
      case 0: return RAM[bank  ][address];
      case 1: return RAM[bank+4][address];
      case 2: switch(bank) {
	case 0: return RAM[4][address];
	case 1: return RAM[5][address];
	case 2: return RAM[6][address];
	case 3: return RAM[3][address];
      }
      case 3: switch(bank) {
	case 0: return RAM[4][address];
	case 1: return RAM[7][address];
	case 2: return RAM[6][address];
	case 3: return RAM[3][address];
      }
      default:
	fprintf(stderr,"Unknown +3 special configuration %d\n",
		machine_current->ram.specialcfg);
	fuse_abort();
    }
  } else {
    if(address<0x4000) return ROM[machine_current->ram.current_rom][address];
    bank=address/0x4000; address-=(bank*0x4000);
    switch(bank) {
      case 1: return RAM[				 5][address]; break;
      case 2: return RAM[				 2][address]; break;
      case 3: return RAM[machine_current->ram.current_page][address]; break;
      default: fuse_abort();
    }
  }

  fuse_abort(); /* To keep gcc quiet; should never be reached */
}

BYTE specplus3_read_screen_memory(WORD offset)
{
  return RAM[machine_current->ram.current_screen][offset];
}

void specplus3_writebyte(WORD address, BYTE b)
{
  int bank;

  if(machine_current->ram.special) {
    bank=address/0x4000; address-=(bank*0x4000);
    switch(machine_current->ram.specialcfg) {
      case 0: break;
      case 1: bank+=4; break;
      case 2:
	switch(bank) {
	  case 0: bank=4; break;
	  case 1: bank=5; break;
	  case 2: bank=6; break;
	  case 3: bank=3; break;
	}
	break;
      case 3: switch(bank) {
	case 0: bank=4; break;
	case 1: bank=7; break;
	case 2: bank=6; break;
	case 3: bank=3; break;
      }
      break;
    }
  } else {
    if(address<0x4000) return;
    bank=address/0x4000; address-=(bank*0x4000);
    switch(bank) {
      case 1: bank=5;				      break;
      case 2: bank=2;				      break;
      case 3: bank=machine_current->ram.current_page; break;
    }
  }
  RAM[bank][address]=b;
  if(bank==machine_current->ram.current_screen && address < 0x1b00) {
    display_dirty(address+0x4000,b); /* Replot necessary pixels */
  }
}

int specplus3_init( machine_info *machine )
{
  int error;

  machine->machine = SPECTRUM_MACHINE_PLUS3;

  machine->reset = specplus3_reset;

  machine_set_timings( machine, 3.54690e6, 24, 128, 24, 52, 311, 8865 );

  machine->ram.read_memory = specplus3_readbyte;
  machine->ram.read_screen = specplus3_read_screen_memory;
  machine->ram.write_memory = specplus3_writebyte;

  error = machine_allocate_roms( machine, 4 );
  if( error ) return error;
  error = machine_read_rom( machine, 0, "plus3-0.rom" );
  if( error ) return error;
  error = machine_read_rom( machine, 1, "plus3-1.rom" );
  if( error ) return error;
  error = machine_read_rom( machine, 2, "plus3-2.rom" );
  if( error ) return error;
  error = machine_read_rom( machine, 3, "plus3-3.rom" );
  if( error ) return error;

  machine->peripherals=specplus3_peripherals;

  machine->ay.present=1;

  return 0;

}

int specplus3_reset(void)
{
  machine_current->ram.current_page=0; machine_current->ram.current_rom=0;
  machine_current->ram.current_screen=5;
  machine_current->ram.locked=0;
  machine_current->ram.special=0; machine_current->ram.specialcfg=0;

  z80_reset();
  sound_ay_reset();

  return 0;
}

void specplus3_memoryport_write(WORD port, BYTE b)
{

  /* Do nothing if we've locked the RAM configuration */
  if( machine_current->ram.locked ) return;

  /* Store the last byte written in case we need it */
  machine_current->ram.last_byte2=b;

  if( b & 0x01) {	/* Check whether we want a special RAM configuration */

    /* If so, select it */
    machine_current->ram.special=1;
    machine_current->ram.specialcfg= ( b & 0x06 ) >> 1;

  } else {

    /* If not, we're selecting the high bit of the current ROM */
    machine_current->ram.special=0;
    machine_current->ram.current_rom=(machine_current->ram.current_rom & 0x01) |
      ( (b & 0x04) >> 1 );

  }

}


