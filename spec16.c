/* spec16.c: Spectrum 16K specific routines
   Copyright (c) 1999-2003 Philip Kendall

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
#include "keyboard.h"
#include "machine.h"
#include "printer.h"
#include "settings.h"
#include "snapshot.h"
#include "sound.h"
#include "spec16.h"
#include "spec48.h"
#include "spectrum.h"
#include "ui/ui.h"
#include "z80/z80.h"

spectrum_port_info spec16_peripherals[] = {
  { 0x0001, 0x0000, spectrum_ula_read, spectrum_ula_write },
  { 0x0004, 0x0000, printer_zxp_read, printer_zxp_write },
  { 0x00e0, 0x0000, joystick_kempston_read, spectrum_port_nowrite },
  { 0, 0, NULL, NULL } /* End marker. DO NOT REMOVE */
};

static libspectrum_byte
spec16_unattached_port( void )
{
  return spectrum_unattached_port( 1 );
}

int spec16_init( fuse_machine_info *machine )
{
  int error;

  machine->machine = LIBSPECTRUM_MACHINE_16;
  machine->id = "16";

  machine->reset = spec16_reset;

  error = machine_set_timings( machine ); if( error ) return error;

  machine->timex = 0;
  machine->ram.read_memory    = spec16_readbyte;
  machine->ram.read_memory_internal  = spec16_readbyte_internal;
  machine->ram.read_screen    = spec48_read_screen_memory;
  machine->ram.write_memory   = spec16_writebyte;
  machine->ram.write_memory_internal = spec16_writebyte_internal;
  machine->ram.contend_memory = spec48_contend_memory;
  machine->ram.contend_port   = spec48_contend_port;
  machine->ram.current_screen = 5;

  error = machine_allocate_roms( machine, 1 );
  if( error ) return error;
  machine->rom_length[0] = 0x4000;

  machine->peripherals = spec16_peripherals;
  machine->unattached_port = spec16_unattached_port;

  machine->ay.present = 0;

  machine->shutdown = NULL;

  return 0;

}

int
spec16_reset( void )
{
  int error;

  error = machine_load_rom( &ROM[0], settings_current.rom_16,
			    machine_current->rom_length[0] );
  if( error ) return error;

  return 0;
}
