/* svgajoystick.c: Joystick emulation (using svgalib)
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

static int
init_stick( int which )
{
  if( !joystick_init( which, JOY_CALIB_STDOUT ) )
  {
    ui_error( UI_ERROR_ERROR, "failed to initialise joystick %i: %s",
	      which + 1, errno ? strerror (errno) : "not configured?" );
    return 1;
  }

  if( joystick_getnumaxes( which ) < 2 || joystick_getnumbuttons( which ) < 1 )
  {
    joystick_close( which );
    ui_error( UI_ERROR_ERROR, "sorry, joystick %i is inadequate!", which + 1 );
    return 1;
  }

  return 0;
}

int
ui_joystick_init( void )
{
  /* If we can't init the first, don't try the second */
  if( init_stick( 0 ) ) return 0;
  if( init_stick( 1 ) ) return 1;
  return 2;
}

int
ui_joystick_end( void )
{
  joystick_close( -1 );
  return 0;
}

libspectrum_byte
ui_joystick_read( libspectrum_word port, libspectrum_byte which )
{
  libspectrum_byte ret = 0;
  int x, y;

  joystick_update();

  x = joystick_x( which );
  y = joystick_y( which );

       if( x > 0 ) ret |= 1; /* right */
  else if( x < 0 ) ret |= 2; /* left */

       if( y > 0 ) ret |= 4; /* down */
  else if( y < 0 ) ret |= 8; /* up */

  if( joystick_button1( which ) ) ret |= 16; /* fire */

  return ret;
}

#endif			/* #if !defined USE_JOYSTICK || defined HAVE_JSW_H */

#endif			/* #ifdef UI_SVGA */
