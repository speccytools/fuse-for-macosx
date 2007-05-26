/* svgajoystick.c: Joystick emulation (using svgalib)
   Copyright (c) 2003-4 Darren Salt

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

   Darren: linux@youmustbejoking.demon.co.uk

*/

#include <config.h>

#ifdef UI_SVGA			/* Use this iff we're using svgalib */

#if !defined USE_JOYSTICK || defined HAVE_JSW_H

/* Fake joystick, or override UI-specific handling */
#include "../uijoystick.c"

#else			/* #if !defined USE_JOYSTICK || defined HAVE_JSW_H */

/* Use the svgalib joystick support */

#include <string.h>
#include <errno.h>

#include <libspectrum.h>
#include <vgajoystick.h>

#include "fuse.h"
#include "joystick.h"
#include "keyboard.h"
#include "settings.h"
#include "spectrum.h"
#include "machine.h"
#include "ui/ui.h"

static int sticks = 0;
static int buttons[2];

static int
init_stick( int which )
{
  if( !joystick_init( which, JOY_CALIB_STDOUT ) ) {
    ui_error( UI_ERROR_ERROR, "failed to initialise joystick %i: %s",
	      which + 1, errno ? strerror (errno) : "not configured?" );
    return 1;
  }

  if( joystick_getnumaxes( which ) < 2    ||
      joystick_getnumbuttons( which ) < 1    ) {
    joystick_close( which );
    ui_error( UI_ERROR_ERROR, "sorry, joystick %i is inadequate!", which + 1 );
    return 1;
  }

  buttons[which] = joystick_getnumbuttons( which );
  if( buttons[which] > 10 ) buttons[which] = 10;

  return 0;
}

int
ui_joystick_init( void )
{
  /* If we can't init the first, don't try the second */
  if( init_stick( 0 ) ) {
    sticks = 0;
  } else if( init_stick( 1 ) ) {
    sticks = 1;
  } else {
    sticks = 2;
  }

  return sticks;
}

void
ui_joystick_end( void )
{
  joystick_close( -1 );
}

static void
do_axis( int which, int position, input_joystick_button negative,
	 input_joystick_button positive )
{
  input_event_t event1, event2;

  event1.types.joystick.which = event2.types.joystick.which = which;

  event1.types.joystick.button = positive;
  event2.types.joystick.button = negative;

  event1.type = position > 0 ? INPUT_EVENT_JOYSTICK_PRESS :
                               INPUT_EVENT_JOYSTICK_RELEASE;
  event2.type = position < 0 ? INPUT_EVENT_JOYSTICK_PRESS :
                               INPUT_EVENT_JOYSTICK_RELEASE;

  input_event( &event1 );
  input_event( &event2 );
}

static void
do_buttons( int which )
{
  input_event_t event;
  int i;

  event.types.joystick.which = which;
  for( i = 0; i < buttons[which]; i++ ) {
    event.type = joystick_getbutton( which, i )
               ? INPUT_EVENT_JOYSTICK_PRESS
               : INPUT_EVENT_JOYSTICK_RELEASE;
    event.types.joystick.button = INPUT_JOYSTICK_FIRE_1 + i;
    input_event( &event );
  }
}

void
ui_joystick_poll( void )
{
  int i;

  joystick_update();

  for( i = 0; i < sticks; i++ ) {
    do_axis( i, joystick_x( i ), INPUT_JOYSTICK_LEFT, INPUT_JOYSTICK_RIGHT );
    do_axis( i, joystick_y( i ), INPUT_JOYSTICK_UP,   INPUT_JOYSTICK_DOWN  );
    do_buttons( i );
  }
}

#endif			/* #if !defined USE_JOYSTICK || defined HAVE_JSW_H */

#endif			/* #ifdef UI_SVGA */
