/* ay.c: AY-8-3912 routines
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

#include "compat.h"
#include "machine.h"
#include "printer.h"
#include "psg.h"
#include "sound.h"

/* Unused bits in the AY registers are silently zeroed out; these masks
   accomplish this */
static const libspectrum_byte mask[16] = {

  0xff, 0x0f, 0xff, 0x0f, 0xff, 0x0f, 0x1f, 0xff,
  0x1f, 0x1f, 0x1f, 0xff, 0xff, 0x0f, 0xff, 0xff,

};

/* What happens when the AY register port (traditionally 0xfffd on the 128K
   machines) is read from */
libspectrum_byte
ay_registerport_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  int current;
  const libspectrum_byte port_input = 0xbf; /* always allow serial output */

  *attached = 1;

  current = machine_current->ay.current_register;

  /* The AY I/O ports return input directly from the port when in
     input mode; but in output mode, they return an AND between the
     register value and the port input. So, allow for this when
     reading R14... */

  if( current == 14 ) {
    if(machine_current->ay.registers[7] & 0x40)
      return (port_input & machine_current->ay.registers[14]);
    else
      return port_input;
  }

  /* R15 is simpler to do, as the 8912 lacks the second I/O port, and
     the input-mode input is always 0xff */
  if( current == 15 && !( machine_current->ay.registers[7] & 0x80 ) )
    return 0xff;

  /* Otherwise return register value, appropriately masked */
  return machine_current->ay.registers[ current ] & mask[ current ];
}

/* And when it's written to */
void
ay_registerport_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  machine_current->ay.current_register = (b & 15);
}

/* What happens when the AY data port (traditionally 0xbffd on the 128K
   machines) is written to; no corresponding read function as this
   always returns 0xff */
void
ay_dataport_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  int current;

  current = machine_current->ay.current_register;

  machine_current->ay.registers[ current ] = b & mask[ current ];
  sound_ay_write( current, b, tstates );
  if( psg_recording ) psg_write_register( current, b );

  if( current == 14 ) printer_serial_write( b );
}
