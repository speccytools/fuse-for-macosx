/* specplus3e.c: Spectrum +3e specific routines
   Copyright (c) 1999-2004 Philip Kendall, Darren Salt

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

#include "machine.h"
#include "periph.h"
#include "settings.h"
#include "specplus3.h"
#include "ui/ui.h"

static int specplus3e_reset( void );

int
specplus3e_init( fuse_machine_info *machine )
{
  int error;

  machine->machine = LIBSPECTRUM_MACHINE_PLUS3;
  machine->id = "plus3e";

  machine->reset = specplus3e_reset;

  error = machine_set_timings( machine ); if( error ) return error;

  machine->timex = 0;
  machine->ram.read_screen	     = specplus3_read_screen_memory;
  machine->ram.contend_port	     = specplus3_contend_port;
  machine->ram.contend_delay	     = specplus3_contend_delay;

  error = machine_allocate_roms( machine, 4 );
  if( error ) return error;
  machine->rom_length[0] = machine->rom_length[1] = 
    machine->rom_length[2] = machine->rom_length[3] = 0x4000;

  machine->unattached_port = specplus3_unattached_port;

  machine->ay.present=1;

  specplus3_765_init();

  machine->shutdown = specplus3_shutdown;

  return 0;
}

static int
specplus3e_reset( void )
{
  int error;

  error = machine_load_rom( &ROM[0], settings_current.rom_plus3e_0,
			    machine_current->rom_length[0] );
  if( error ) return error;
  error = machine_load_rom( &ROM[1], settings_current.rom_plus3e_1,
			    machine_current->rom_length[1] );
  if( error ) return error;
  error = machine_load_rom( &ROM[2], settings_current.rom_plus3e_2,
			    machine_current->rom_length[2] );
  if( error ) return error;
  error = machine_load_rom( &ROM[3], settings_current.rom_plus3e_3,
			    machine_current->rom_length[3] );
  if( error ) return error;

  error = periph_setup( specplus3_peripherals, specplus3_peripherals_count,
			PERIPH_PRESENT_OPTIONAL );
  if( error ) return error;

#ifdef HAVE_765_H
  specplus3_fdc_reset();
  specplus3_menu_items();
#endif				/* #ifdef HAVE_765_H */

  return specplus3_plus2a_common_reset();
}
