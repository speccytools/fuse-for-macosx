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

#include "ay.h"
#include "display.h"
#include "fuse.h"
#include "joystick.h"
#include "keyboard.h"
#include "machine.h"
#include "settings.h"
#include "snapshot.h"
#include "sound.h"
#include "pentagon.h"
#include "spectrum.h"
#include "trdos.h"
#include "z80/z80.h"

static BYTE pentagon_select_1f_read( WORD port );
static BYTE pentagon_select_ff_read( WORD port );
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

BYTE
pentagon_select_1f_read( WORD port )
{
  if( trdos_active ) {
    return trdos_sr_read( port );
  } else {
    return joystick_kempston_read( port );
  }
}

BYTE
pentagon_select_ff_read( WORD port )
{
  if( trdos_active ) {
    return trdos_sp_read( port );
  } else {
    return pentagon_unattached_port();
  }
}

BYTE
pentagon_unattached_port( void )
{
  return spectrum_unattached_port( 3 );
}

BYTE
pentagon_read_screen_memory(WORD offset)
{
  return RAM[machine_current->ram.current_screen][offset];
}

DWORD
pentagon_contend_memory( WORD address GCC_UNUSED )
{
  /* No contended memory */

  return 0;
}

DWORD
pentagon_contend_port( WORD port GCC_UNUSED )
{
  /* No contention on ports AFAIK */

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
  machine->ram.read_memory    = pentagon_readbyte;
  machine->ram.read_memory_internal  = pentagon_readbyte_internal;
  machine->ram.read_screen    = pentagon_read_screen_memory;
  machine->ram.write_memory   = pentagon_writebyte;
  machine->ram.write_memory_internal  = pentagon_writebyte_internal;
  machine->ram.contend_memory = pentagon_contend_memory;
  machine->ram.contend_port   = pentagon_contend_port;

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

  machine_current->ram.locked=0;
  machine_current->ram.current_page=0;
  machine_current->ram.current_rom=0;
  machine_current->ram.current_screen=5;

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

  return 0;
}

void
pentagon_memoryport_write( WORD port GCC_UNUSED, BYTE b )
{
  int old_screen;

  /* Do nothing if we've locked the RAM configuration */
  if(machine_current->ram.locked) return;
    
  old_screen=machine_current->ram.current_screen;

  /* Store the last byte written in case we need it */
  machine_current->ram.last_byte=b;

  /* Work out which page, screen and ROM are selected */
  machine_current->ram.current_page= b & 0x07;
  machine_current->ram.current_screen=( b & 0x08 ) ? 7 : 5;
  machine_current->ram.current_rom=(machine_current->ram.current_rom & 0x02) |
    ( (b & 0x10) >> 4 );

  /* Are we locking the RAM configuration? */
  machine_current->ram.locked=( b & 0x20 );

  /* If we changed the active screen, mark the entire display file as
     dirty so we redraw it on the next pass */
  if(machine_current->ram.current_screen!=old_screen) display_refresh_all();
}

static int
pentagon_shutdown( void )
{
  trdos_end();

  return 0;
}
