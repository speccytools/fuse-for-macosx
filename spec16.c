/* spec16.c: Spectrum 16K specific routines
   Copyright (c) 1999-2003 Philip Kendall

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

#include <libspectrum.h>

#include "joystick.h"
#include "keyboard.h"
#include "machine.h"
#include "memory.h"
#include "printer.h"
#include "settings.h"
#include "spec16.h"
#include "spec48.h"
#include "spectrum.h"

static libspectrum_byte empty_chunk[0x2000];

spectrum_port_info spec16_peripherals[] = {
  { 0x0001, 0x0000, spectrum_ula_read, spectrum_ula_write },
  { 0x0004, 0x0000, printer_zxp_read, printer_zxp_write },
  { 0x00e0, 0x0000, joystick_kempston_read, spectrum_port_nowrite },
  { 0, 0, NULL, NULL } /* End marker. DO NOT REMOVE */
};

static libspectrum_byte
spec16_unattached_port( void )
{
  return spectrum_unattached_port( 1 );
}

int spec16_init( fuse_machine_info *machine )
{
  int error;

  machine->machine = LIBSPECTRUM_MACHINE_16;
  machine->id = "16";

  machine->reset = spec16_reset;

  error = machine_set_timings( machine ); if( error ) return error;

  machine->timex = 0;
  machine->ram.read_screen    = spec48_read_screen_memory;
  machine->ram.contend_port   = spec48_contend_port;
  machine->ram.contend_delay  = spec48_contend_delay;
  machine->ram.current_screen = 5;

  error = machine_allocate_roms( machine, 1 );
  if( error ) return error;
  machine->rom_length[0] = 0x4000;

  memset( empty_chunk, 0xff, 0x2000 );

  machine->peripherals = spec16_peripherals;
  machine->unattached_port = spec16_unattached_port;

  machine->ay.present = 0;

  machine->shutdown = NULL;

  return 0;

}

int
spec16_reset( void )
{
  int error;
  size_t i;

  error = machine_load_rom( &ROM[0], settings_current.rom_16,
			    machine_current->rom_length[0] );
  if( error ) return error;

  memory_map[0] = &ROM[0][0x0000];
  memory_map[1] = &ROM[0][0x2000];
  memory_map[2] = &RAM[5][0x0000];
  memory_map[3] = &RAM[5][0x2000];
  memory_map[4] = empty_chunk;
  memory_map[5] = empty_chunk;
  memory_map[6] = empty_chunk;
  memory_map[7] = empty_chunk;

  for( i = 0; i < 8; i++ ) memory_writable[i] = 0;
  memory_writable[2] = memory_writable[3] = 1;

  for( i = 0; i < 8; i++ ) memory_contended[i] = 0;
  memory_contended[2] = memory_contended[3] = 1;

  memory_screen_chunk1 = RAM[5];
  memory_screen_chunk2 = NULL;
  memory_screen_top = 0x1b00;

  return 0;
}
