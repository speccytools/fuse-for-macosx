/* tc2048.c: Timex TC2048 specific routines
   Copyright (c) 1999-2005 Philip Kendall
   Copyright (c) 2002-2004 Fredrick Meunier

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

#include <string.h>

#include <libspectrum.h>

#include "joystick.h"
#include "machine.h"
#include "machines.h"
#include "memory.h"
#include "periph.h"
#include "printer.h"
#include "settings.h"
#include "scld.h"
#include "spec48.h"
#include "tc2068.h"
#include "ula.h"
#include "if1.h"

static int tc2048_reset( void );
static libspectrum_byte tc2048_contend_delay( libspectrum_dword time );

static const periph_t peripherals[] = {
  { 0x0020, 0x0000, joystick_kempston_read, NULL },
  { 0x0018, 0x0010, if1_port_in, if1_port_out },
  { 0x0018, 0x0008, if1_port_in, if1_port_out },
  { 0x0018, 0x0000, if1_port_in, if1_port_out },
  { 0x00ff, 0x00f4, scld_hsr_read, scld_hsr_write },

  /* TS2040/Alphacom printer */
  { 0x00ff, 0x00fb, printer_zxp_read, printer_zxp_write },

  /* Lower 8 bits of Timex ports are fully decoded */
  { 0x00ff, 0x00fe, ula_read, ula_write },

  { 0x00ff, 0x00ff, scld_dec_read, scld_dec_write },
};

static const size_t peripherals_count =
  sizeof( peripherals ) / sizeof( periph_t );

static libspectrum_byte
tc2048_unattached_port( void )
{
  /* TC2048 does not have floating ULA values on any port (despite rumours
     to the contrary), it returns 0xff on unattached ports */
  return 0xff;
}

int
tc2048_port_from_ula( libspectrum_word port )
{
  /* Ports F4 (HSR), FE (SCLD) and FF (DEC) supplied by ULA */
  port &= 0xff;

  return( port == 0xf4 || port == 0xfe || port == 0xff );
}

static libspectrum_byte
tc2048_contend_delay( libspectrum_dword time )
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

int tc2048_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_TC2048;
  machine->id = "2048";

  machine->reset = tc2048_reset;

  machine->timex = 1;
  machine->ram.port_from_ula	     = tc2048_port_from_ula;
  machine->ram.contend_delay	     = tc2048_contend_delay;
  machine->ram.contend_delay_no_mreq = tc2048_contend_delay;

  memset( fake_bank, 0xff, MEMORY_PAGE_SIZE );

  fake_mapping.page = fake_bank;
  fake_mapping.writable = 0;
  fake_mapping.contended = 0;
  fake_mapping.offset = 0x0000;

  machine->unattached_port = tc2048_unattached_port;

  machine->shutdown = NULL;

  machine->memory_map = tc2068_memory_map;

  return 0;

}

static int
tc2048_reset( void )
{
  size_t i;
  int error;

  error = machine_load_rom( 0, 0, settings_current.rom_tc2048,
                            settings_default.rom_tc2048, 0x4000 );
  if( error ) return error;

  error = periph_setup( peripherals, peripherals_count );
  if( error ) return error;
  periph_setup_kempston( PERIPH_PRESENT_ALWAYS );
  periph_setup_interface1( PERIPH_PRESENT_OPTIONAL );
  periph_setup_interface2( PERIPH_PRESENT_OPTIONAL );
  periph_setup_plusd( PERIPH_PRESENT_OPTIONAL );
  periph_update();

  for( i = 0; i < 8; i++ ) {

    timex_dock[i] = fake_mapping;
    timex_dock[i].bank= MEMORY_BANK_DOCK;
    timex_dock[i].page_num = i;
    timex_dock[i].source= MEMORY_SOURCE_SYSTEM;
    memory_map_dock[i] = &timex_dock[i];

    timex_exrom[i] = fake_mapping;
    timex_exrom[i].bank = MEMORY_BANK_EXROM;
    timex_exrom[i].page_num = i;
    timex_exrom[i].source= MEMORY_SOURCE_SYSTEM;
    memory_map_exrom[i] = &timex_exrom[i];

  }

  return tc2068_tc2048_common_reset();
}
