/* pentagon.c: Pentagon 128K specific routines
   Copyright (c) 1999-2004 Philip Kendall and Fredrick Meunier

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

#include <libspectrum.h>

#include "compat.h"
#include "joystick.h"
#include "machine.h"
#include "memory.h"
#include "periph.h"
#include "settings.h"
#include "spec128.h"
#include "trdos.h"

static libspectrum_byte pentagon_select_1f_read( libspectrum_word port,
						 int *attached );
static libspectrum_byte pentagon_select_ff_read( libspectrum_word port,
						 int *attached );
static libspectrum_byte pentagon_contend_delay( libspectrum_dword time );
static void pentagon_memoryport_write( libspectrum_word port,
				       libspectrum_byte b );
static int pentagon_reset( void );
static int pentagon_shutdown( void );

const static periph_t peripherals[] = {
  { 0x00ff, 0x001f, pentagon_select_1f_read, trdos_cr_write },
  { 0x00ff, 0x003f, trdos_tr_read, trdos_tr_write },
  { 0x00ff, 0x005f, trdos_sec_read, trdos_sec_write },
  { 0x00ff, 0x007f, trdos_dr_read, trdos_dr_write },
  { 0x00ff, 0x00fe, spectrum_ula_read, spectrum_ula_write },
  { 0x00ff, 0x00ff, pentagon_select_ff_read, trdos_sp_write },
  { 0xc002, 0xc000, ay_registerport_read, ay_registerport_write },
  { 0xc002, 0x8000, NULL, ay_dataport_write },
  { 0x8002, 0x0000, NULL, pentagon_memoryport_write },
};

const static size_t peripherals_count =
  sizeof( peripherals ) / sizeof( periph_t );

static libspectrum_byte
pentagon_select_1f_read( libspectrum_word port, int *attached )
{
  libspectrum_byte data;

  data = trdos_sr_read( port, attached ); if( *attached ) return data;
  data = joystick_kempston_read( port, attached ); if( *attached ) return data;

  return 0xff;
}

static libspectrum_byte
pentagon_select_ff_read( libspectrum_word port, int *attached )
{
  libspectrum_byte data;

  data = trdos_sp_read( port, attached ); if( *attached ) return data;

  return 0xff;
}

static libspectrum_byte
pentagon_unattached_port( void )
{
  return spectrum_unattached_port( 3 );
}

static libspectrum_dword
pentagon_contend_port( libspectrum_word port GCC_UNUSED )
{
  /* No contention on ports AFAIK */

  return 0;
}

static libspectrum_byte
pentagon_contend_delay( libspectrum_dword time GCC_UNUSED )
{
  /* No contention */
  return 0;
}

int
pentagon_init( fuse_machine_info *machine )
{
  int error;

  machine->machine = LIBSPECTRUM_MACHINE_PENT;
  machine->id = "pentagon";

  machine->reset = pentagon_reset;

  machine->timex = 0;
  machine->ram.contend_port   = pentagon_contend_port;
  machine->ram.contend_delay  = pentagon_contend_delay;

  error = machine_allocate_roms( machine, 3 );
  if( error ) return error;
  machine->rom_length[0] = machine->rom_length[1] = 
    machine->rom_length[2] = 0x4000;

  machine->unattached_port = pentagon_unattached_port;

  machine->shutdown = pentagon_shutdown;

  return 0;

}

static int
pentagon_reset(void)
{
  int error;

  trdos_reset();

  error = machine_load_rom( &ROM[0], settings_current.rom_pentagon_0,
			    machine_current->rom_length[0] );
  if( error ) return error;
  error = machine_load_rom( &ROM[1], settings_current.rom_pentagon_1,
			    machine_current->rom_length[1] );
  if( error ) return error;
  error = machine_load_rom( &ROM[2], settings_current.rom_pentagon_2,
			    machine_current->rom_length[2] );
  if( error ) return error;

  trdos_available = 1;

  error = periph_setup( peripherals, peripherals_count,
			PERIPH_PRESENT_OPTIONAL );
  if( error ) return error;

  return spec128_common_reset( 0 );
}

void
pentagon_memoryport_write( libspectrum_word port GCC_UNUSED,
			   libspectrum_byte b )
{
  int page, rom, screen;

  if( machine_current->ram.locked ) return;
    
  page = b & 0x07;
  screen = ( b & 0x08 ) ? 7 : 5;
  rom = ( b & 0x10 ) >> 4;

  memory_map[0].page = &ROM[ rom ][0x0000];
  memory_map[1].page = &ROM[ rom ][0x2000];

  memory_map[6].page = &RAM[ page ][0x0000];
  memory_map[7].page = &RAM[ page ][0x2000];
  memory_map[6].reverse = memory_map[7].reverse = page;

  /* If we changed the active screen, mark the entire display file as
     dirty so we redraw it on the next pass */
  if( memory_current_screen != screen ) {
    display_refresh_all();
    memory_current_screen = screen;
  }

  machine_current->ram.current_page = page;
  machine_current->ram.current_rom = rom;
  machine_current->ram.locked = ( b & 0x20 );

  machine_current->ram.last_byte = b;
}

static int
pentagon_shutdown( void )
{
  trdos_end();

  return 0;
}
