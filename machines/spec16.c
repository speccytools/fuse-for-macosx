/* spec16.c: Spectrum 16K specific routines
   Copyright (c) 1999-2004 Philip Kendall

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
#include <string.h>

#include <libspectrum.h>

#include "joystick.h"
#include "machine.h"
#include "machines.h"
#include "memory.h"
#include "periph.h"
#include "printer.h"
#include "settings.h"
#include "spec48.h"

static int spec16_reset( void );

static libspectrum_byte empty_chunk[0x2000];

const static periph_t peripherals[] = {
  { 0x0001, 0x0000, spectrum_ula_read, spectrum_ula_write },
  { 0x0004, 0x0000, printer_zxp_read, printer_zxp_write },
  { 0x00e0, 0x0000, joystick_kempston_read, NULL },
};

const static size_t peripherals_count =
  sizeof( peripherals ) / sizeof( periph_t );

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

  machine->timex = 0;
  machine->ram.contend_port   = spec48_contend_port;
  machine->ram.contend_delay  = spec48_contend_delay;

  error = machine_allocate_roms( machine, 1 );
  if( error ) return error;
  machine->rom_length[0] = 0x4000;

  memset( empty_chunk, 0xff, 0x2000 );

  machine->unattached_port = spec16_unattached_port;

  machine->shutdown = NULL;

  return 0;

}

static int
spec16_reset( void )
{
  int error;
  size_t i;

  error = machine_load_rom( &ROM[0], settings_current.rom_16,
			    machine_current->rom_length[0] );
  if( error ) return error;

  error = periph_setup( peripherals, peripherals_count,
			PERIPH_PRESENT_OPTIONAL );
  if( error ) return error;

  memory_map[0].page = &ROM[0][0x0000];
  memory_map[1].page = &ROM[0][0x2000];
  memory_map[0].reverse = memory_map[1].reverse = -1;

  memory_map[2].page = &RAM[5][0x0000];
  memory_map[3].page = &RAM[5][0x2000];
  memory_map[2].reverse = memory_map[3].reverse = 5;

  memory_map[4].page = empty_chunk;
  memory_map[5].page = empty_chunk;
  memory_map[6].page = empty_chunk;
  memory_map[7].page = empty_chunk;
  memory_map[4].reverse = memory_map[5].reverse =
    memory_map[6].reverse = memory_map[7].reverse = -1;

  for( i = 0; i < 8; i++ ) memory_map[i].offset = ( i & 1 ? 0x2000 : 0x0000 );

  for( i = 0; i < 8; i++ ) memory_map[i].writable = 0;
  memory_map[2].writable = memory_map[3].writable = 1;

  for( i = 0; i < 8; i++ ) memory_map[i].contended = 0;
  memory_map[2].contended = memory_map[3].contended = 1;

  memory_current_screen = 5;
  memory_screen_mask = 0xffff;

  return 0;
}
