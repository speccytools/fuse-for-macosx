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
#include "settings.h"
#include "spectrum.h"
#include "machine.h"
#include "ui/ui.h"
#include "ui/uijoystick.h"

/* Number of joysticks known about & initialised */
int joysticks_supported = 0;

/* The bit masks used by the various joysticks. The order is the same
   as the ordering of buttons in joystick.h:joystick_button (left,
   right, up, down, fire ) */
static const libspectrum_byte kempston_mask[5] =
  { 0x02, 0x01, 0x08, 0x04, 0x10 };
static const libspectrum_byte timex_mask[5] =
  { 0x04, 0x08, 0x01, 0x02, 0x80 };

/* The keys used by the Cursor joystick */
static const keyboard_key_name cursor_key[5] =
  { KEYBOARD_5, KEYBOARD_8, KEYBOARD_7, KEYBOARD_6, KEYBOARD_0 };

/* The keys used by the two Sinclair joysticks */
static const keyboard_key_name sinclair1_key[5] =
  { KEYBOARD_1, KEYBOARD_2, KEYBOARD_4, KEYBOARD_3, KEYBOARD_5 };
static const keyboard_key_name sinclair2_key[5] =
  { KEYBOARD_6, KEYBOARD_7, KEYBOARD_9, KEYBOARD_8, KEYBOARD_0 };

/* The current values for the joysticks we can emulate */
static libspectrum_byte kempston_value;
static libspectrum_byte timex1_value;
static libspectrum_byte timex2_value;

/* The names of the joysticks we can emulate. Order must correspond to
   that of joystick.h:joystick_type_t */
const char *joystick_name[ JOYSTICK_TYPE_COUNT ] = {
  "None",
  "Cursor",
  "Kempston",
  "Sinclair 1", "Sinclair 2",
  "Timex 1", "Timex 2"
};

/* Init/shutdown functions. Errors aren't important here */

void
fuse_joystick_init (void)
{
  joysticks_supported = ui_joystick_init();
  kempston_value = timex1_value = timex2_value = 0x00;
}

void
fuse_joystick_end (void)
{
  ui_joystick_end();
}

int
joystick_press( int which, joystick_button button, int press )
{
  joystick_type_t type;

  switch( which ) {
  case 0: type = settings_current.joystick_1_output; break;
  case 1: type = settings_current.joystick_2_output; break;

  case JOYSTICK_KEYBOARD:
    type = settings_current.joystick_keyboard_output; break;

  default:
    return 0;
  }

  switch( type ) {

  case JOYSTICK_TYPE_CURSOR:
    if( press ) {
      keyboard_press( cursor_key[ button ] );
    } else {
      keyboard_release( cursor_key[ button ] );
    }
    return 1;

  case JOYSTICK_TYPE_KEMPSTON:
    if( press ) {
      kempston_value |=  kempston_mask[ button ];
    } else {
      kempston_value &= ~kempston_mask[ button ];
    }
    return 1;

  case JOYSTICK_TYPE_SINCLAIR_1:
    if( press ) {
      keyboard_press( sinclair1_key[ button ] );
    } else {
      keyboard_release( sinclair1_key[ button ] );
    }
    return 1;

  case JOYSTICK_TYPE_SINCLAIR_2:
    if( press ) {
      keyboard_press( sinclair2_key[ button ] );
    } else {
      keyboard_release( sinclair2_key[ button ] );
    }
    return 1;

  case JOYSTICK_TYPE_TIMEX_1:
    if( press ) {
      timex1_value |=  timex_mask[ button ];
    } else {
      timex1_value &= ~timex_mask[ button ];
    }
    return 1;

  case JOYSTICK_TYPE_TIMEX_2:
    if( press ) {
      timex2_value |=  timex_mask[ button ];
    } else {
      timex2_value &= ~timex_mask[ button ];
    }
    return 1;

  case JOYSTICK_TYPE_NONE: return 0;
  }

  ui_error( UI_ERROR_ERROR, "Unknown joystick type %d",
	    settings_current.joystick_1_output );
  fuse_abort();

  return 0;			/* Avoid warning */
}

/* Read functions for specific interfaces */

libspectrum_byte
joystick_kempston_read( libspectrum_word port, int *attached )
{
  if( !periph_kempston_active ) return 0xff;

  *attached = 1;

  return kempston_value;
}

libspectrum_byte
joystick_timex_read( libspectrum_word port, libspectrum_byte which )
{
  return which ? timex2_value : timex1_value;
}
