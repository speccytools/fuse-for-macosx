/* specplus2a.c: Spectrum +2A specific routines
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

#include "ay.h"
#include "display.h"
#include "joystick.h"
#include "machine.h"
#include "printer.h"
#include "settings.h"
#include "spec128.h"
#include "specplus2a.h"
#include "specplus3.h"
#include "spectrum.h"

spectrum_port_info
specplus2a_peripherals[] = {
  { 0x0001, 0x0000, spectrum_ula_read, spectrum_ula_write },
  { 0x00e0, 0x0000, joystick_kempston_read, spectrum_port_nowrite },
  { 0xc002, 0xc000, ay_registerport_read, ay_registerport_write },
  { 0xc002, 0x8000, spectrum_port_noread, ay_dataport_write },
  { 0xc002, 0x4000, spectrum_port_noread, spec128_memoryport_write },
  { 0xf002, 0x1000, spectrum_port_noread, specplus3_memoryport_write },
  { 0xf002, 0x0000, printer_parallel_read, printer_parallel_write },
  { 0, 0, NULL, NULL } /* End marker. DO NOT REMOVE */
};

int
specplus2a_init( fuse_machine_info *machine )
{
  int error;

  machine->machine = LIBSPECTRUM_MACHINE_PLUS2A;
  machine->id = "plus2a";

  machine->reset = specplus2a_reset;

  machine_set_timings( machine, 3.54690e6, 24, 128, 24, 52, 311, 8865 );

  machine->timex = 0;
  machine->ram.read_memory	     = specplus3_readbyte;
  machine->ram.read_memory_internal  = specplus3_readbyte_internal;
  machine->ram.read_screen	     = specplus3_read_screen_memory;
  machine->ram.write_memory          = specplus3_writebyte;
  machine->ram.write_memory_internal = specplus3_writebyte_internal;
  machine->ram.contend_memory	     = specplus3_contend_memory;
  machine->ram.contend_port	     = specplus3_contend_port;

  error = machine_allocate_roms( machine, 4 );
  if( error ) return error;
  error = machine_allocate_rom( machine, 0, settings_current.rom_plus2a_0,
				0x4000 );
  if( error ) return error;
  error = machine_allocate_rom( machine, 1, settings_current.rom_plus2a_1,
				0x4000 );
  if( error ) return error;
  error = machine_allocate_rom( machine, 2, settings_current.rom_plus2a_2,
				0x4000 );
  if( error ) return error;
  error = machine_allocate_rom( machine, 3, settings_current.rom_plus2a_3,
				0x4000 );
  if( error ) return error;

  machine->peripherals = specplus2a_peripherals;
  machine->unattached_port = specplus3_unattached_port;

  machine->ay.present=1;

  machine->shutdown = NULL;

  return 0;

}

int specplus2a_reset(void)
{
  machine_current->ram.current_page = 0;
  machine_current->ram.current_rom = 0;
  machine_current->ram.current_screen = 5;
  machine_current->ram.locked = 0;
  machine_current->ram.special = 0;
  machine_current->ram.specialcfg = 0;

  return 0;
}

