/* tc2068.c: Timex TC2068 specific routines
   Copyright (c) 1999-2004 Philip Kendall
   Copyright (c) 2002-2004 Fredrick Meunier
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

#include <libspectrum.h>

#include "dck.h"
#include "joystick.h"
#include "machine.h"
#include "machines.h"
#include "periph.h"
#include "printer.h"
#include "scld.h"
#include "settings.h"

#define ADDR_TO_CHUNK(addr) 2 + (addr >> 13)

static libspectrum_byte tc2068_ay_registerport_read( libspectrum_word port,
						     int *attached );
static libspectrum_byte tc2068_ay_dataport_read( libspectrum_word port,
						 int *attached );
static libspectrum_byte tc2068_contend_delay( libspectrum_dword time );
static int dock_exrom_reset( void );

static int tc2068_reset( void );

const static periph_t peripherals[] = {
  { 0x00ff, 0x00f4, scld_hsr_read, scld_hsr_write },

  /* TS2040/Alphacom printer */
  { 0x00ff, 0x00fb, printer_zxp_read, printer_zxp_write },

  /* Lower 8 bits of Timex ports are fully decoded */
  { 0x00ff, 0x00fe, spectrum_ula_read, spectrum_ula_write },

  { 0x00ff, 0x00f5, tc2068_ay_registerport_read, ay_registerport_write },
  { 0x00ff, 0x00f6, tc2068_ay_dataport_read, ay_dataport_write },

  { 0x00ff, 0x00ff, scld_dec_read, scld_dec_write },
};

const static size_t peripherals_count =
  sizeof( peripherals ) / sizeof( periph_t );

static libspectrum_byte
tc2068_ay_registerport_read( libspectrum_word port, int *attached )
{
  if( machine_current->ay.current_register == 14 ) return 0xff;

  return ay_registerport_read( port, attached );
}

static libspectrum_byte
tc2068_ay_dataport_read( libspectrum_word port, int *attached )
{
  if (machine_current->ay.current_register != 14) {
    return ay_registerport_read( port, attached );
  } else {

    libspectrum_byte ret;

    /* In theory, we may need to distinguish cases where some data
       is returned here and were it isn't. In practice, this doesn't
       matter for the TC2068 as it doesn't have a floating bus, so we'll
       get 0xff in both cases anwyay */
    *attached = 1;

    ret =   machine_current->ay.registers[7] & 0x40
	  ? machine_current->ay.registers[14]
	  : 0xff;

    if( port & 0x0100 ) ret &= ~joystick_timex_read( port, 0 );
    if( port & 0x0200 ) ret &= ~joystick_timex_read( port, 1 );

    return ret;
  }
}

static libspectrum_byte
tc2068_unattached_port( void )
{
  /* TC2068 does not have floating ULA values on any port (despite
     rumours to the contrary), it returns 0xff on unattached ports */
  return 0xff;
}

libspectrum_byte
tc2068_read_screen_memory( libspectrum_word offset )
{
  /* The SCLD always reads the real screen memory regardless of paging
     activity */
  libspectrum_word off = offset & 0x1fff;

  return timex_home[ADDR_TO_CHUNK(offset)].page[off];
}

libspectrum_dword
tc2068_contend_port( libspectrum_word port )
{
  /* Contention occurs for ports F4, FE and FF (HSR, ULA and DEC) */
  /* It's a guess that contention occurs for ports F5 and F6, too */
  if( ( port & 0xff ) == 0xf4 ||
      ( port & 0xff ) == 0xf5 ||
      ( port & 0xff ) == 0xf6 ||
      ( port & 0xff ) == 0xfe ||
      ( port & 0xff ) == 0xff    ) return tc2068_contend_delay( tstates );

  return 0;
}

static libspectrum_byte
tc2068_contend_delay( libspectrum_dword time )
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

int
tc2068_init( fuse_machine_info *machine )
{
  int error;

  machine->machine = LIBSPECTRUM_MACHINE_TC2068;
  machine->id = "2068";

  machine->reset = tc2068_reset;

  error = machine_set_timings( machine ); if( error ) return error;

  machine->timex = 1;
  machine->ram.read_screen	     = tc2068_read_screen_memory;
  machine->ram.contend_port	     = tc2068_contend_port;
  machine->ram.contend_delay	     = tc2068_contend_delay;

  error = machine_allocate_roms( machine, 2 );
  if( error ) return error;
  machine->rom_length[0] = 0x4000;
  machine->rom_length[1] = 0x2000;

  machine->unattached_port = tc2068_unattached_port;

  machine->ay.present = 1;

  machine->shutdown = NULL;

  return 0;
}

static int
dock_exrom_reset( void )
{
  int i;
  int error;

  scld_home_free();
  scld_dock_free();
  scld_exrom_free();

  memset( timex_fake_bank, 0xff, 0x2000);

  timex_home[0].page = ROM[0];
  timex_home[1].page = ROM[0] + 0x2000;
  timex_home[0].reverse = timex_home[1].reverse = -1;

  timex_home[2].page = RAM[5];
  timex_home[3].page = RAM[5] + 0x2000;
  timex_home[2].reverse = timex_home[3].reverse = 5;

  timex_home[4].page = RAM[2];
  timex_home[5].page = RAM[2] + 0x2000;
  timex_home[4].reverse = timex_home[5].reverse = 2;

  timex_home[6].page = RAM[0];
  timex_home[7].page = RAM[0] + 0x2000;
  timex_home[6].reverse = timex_home[7].reverse = 0;

  for( i = 0 ; i < 8; i++ ) timex_home[i].offset = ( i & 1 ? 0x2000 : 0x0000 );

  timex_home[0].writable = timex_home[1].writable = 0;
  for( i = 2; i < 8; i++ ) timex_home[i].writable = 1;
  for( i = 0; i < 8; i++ ) timex_home[i].allocated = 0;
  for( i = 0; i < 8; i++ ) timex_home[i].contended = 0;
  timex_home[2].contended = timex_home[3].contended = 1;

  for( i = 0; i < 8; i++ ) {

    timex_dock[i].page = timex_fake_bank;
    timex_dock[i].writable = 0;
    timex_dock[i].allocated = 0;
    timex_dock[i].contended = 0;

    timex_exrom[i].page = ROM[1];
    timex_exrom[i].writable = 0;
    timex_exrom[i].allocated = 0;
    timex_exrom[i].contended = 0;

  }

  if( settings_current.dck_file ) {
    error = dck_read( settings_current.dck_file ); if( error ) return error;
  }

  return 0;
}

static int
tc2068_reset( void )
{
  int error;

  error = machine_load_rom( &ROM[0], settings_current.rom_tc2068_0,
			    machine_current->rom_length[0] );
  if( error ) return error;
  error = machine_load_rom( &ROM[1], settings_current.rom_tc2068_1,
			    machine_current->rom_length[1] );
  if( error ) return error;

  error = periph_setup( peripherals, peripherals_count, PERIPH_PRESENT_NEVER );
  if( error ) return error;

  error = dock_exrom_reset(); if( error ) return error;

  scld_dec_write( 0x00ff, 0x80 );
  scld_dec_write( 0x00ff, 0x00 );
  scld_hsr_write( 0x00f4, 0x00 );

  memory_current_screen = 5;
  memory_screen_mask = 0xdfff;

  return 0;
}
