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

/* FIXME: make two joysticks work */

/* Number of joysticks known about & initialised */
int joysticks_supported = 0;

/* The current state of the emulated Kempston joystick */
static libspectrum_byte kempston_value;

/* Bits used by the Kempston joystick */
static const libspectrum_byte KEMPSTON_MASK_RIGHT = 0x01;
static const libspectrum_byte KEMPSTON_MASK_LEFT  = 0x02;
static const libspectrum_byte KEMPSTON_MASK_DOWN  = 0x04;
static const libspectrum_byte KEMPSTON_MASK_UP    = 0x08;
static const libspectrum_byte KEMPSTON_MASK_FIRE  = 0x10;

/* The current state of the emulated Timex joystick */
static libspectrum_byte timex_value;

/* Bits used by the Kempston joystick */
static const libspectrum_byte TIMEX_MASK_RIGHT = 0x08;
static const libspectrum_byte TIMEX_MASK_LEFT  = 0x04;
static const libspectrum_byte TIMEX_MASK_DOWN  = 0x02;
static const libspectrum_byte TIMEX_MASK_UP    = 0x01;
static const libspectrum_byte TIMEX_MASK_FIRE  = 0x80;

char *joystick_name[ JOYSTICK_TYPE_COUNT ] = {
  "None", "Cursor", "Kempston", "Timex"
};

/* Init/shutdown functions. Errors aren't important here */

void
fuse_joystick_init (void)
{
  joysticks_supported = ui_joystick_init();
  kempston_value = timex_value = 0x00;
}

void
fuse_joystick_end (void)
{
  ui_joystick_end();
}

int
joystick_press( int which, joystick_button button, int press )
{
  keyboard_key_name key;
  libspectrum_byte mask;

  if( which ) return 0;

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
      kempston_value |=  mask;
    } else {
      kempston_value &= ~mask;
    }

    return 1;

  case JOYSTICK_TYPE_TIMEX:
    mask = 0x00;		/* Avoid warning */

    switch( button ) {
    case JOYSTICK_BUTTON_UP:    mask = TIMEX_MASK_UP; break;
    case JOYSTICK_BUTTON_DOWN:  mask = TIMEX_MASK_DOWN; break;
    case JOYSTICK_BUTTON_LEFT:  mask = TIMEX_MASK_LEFT; break;
    case JOYSTICK_BUTTON_RIGHT: mask = TIMEX_MASK_RIGHT; break;
    case JOYSTICK_BUTTON_FIRE:  mask = TIMEX_MASK_FIRE; break;
    }

    if( press ) {
      timex_value |=  mask;
    } else {
      timex_value &= ~mask;
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
  return which ? 0x00 : timex_value;
}
