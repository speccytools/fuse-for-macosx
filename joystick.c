/* joystick.c: Joystick emulation support
   Copyright (c) 2001 Russell Marks, Philip Kendall
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
#include "settings.h"
#include "spectrum.h"
#include "machine.h"

BYTE joystick_kempston_read(WORD port)
{
  /* Offset/mask in keyboard_return_values[] for joystick keys,
     in order right, left, down, up, fire. These are p/o/a/q/Space */
  static int offset[5] = {    5,    5,    1,    2,    7 };
  static int mask[5]   = { 0x01, 0x02, 0x01, 0x01, 0x01 };
  BYTE return_value = 0, jmask = 1;
  int i;

  /* The TC2048 has a unremoveable Kempston interface */
  if( ( machine_current->machine != LIBSPECTRUM_MACHINE_TC2048 )
	  && !settings_current.joy_kempston )
    return spectrum_port_noread(port);

  for( i=0; i<5; i++, jmask<<=1 ) {
    if( !(keyboard_return_values[offset[i]] & mask[i]) )
      return_value|=jmask;
  }

  return return_value;
}

BYTE
joystick_timex_read( WORD port, int which )
{
  static const BYTE translate[] = {
    0x00, 0x08, 0x04, 0x0C, 0x02, 0x0A, 0x06, 0x0E,
    0x01, 0x09, 0x05, 0x0D, 0x03, 0x0B, 0x07, 0x0F,
    0x80, 0x88, 0x84, 0x8C, 0x82, 0x8A, 0x86, 0x8E,
    0x81, 0x89, 0x85, 0x8D, 0x83, 0x8B, 0x87, 0x8F,
  };

  if( which == 0 )
    return translate[ joystick_kempston_read( port ) ];

  return 0;
}
