/* tc2068.c: Timex TC2068 specific routines
   Copyright (c) 1999-2003 Philip Kendall
   Copyright (c) 2002 Fredrick Meunier
   Copyright (c) 2003 Witold Filipczyk

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

#include "ay.h"
#include "display.h"
#include "fuse.h"
#include "joystick.h"
#include "machine.h"
#include "printer.h"
#include "settings.h"
#include "scld.h"
#include "spectrum.h"
#include "tc2068.h"

static DWORD tc2068_contend_delay( void );

spectrum_port_info tc2068_peripherals[] = {

  { 0x00ff, 0x00f4, scld_hsr_read, scld_hsr_write },
  { 0x00ff, 0x00f5, spectrum_port_noread, ay_registerport_write},
  { 0x00ff, 0x00f6, ay_registerport_read, ay_dataport_write },

  /* TS2040/Alphacom printer */
  { 0x00ff, 0x00fb, printer_zxp_read, printer_zxp_write },

  /* Lower 8 bits of Timex ports are fully decoded */
  { 0x00ff, 0x00fe, spectrum_ula_read, spectrum_ula_write },

  { 0x00ff, 0x00ff, scld_dec_read, scld_dec_write },
  { 0, 0, NULL, NULL } /* End marker. DO NOT REMOVE */
};


static
BYTE tc2068_unattached_port( void )
{
  /* TC2068 does not have floating ULA values on any port (despite
     rumours to the contrary), it returns 255 on unattached ports */
  return 255;
}

BYTE
tc2068_read_screen_memory(WORD offset)
{
  if (offset >= 0x2000) return timex_memory[3][offset - 0x2000];
  return timex_memory[2][offset];
}

DWORD
tc2068_contend_memory( WORD address )
{
  /* Contention occurs only in the lowest 16Kb of RAM */
  if( address < 0x4000 || address > 0x7fff ) return 0;

  return tc2068_contend_delay();
}

DWORD
tc2068_contend_port( WORD port )
{
  /* Contention occurs for ports F4, FE and FF (HSR, ULA and DCE) */
  /* It's a guess that contention occurs for ports F5 and F6, too */
  if( ( port & 0xff ) == 0xf4 ||
      ( port & 0xff ) == 0xf5 ||
      ( port & 0xff ) == 0xf6 ||
      ( port & 0xff ) == 0xfe ||
      ( port & 0xff ) == 0xff    ) return tc2068_contend_delay();

  return 0;
}

static DWORD
tc2068_contend_delay( void )
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
    ( tstates + machine_current->timings.left_border ) %
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

int
tc2068_init( fuse_machine_info *machine )
{
  int error;

  machine->machine = LIBSPECTRUM_MACHINE_TC2068;
  machine->id = "2068";

  machine->reset = tc2068_reset;

  error = machine_set_timings( machine ); if( error ) return error;

  machine->timex = 1;
  machine->ram.read_memory	     = tc2068_readbyte;
  machine->ram.read_memory_internal  = tc2068_readbyte_internal;
  machine->ram.read_screen	     = tc2068_read_screen_memory;
  machine->ram.write_memory          = tc2068_writebyte;
  machine->ram.write_memory_internal = tc2068_writebyte_internal;
  machine->ram.contend_memory	     = tc2068_contend_memory;
  machine->ram.contend_port	     = tc2068_contend_port;
  machine->ram.current_screen = 5;

  error = machine_allocate_roms( machine, 2 );
  if( error ) return error;
  machine->rom_length[0] = 0x4000;
  machine->rom_length[1] = 0x2000;

  machine->peripherals = tc2068_peripherals;
  machine->unattached_port = tc2068_unattached_port;

  machine->ay.present = 1;

  machine->shutdown = NULL;
  return 0;

}

int
tc2068_reset( void )
{
  int error;
  int i;

  error = machine_load_rom( &ROM[0], settings_current.rom_tc2068_0,
			    machine_current->rom_length[0] );
  if( error ) return error;
  error = machine_load_rom( &ROM[1], settings_current.rom_tc2068_1,
			    machine_current->rom_length[1] );
  if( error ) return error;

  memset(timex_fake_bank, 255, sizeof(timex_fake_bank));

  timex_home[0] = ROM[0];
  timex_home[1] = ROM[0] + 0x2000;
  timex_home[2] = RAM[5];
  timex_home[3] = RAM[5] + 0x2000;
  timex_home[4] = RAM[2];
  timex_home[5] = RAM[2] + 0x2000;
  timex_home[6] = RAM[0];
  timex_home[7] = RAM[0] + 0x2000;
  timex_exrom = ROM[1];

  timex_exrom_writeable = 0;
  timex_home_writeable[0] = timex_home_writeable[1] = 0;
  for( i = 2; i < 8; i++ ) timex_home_writeable[i] = 1;
  for( i = 0; i < 8; i++ ) timex_dock_writeable[i] = 0;
  for( i = 0; i < 8; i++ ) timex_dock[i] = timex_fake_bank;

  scld_dec_write(255, 128);
  scld_dec_write(255, 0);
  scld_hsr_write(244, 0);

  return 0;
}
