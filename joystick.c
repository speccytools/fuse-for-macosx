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

/* The current state of the emulated Kempston joystick */
static libspectrum_byte joystick_value;

/* Bits used by the Kempston joystick */
static const libspectrum_byte KEMPSTON_MASK_RIGHT = 0x01;
static const libspectrum_byte KEMPSTON_MASK_LEFT  = 0x02;
static const libspectrum_byte KEMPSTON_MASK_DOWN  = 0x04;
static const libspectrum_byte KEMPSTON_MASK_UP    = 0x08;
static const libspectrum_byte KEMPSTON_MASK_FIRE  = 0x10;

char *joystick_name[ JOYSTICK_TYPE_COUNT ] = { "None", "Cursor", "Kempston" };

/* Init/shutdown functions. Errors aren't important here */

void
fuse_joystick_init (void)
{
  joysticks_supported = ui_joystick_init();
  joystick_value = 0x00;
}

void
fuse_joystick_end (void)
{
  ui_joystick_end();
}

int
joystick_press( joystick_button button, int press )
{
  keyboard_key_name key;
  libspectrum_byte mask;

  switch( settings_current.joystick_1_output ) {

  case JOYSTICK_TYPE_CURSOR:
    key = KEYBOARD_NONE;	/* Avoid warning */

    switch( button ) {
    case JOYSTICK_BUTTON_UP:    key = KEYBOARD_7; break;
    case JOYSTICK_BUTTON_DOWN:  key = KEYBOARD_6; break;
    case JOYSTICK_BUTTON_LEFT:  key = KEYBOARD_5; break;
    case JOYSTICK_BUTTON_RIGHT: key = KEYBOARD_8; break;
    case JOYSTICK_BUTTON_FIRE:  key = KEYBOARD_0; break;
    }

    if( press ) {
      keyboard_press( key );
    } else {
      keyboard_release( key );
    }

    return 1;

  case JOYSTICK_TYPE_KEMPSTON:
    mask = 0x00;		/* Avoid warning */

    switch( button ) {
    case JOYSTICK_BUTTON_UP:    mask = KEMPSTON_MASK_UP; break;
    case JOYSTICK_BUTTON_DOWN:  mask = KEMPSTON_MASK_DOWN; break;
    case JOYSTICK_BUTTON_LEFT:  mask = KEMPSTON_MASK_LEFT; break;
    case JOYSTICK_BUTTON_RIGHT: mask = KEMPSTON_MASK_RIGHT; break;
    case JOYSTICK_BUTTON_FIRE:  mask = KEMPSTON_MASK_FIRE; break;
    }

    if( press ) {
      joystick_value |=  mask;
    } else {
      joystick_value &= ~mask;
    }

    return 1;

  case JOYSTICK_TYPE_NONE: return 0;
  }

  ui_error( UI_ERROR_ERROR, "Unknown joystick type %d",
	    settings_current.joystick_1_output );
  fuse_abort();

  return 0;			/* Avoid warning */
}

libspectrum_byte
joystick_default_read( libspectrum_word port, libspectrum_byte which )
{
  /* We only support one "joystick" */
  if( which ) return 0;

  return joystick_value;
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
