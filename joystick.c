/* joystick.c: Joystick emulation support
   Copyright (c) 2001 Russell Marks, Philip Kendall

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

#include "fuse.h"
#include "joystick.h"
#include "keyboard.h"
#include "settings.h"
#include "spectrum.h"

BYTE joystick_kempston_read(WORD port)
{
  /* Offset/mask in keyboard_return_values[] for joystick keys,
     in order right, left, down, up, fire. These are p/o/a/q/Space */
  static int offset[5] = {    5,    5,    1,    2,    7 };
  static int mask[5]   = { 0x01, 0x02, 0x01, 0x01, 0x01 };
  BYTE return_value = 0, jmask = 1;
  int i;

  if( !settings_current.joy_kempston )
    return spectrum_port_noread(port);

  for( i=0; i<5; i++, jmask<<=1 ) {
    if( !(keyboard_return_values[offset[i]] & mask[i]) )
      return_value|=jmask;
  }

  return return_value;
}


void joystick_kempston_write(WORD port, BYTE b)
{
  /* nothing, presumably... */
}
