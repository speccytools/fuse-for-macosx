/* spec128.c: Spectrum 128K specific routines
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

#include <libspectrum.h>

#include "ay.h"
#include "compat.h"
#include "joystick.h"
#include "machine.h"
#include "memory.h"
#include "periph.h"
#include "settings.h"
#include "spec128.h"

static int spec128_reset( void );

const periph_t spec128_peripherals[] = {
  { 0x0001, 0x0000, spectrum_ula_read, spectrum_ula_write },
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
  return spectrum_unattached_port( 3 );
}

libspectrum_dword
spec128_contend_port( libspectrum_word port )
{
  /* Contention occurs for the ULA, or for the memory paging port */
  if( ( port & 0x0001 ) == 0x0000 ||
      ( port & 0x8002 ) == 0x0000    ) return spec128_contend_delay( tstates );

  return 0;
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
  int error;

  machine->machine = LIBSPECTRUM_MACHINE_128;
  machine->id = "128";

  machine->reset = spec128_reset;

  machine->timex = 0;
  machine->ram.contend_port	     = spec128_contend_port;
  machine->ram.contend_delay	     = spec128_contend_delay;

  error = machine_allocate_roms( machine, 2 );
  if( error ) return error;
  machine->rom_length[0] = machine->rom_length[1] = 0x4000;

  machine->unattached_port = spec128_unattached_port;

  machine->shutdown = NULL;

  return 0;

}

static int
spec128_reset( void )
{
  int error;

  error = machine_load_rom( &ROM[0], settings_current.rom_128_0,
			    machine_current->rom_length[0] );
  if( error ) return error;
  error = machine_load_rom( &ROM[1], settings_current.rom_128_1,
			    machine_current->rom_length[1] );
  if( error ) return error;

  error = periph_setup( spec128_peripherals, spec128_peripherals_count,
			PERIPH_PRESENT_OPTIONAL );
  if( error ) return error;

  return spec128_common_reset( 1 );
}

int
spec128_common_reset( int contention )
{
  size_t i;

  machine_current->ram.locked=0;
  machine_current->ram.last_byte = 0;

  machine_current->ram.current_page=0;
  machine_current->ram.current_rom=0;

  memory_map[0].page = &ROM[0][0x0000];
  memory_map[1].page = &ROM[0][0x2000];
  memory_map[0].reverse = memory_map[1].reverse = MEMORY_PAGE_OFFSET_ROM;

  memory_map[2].page = &RAM[5][0x0000];
  memory_map[3].page = &RAM[5][0x2000];
  memory_map[2].reverse = memory_map[3].reverse = 5;

  memory_map[4].page = &RAM[2][0x0000];
  memory_map[5].page = &RAM[2][0x2000];
  memory_map[4].reverse = memory_map[5].reverse = 2;

  memory_map[6].page = &RAM[0][0x0000];
  memory_map[7].page = &RAM[0][0x2000];
  memory_map[6].reverse = memory_map[7].reverse = 0;

  for( i = 0; i < 8; i++ ) memory_map[i].offset = ( i & 1 ? 0x2000 : 0x0000 );

  memory_map[0].writable = memory_map[1].writable = 0;
  for( i = 2; i < 8; i++ ) memory_map[i].writable = 1;

  for( i = 0; i < 8; i++ ) memory_map[i].contended = 0;

  if( contention ) {
    memory_map[2].contended = memory_map[3].contended = 1;
  }

  memory_current_screen = 5;
  memory_screen_mask = 0xffff;

  return 0;
}

void
spec128_memoryport_write( libspectrum_word port GCC_UNUSED,
			  libspectrum_byte b )
{
  int page, rom, screen;

  if( machine_current->ram.locked ) return;
    
  page = b & 0x07;
  screen = ( b & 0x08 ) ? 7 : 5;
  rom = ( b & 0x10 ) >> 4;

  memory_map[0].page = &ROM[ rom ][0x0000];
  memory_map[1].page = &ROM[ rom ][0x2000];
  memory_map[0].reverse = memory_map[1].reverse = MEMORY_PAGE_OFFSET_ROM + rom;

  memory_map[6].page = &RAM[ page ][0x0000];
  memory_map[7].page = &RAM[ page ][0x2000];
  memory_map[6].reverse = memory_map[7].reverse = page;

  /* Pages 1, 3, 5 and 7 are contended */
  memory_map[6].contended = memory_map[7].contended = page & 0x01;

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
