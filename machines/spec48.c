/* spec48.c: Spectrum 48K specific routines
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

#include "joystick.h"
#include "machine.h"
#include "memory.h"
#include "periph.h"
#include "printer.h"
#include "settings.h"
#include "spec48.h"
#include "spectrum.h"
#include "ula.h"
#include "if1.h"

static int spec48_reset( void );

const static periph_t peripherals[] = {
  { 0x0001, 0x0000, ula_read, ula_write },
  { 0x0018, 0x0010, if1_port_in, if1_port_out },
  { 0x0018, 0x0008, if1_port_in, if1_port_out },
  { 0x0018, 0x0000, if1_port_in, if1_port_out },
  { 0x0004, 0x0000, printer_zxp_read, printer_zxp_write },
  { 0x00e0, 0x0000, joystick_kempston_read, NULL },
};

const static size_t peripherals_count =
  sizeof( peripherals ) / sizeof( periph_t );

static libspectrum_byte
spec48_unattached_port( void )
{
  return spectrum_unattached_port();
}

int
spec48_port_contended( libspectrum_word port )
{
  /* All even ports contended */
  return !( port & 0x0001 );
}

libspectrum_byte
spec48_contend_delay( libspectrum_dword time )
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
  if( tstates_through_line < machine_current->timings.left_border - 1 ) 
    return 0;

  /* Or the right border or retrace */
  if( tstates_through_line >= machine_current->timings.left_border +
                              machine_current->timings.horizontal_screen - 1 )
    return 0;

  /* We now know the ULA is reading the screen, so put in the appropriate
     delay */
  switch( tstates_through_line % 8 ) {
    case 7: return 6; break;
    case 0: return 5; break;
    case 1: return 4; break;
    case 2: return 3; break;
    case 3: return 2; break;
    case 4: return 1; break;
    case 5: return 0; break;
    case 6: return 0; break;
  }

  return 0;	/* Shut gcc up */
}

int spec48_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_48;
  machine->id = "48";

  machine->reset = spec48_reset;

  machine->timex = 0;
  machine->ram.port_contended        = spec48_port_contended;
  machine->ram.contend_delay	     = spec48_contend_delay;

  machine->unattached_port = spec48_unattached_port;

  machine->shutdown = NULL;

  machine->memory_map = spec48_memory_map;

  return 0;

}

static int
spec48_reset( void )
{
  int error;

  error = machine_load_rom( 0, 0, settings_current.rom_48,
                            settings_default.rom_48, 0x4000 );
  if( error ) return error;

  error = periph_setup( peripherals, peripherals_count );
  if( error ) return error;
  periph_setup_kempston( PERIPH_PRESENT_OPTIONAL );
  periph_setup_interface1( PERIPH_PRESENT_OPTIONAL );
  periph_setup_interface2( PERIPH_PRESENT_OPTIONAL );
  periph_update();

  memory_current_screen = 5;
  memory_screen_mask = 0xffff;

  return spec48_common_reset();
}

int
spec48_common_reset( void )
{
  size_t i;

  /* ROM 0, RAM 5, RAM 2, RAM 0 */
  memory_map_home[0] = &memory_map_rom[ 0];
  memory_map_home[1] = &memory_map_rom[ 1];

  memory_map_home[2] = &memory_map_ram[10];
  memory_map_home[3] = &memory_map_ram[11];

  memory_map_home[4] = &memory_map_ram[ 4];
  memory_map_home[5] = &memory_map_ram[ 5];

  memory_map_home[6] = &memory_map_ram[ 0];
  memory_map_home[7] = &memory_map_ram[ 1];

  /* 0x4000 - 0x7fff contended */
  memory_map_home[2]->contended = memory_map_home[3]->contended = 1;

  /* 0x8000 - 0xffff not contended */
  memory_map_home[ 4]->contended = memory_map_home[ 5]->contended = 0;
  memory_map_home[ 6]->contended = memory_map_home[ 7]->contended = 0;

  /* Mark as present/writeable */
  for( i = 2; i < 8; ++i )
    memory_map_home[i]->writable = 1;

  for( i = 0; i < 8; i++ )
    memory_map_read[i] = memory_map_write[i] = *memory_map_home[i];

  return 0;
}

int
spec48_memory_map( void )
{
  /* By default, 0x0000 to 0x3fff come from the home bank */
  memory_map_read[0] = memory_map_write[0] = *memory_map_home[0];
  memory_map_read[1] = memory_map_write[1] = *memory_map_home[1];

  memory_romcs_map();

  return 0;
}
