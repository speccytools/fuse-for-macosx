/* pentagon.c: Pentagon 128K specific routines
   Copyright (c) 1999-2003 Philip Kendall and Fredrick Meunier

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
#include <unistd.h>

#include <libspectrum.h>

#include "ay.h"
#include "compat.h"
#include "joystick.h"
#include "memory.h"
#include "settings.h"
#include "pentagon.h"
#include "spec128.h"
#include "spectrum.h"
#include "trdos.h"

static libspectrum_byte pentagon_select_1f_read( libspectrum_word port );
static libspectrum_byte pentagon_select_ff_read( libspectrum_word port );
static libspectrum_byte pentagon_contend_delay( libspectrum_dword time );
static int pentagon_shutdown( void );

spectrum_port_info pentagon_peripherals[] = {
  { 0x00ff, 0x001f, pentagon_select_1f_read, trdos_cr_write },
  { 0x00ff, 0x003f, trdos_tr_read, trdos_tr_write },
  { 0x00ff, 0x005f, trdos_sec_read, trdos_sec_write },
  { 0x00ff, 0x007f, trdos_dr_read, trdos_dr_write },
  { 0x00ff, 0x00fe, spectrum_ula_read, spectrum_ula_write },
  { 0x00ff, 0x00ff, pentagon_select_ff_read, trdos_sp_write },
  { 0xc002, 0xc000, ay_registerport_read, ay_registerport_write },
  { 0xc002, 0x8000, spectrum_port_noread, ay_dataport_write },
  { 0x8002, 0x0000, spectrum_port_noread, pentagon_memoryport_write },
  { 0, 0, NULL, NULL } /* End marker. DO NOT REMOVE */
};

libspectrum_byte
pentagon_select_1f_read( libspectrum_word port )
{
  if( trdos_active ) {
    return trdos_sr_read( port );
  } else {
    return joystick_kempston_read( port );
  }
}

libspectrum_byte
pentagon_select_ff_read( libspectrum_word port )
{
  if( trdos_active ) {
    return trdos_sp_read( port );
  } else {
    return pentagon_unattached_port();
  }
}

libspectrum_byte
pentagon_unattached_port( void )
{
  return spectrum_unattached_port( 3 );
}

libspectrum_byte
pentagon_read_screen_memory( libspectrum_word offset )
{
  return RAM[machine_current->ram.current_screen][offset];
}

libspectrum_dword
pentagon_contend_port( libspectrum_word port GCC_UNUSED )
{
  /* No contention on ports AFAIK */

  return 0;
}

static libspectrum_byte
pentagon_contend_delay( libspectrum_dword time )
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

  error = machine_set_timings( machine ); if( error ) return error;

  machine->timex = 0;
  machine->ram.read_screen    = pentagon_read_screen_memory;
  machine->ram.contend_port   = pentagon_contend_port;
  machine->ram.contend_delay  = pentagon_contend_delay;

  error = machine_allocate_roms( machine, 3 );
  if( error ) return error;
  machine->rom_length[0] = machine->rom_length[1] = 
    machine->rom_length[2] = 0x4000;

  machine->peripherals = pentagon_peripherals;
  machine->unattached_port = pentagon_unattached_port;

  machine->ay.present = 1;

  machine->shutdown = pentagon_shutdown;

  return 0;

}

int
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

  memory_screen_chunk1 = RAM[ screen ];

  /* If we changed the active screen, mark the entire display file as
     dirty so we redraw it on the next pass */
  if( screen != machine_current->ram.current_screen )
    display_refresh_all();

  machine_current->ram.current_page = page;
  machine_current->ram.current_rom = rom;
  machine_current->ram.current_screen = screen;
  machine_current->ram.locked = ( b & 0x20 );

  machine_current->ram.last_byte = b;
}

static int
pentagon_shutdown( void )
{
  trdos_end();

  return 0;
}
