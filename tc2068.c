/* tc2068.c: Timex TC2068 specific routines
   Copyright (c) 1999-2003 Philip Kendall
   Copyright (c) 2002 Fredrick Meunier
   Copyright (c) 2003 Witold Filipczyk
   Copyright (c) 2003 Darren Salt

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
#include "dck.h"
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

  /* TS2040/Alphacom printer */
  { 0x00ff, 0x00fb, printer_zxp_read, printer_zxp_write },

  /* Lower 8 bits of Timex ports are fully decoded */
  { 0x00ff, 0x00fe, spectrum_ula_read, spectrum_ula_write },

  { 0x00ff, 0x00f5, tc2068_ay_registerport_read, ay_registerport_write },
  { 0x00ff, 0x00f6, tc2068_ay_dataport_read, ay_dataport_write },

  { 0x00ff, 0x00ff, scld_dec_read, scld_dec_write },
  { 0, 0, NULL, NULL } /* End marker. DO NOT REMOVE */
};

BYTE
tc2068_ay_registerport_read( WORD port )
{
  return   machine_current->ay.current_register != 14
         ? ay_registerport_read( port )
         : 0xff;
}

BYTE
tc2068_ay_dataport_read( WORD port )
{
  if (machine_current->ay.current_register != 14) {
    return ay_registerport_read( port );
  } else {

    BYTE ret =   machine_current->ay.registers[7] & 0x40
               ? machine_current->ay.registers[14]
               : 0xff;

    switch( port & 0x0300 ) {
    case 0x0100:
      ret &= ~joystick_timex_read( port, 0 );
      break;
    case 0x0200:
      ret &= ~joystick_timex_read( port, 1 );
      break;
    case 0x0300:
      ret &= ~joystick_timex_read( port, 0 ) | ~joystick_timex_read( port, 1 );
      break;
    }

    return ret;
  }
}

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
  /* The SCLD always reads the real screen memory regardless of paging
     activity */
  WORD off = offset & 0x1fff;
  int chunk = 2 + (offset >> 13);

  return timex_home[chunk].page[off];
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
tc2068_dock_exrom_reset( void )
{
  int i;
  int error;

  scld_home_free();
  scld_dock_free();
  scld_exrom_free();

  memset(timex_fake_bank, 255, sizeof(timex_fake_bank));

  timex_home[0].page = ROM[0];
  timex_home[1].page = ROM[0] + 0x2000;
  timex_home[2].page = RAM[5];
  timex_home[3].page = RAM[5] + 0x2000;
  timex_home[4].page = RAM[2];
  timex_home[5].page = RAM[2] + 0x2000;
  timex_home[6].page = RAM[0];
  timex_home[7].page = RAM[0] + 0x2000;

  timex_home[0].writeable = timex_home[1].writeable = 0;
  for( i = 2; i < 8; i++ ) timex_home[i].writeable = 1;
  for( i = 0; i < 8; i++ ) timex_home[i].allocated = 0;
  for( i = 0; i < 8; i++ ) timex_dock[i].writeable = 0;
  for( i = 0; i < 8; i++ ) timex_dock[i].page = timex_fake_bank;
  for( i = 0; i < 8; i++ ) timex_dock[i].allocated = 0;
  for( i = 0; i < 8; i++ ) timex_exrom[i].page = ROM[1];
  for( i = 0; i < 8; i++ ) timex_exrom[i].writeable = 0;
  for( i = 0; i < 8; i++ ) timex_exrom[i].allocated = 0;

  if( settings_current.dck_file ) {
    error = dck_read( settings_current.dck_file ); if( error ) return error;
  }

  return 0;
}

int
tc2068_reset( void )
{
  int error;

  error = machine_load_rom( &ROM[0], settings_current.rom_tc2068_0,
			    machine_current->rom_length[0] );
  if( error ) return error;
  error = machine_load_rom( &ROM[1], settings_current.rom_tc2068_1,
			    machine_current->rom_length[1] );
  if( error ) return error;

  error = tc2068_dock_exrom_reset(); if( error ) return error;

  scld_dec_write(255, 128);
  scld_dec_write(255, 0);
  scld_hsr_write(244, 0);

  return 0;
}
