/* spec48.c: Spectrum 48K specific routines
   Copyright (c) 1999-2002 Philip Kendall

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

#include "display.h"
#include "fuse.h"
#include "joystick.h"
#include "machine.h"
#include "printer.h"
#include "settings.h"
#include "spec48.h"
#include "spectrum.h"

static DWORD spec48_contend_delay( void );

spectrum_port_info spec48_peripherals[] = {
  { 0x0001, 0x0000, spectrum_ula_read, spectrum_ula_write },
  { 0x0004, 0x0000, printer_zxp_read, printer_zxp_write },
  { 0x00e0, 0x0000, joystick_kempston_read, spectrum_port_nowrite },
  { 0, 0, NULL, NULL } /* End marker. DO NOT REMOVE */
};

static BYTE spec48_unattached_port( void )
{
  return spectrum_unattached_port( 1 );
}

BYTE spec48_read_screen_memory(WORD offset)
{
  return RAM[5][offset];
}

DWORD spec48_contend_memory( WORD address )
{
  /* Contention occurs only in the lowest 16Kb of RAM */
  if( address < 0x4000 || address > 0x7fff ) return 0;

  return spec48_contend_delay();
}

DWORD spec48_contend_port( WORD port )
{
  /* Contention occurs only for even-numbered ports */
  if( ( port & 0x01 ) == 0 ) return spec48_contend_delay();

  return 0;
}

static DWORD spec48_contend_delay( void )
{
  WORD tstates_through_line;
  
  /* No contention in the upper border */
  if( tstates < machine_current->line_times[ DISPLAY_BORDER_HEIGHT ] )
    return 0;

  /* Or the lower border */
  if( tstates >= machine_current->line_times[ DISPLAY_BORDER_HEIGHT + 
                                              DISPLAY_HEIGHT          ] )
    return 0;

  /* Work out where we are in this line */
  tstates_through_line =
    ( tstates + machine_current->timings.left_border_cycles ) %
    machine_current->timings.cycles_per_line;

  /* No contention if we're in the left border */
  if( tstates_through_line < machine_current->timings.left_border_cycles - 1 ) 
    return 0;

  /* Or the right border or retrace */
  if( tstates_through_line >= machine_current->timings.left_border_cycles +
                              machine_current->timings.screen_cycles - 1 )
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
  int error;

  machine->machine = LIBSPECTRUM_MACHINE_48;
  machine->id = "48";

  machine->reset = spec48_reset;

  machine_set_timings( machine, 3.5e6, 24, 128, 24, 48, 312, 8936 );

  machine->timex = 0;
  machine->ram.read_memory           = spec48_readbyte;
  machine->ram.read_memory_internal  = spec48_readbyte_internal;
  machine->ram.read_screen           = spec48_read_screen_memory;
  machine->ram.write_memory	     = spec48_writebyte;
  machine->ram.write_memory_internal = spec48_writebyte_internal;
  machine->ram.contend_memory        = spec48_contend_memory;
  machine->ram.contend_port          = spec48_contend_port;

  error = machine_allocate_roms( machine, 1 );
  if( error ) return error;
  machine->rom_length[0] = 0x4000;

  machine->peripherals = spec48_peripherals;
  machine->unattached_port = spec48_unattached_port;

  machine->ay.present = 0;

  machine->shutdown = NULL;

  return 0;

}

int
spec48_reset( void )
{
  int error;

  error = machine_load_rom( &ROM[0], settings_current.rom_48,
			    machine_current->rom_length[0] );
  if( error ) return error;

  return 0;
}
