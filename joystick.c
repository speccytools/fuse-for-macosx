/* joystick.c: Joystick emulation support
   Copyright (c) 2001-2004 Russell Marks, Philip Kendall
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

#include "fuse.h"
#include "joystick.h"
#include "keyboard.h"
#include "periph.h"
#include "spectrum.h"
#include "machine.h"
#include "ui/ui.h"
#include "ui/uijoystick.h"

/* Number of joysticks known about & initialised */
int joysticks_supported = 0;

/* Init/shutdown functions. Errors aren't important here */

void
fuse_joystick_init (void)
{
  joysticks_supported = ui_joystick_init();
}

void
fuse_joystick_end (void)
{
  ui_joystick_end();
}

/* Returns joystick direction/button state in Kempston format.
   This is used if no (hardware) joysticks are found. */

libspectrum_byte
joystick_default_read( libspectrum_word port, libspectrum_byte which )
{
  /* Offset/mask in keyboard_return_values[] for joystick keys,
     in order right, left, down, up, fire. These are p/o/a/q/Space */
  static const int offset[5] = {    5,    5,    1,    2,    7 };
  static const int mask[5]   = { 0x01, 0x02, 0x01, 0x01, 0x01 };
  libspectrum_byte return_value = 0, jmask = 1;
  int i;

  /* We only support one "joystick" */
  if( which ) return 0;

  for( i=0; i<5; i++, jmask<<=1 ) {
    if( !(keyboard_return_values[offset[i]] & mask[i]) )
      return_value|=jmask;
  }

  return return_value;
}

/* Read functions for specific interfaces */

libspectrum_byte
joystick_kempston_read( libspectrum_word port, int *attached )
{
  if( !periph_kempston_active ) return 0xff;

  *attached = 1;

  /* If we have no real joysticks, return the QAOP<space>-emulated value */
  if( joysticks_supported == 0 ) return joystick_default_read( port, 0 );

  /* Return the value from the actual joystick */
  return ui_joystick_read( port, 0 );
}

libspectrum_byte
joystick_timex_read( libspectrum_word port, libspectrum_byte which )
{
  static const libspectrum_byte translate[] = {
    0x00, 0x08, 0x04, 0x0C, 0x02, 0x0A, 0x06, 0x0E,
    0x01, 0x09, 0x05, 0x0D, 0x03, 0x0B, 0x07, 0x0F,
    0x80, 0x88, 0x84, 0x8C, 0x82, 0x8A, 0x86, 0x8E,
    0x81, 0x89, 0x85, 0x8D, 0x83, 0x8B, 0x87, 0x8F,
  };

  /* If we don't have a real joystick for this, use the QAOP<space>-emulated
     value */
  if( joysticks_supported <= which )
    return translate[ joystick_default_read( port, which ) ];

  /* If we do have a real joystick, read it */
  return translate[ ui_joystick_read( port, which ) ];
}
