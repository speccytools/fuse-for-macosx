/* spec128.c: Spectrum 128K specific routines
   Copyright (c) 1999-2007 Philip Kendall

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <stdio.h>

#include <libspectrum.h>

#include "ay.h"
#include "compat.h"
#include "disk/beta.h"
#include "joystick.h"
#include "machine.h"
#include "memory.h"
#include "periph.h"
#include "settings.h"
#include "spec128.h"
#include "spec48.h"
#include "ula.h"
#include "if1.h"

static int spec128_reset( void );

const periph_t spec128_peripherals[] = {
  { 0x0001, 0x0000, ula_read, ula_write },
  { 0x0018, 0x0010, if1_port_in, if1_port_out },
  { 0x0018, 0x0008, if1_port_in, if1_port_out },
  { 0x0018, 0x0000, if1_port_in, if1_port_out },
  { 0x00e0, 0x0000, joystick_kempston_read, NULL },
  { 0xc002, 0xc000, ay_registerport_read, ay_registerport_write },
  { 0xc002, 0x8000, NULL, ay_dataport_write },
  { 0x8002, 0x0000, NULL, spec128_memoryport_write },
};

const size_t spec128_peripherals_count =
  sizeof( spec128_peripherals ) / sizeof( periph_t );

libspectrum_byte
spec128_unattached_port( void )
{
  return spectrum_unattached_port();
}

libspectrum_byte
spec128_contend_delay( libspectrum_dword time )
{
  libspectrum_word tstates_through_line;
  
  /* No contention in the upper border */
  if( time < machine_current->line_times[ DISPLAY_BORDER_HEIGHT ] )
    return 0;

  /* Or the lower border */
  if( time >= machine_current->line_times[ DISPLAY_BORDER_HEIGHT + 
					   DISPLAY_HEIGHT          ] )
    return 0;

  /* Work out where we are in this line */
  tstates_through_line =
    ( time + machine_current->timings.left_border ) %
    machine_current->timings.tstates_per_line;

  /* No contention if we're in the left border */
  if( tstates_through_line < machine_current->timings.left_border - 3 ) 
    return 0;

  /* Or the right border or retrace */
  if( tstates_through_line >= machine_current->timings.left_border +
                              machine_current->timings.horizontal_screen - 3 )
    return 0;

  /* We now know the ULA is reading the screen, so put in the appropriate
     delay */
  switch( tstates_through_line % 8 ) {
    case 5: return 6; break;
    case 6: return 5; break;
    case 7: return 4; break;
    case 0: return 3; break;
    case 1: return 2; break;
    case 2: return 1; break;
    case 3: return 0; break;
    case 4: return 0; break;
  }

  return 0;	/* Shut gcc up */
}

int spec128_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_128;
  machine->id = "128";

  machine->reset = spec128_reset;

  machine->timex = 0;
  machine->ram.port_contended	     = spec48_port_contended;
  machine->ram.contend_delay	     = spec128_contend_delay;
  machine->ram.contend_delay_no_mreq = spec128_contend_delay;

  machine->unattached_port = spec128_unattached_port;

  machine->shutdown = NULL;

  machine->memory_map = spec128_memory_map;

  return 0;

}

static int
spec128_reset( void )
{
  int error;

  error = machine_load_rom( 0, 0, settings_current.rom_128_0,
                            settings_default.rom_128_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 2, 1, settings_current.rom_128_1,
                            settings_default.rom_128_1, 0x4000 );
  if( error ) return error;

  error = periph_setup( spec128_peripherals, spec128_peripherals_count );
  if( error ) return error;

  error = spec128_common_reset( 1 );
  if( error ) return error;

  periph_setup_kempston( PERIPH_PRESENT_OPTIONAL );
  periph_setup_interface1( PERIPH_PRESENT_OPTIONAL );
  periph_setup_interface2( PERIPH_PRESENT_OPTIONAL );
  periph_setup_plusd( PERIPH_PRESENT_OPTIONAL );
  periph_update();

  return 0;
}

int
spec128_common_reset( int contention )
{
  size_t i;

  machine_current->ram.locked=0;
  machine_current->ram.last_byte = 0;

  machine_current->ram.current_page=0;
  machine_current->ram.current_rom=0;

  memory_current_screen = 5;
  memory_screen_mask = 0xffff;

  /* ROM 0, RAM 5, RAM 2, RAM 0 */
  memory_map_home[0] = &memory_map_rom[ 0];
  memory_map_home[1] = &memory_map_rom[ 1];

  memory_map_home[2] = &memory_map_ram[10];
  memory_map_home[3] = &memory_map_ram[11];

  memory_map_home[4] = &memory_map_ram[ 4];
  memory_map_home[5] = &memory_map_ram[ 5];

  memory_map_home[6] = &memory_map_ram[ 0];
  memory_map_home[7] = &memory_map_ram[ 1];

  /* Mark as present/writeable */
  for( i = 0; i < 16; i++ )
    memory_map_ram[i].writable = 1;

  /* Odd pages contended on the 128K/+2; the loop is up to 16 to
     ensure all of the Scorpion's 256Kb RAM is not contended */
  for( i = 0; i < 16; i++ )
    memory_map_ram[ 2 * i     ].contended =
      memory_map_ram[ 2 * i + 1 ].contended = i & 1 ? contention : 0;

  for( i = 0; i < 8; i++ )
    memory_map_read[i] = memory_map_write[i] = *memory_map_home[i];

  return 0;
}

void
spec128_memoryport_write( libspectrum_word port GCC_UNUSED,
			  libspectrum_byte b )
{
  if( machine_current->ram.locked ) return;

  machine_current->ram.last_byte = b;

  machine_current->memory_map();

  machine_current->ram.locked = b & 0x20;
}

void
spec128_select_rom( int rom )
{
  memory_map_home[0] = &memory_map_rom[ 2 * rom     ];
  memory_map_home[1] = &memory_map_rom[ 2 * rom + 1 ];
  machine_current->ram.current_rom = rom;
}

void
spec128_select_page( int page )
{
  memory_map_home[6] = &memory_map_ram[ 2 * page     ];
  memory_map_home[7] = &memory_map_ram[ 2 * page + 1 ];
  machine_current->ram.current_page = page;
}

int
spec128_memory_map( void )
{
  int page, screen, rom;
  size_t i;

  page = machine_current->ram.last_byte & 0x07;
  screen = ( machine_current->ram.last_byte & 0x08 ) ? 7 : 5;
  rom = ( machine_current->ram.last_byte & 0x10 ) >> 4;

  /* If we changed the active screen, mark the entire display file as
     dirty so we redraw it on the next pass */
  if( memory_current_screen != screen ) {
    display_update_critical( 0, 0 );
    display_refresh_main_screen();
    memory_current_screen = screen;
  }

  spec128_select_rom( rom );
  spec128_select_page( page );

  for( i = 0; i < 8; i++ )
    memory_map_read[i] = memory_map_write[i] = *memory_map_home[i];

  memory_romcs_map();

  return 0;
}
