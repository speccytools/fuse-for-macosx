/* uijoystick.c: Joystick emulation (using libjsw)
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

/* Usage note: build this from within a specific UI unless that UI implements
 * its own joystick support using some other library.
 * Inclusion as follows:
 * #if !defined USE_JOYSTICK || defined HAVE_JSW_H
 * # include "../uijoystick.c"
 * #else
 *  // UI-specific code implementing the following (exported) functions
 * #endif
 */

#include <config.h>

#include "../joystick.h"
#include "joystick.h"

#if defined USE_JOYSTICK && defined HAVE_JSW_H

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <jsw.h>

#include <libspectrum.h>

#include "fuse.h"
#include "keyboard.h"
#include "settings.h"
#include "spectrum.h"
#include "machine.h"
#include "ui/ui.h"

static js_data_struct jsd[2];


static int
init_stick( int which, const char *const device,
	    const char *const calibration )
{
  switch( JSInit( &jsd[which], device, calibration, JSFlagNonBlocking ) ) {

  case JSSuccess:
    if( JSLoadCalibrationUNIX( &jsd[which] ) && errno != ENOENT ) {
      ui_error( UI_ERROR_ERROR,
		"failed to read calibration for joystick %i: %s", which + 1,
		strerror( errno ) );
      break;
    }

    if( jsd[which].total_axises < 2 || jsd[which].total_buttons < 1 )
    {
      ui_error( UI_ERROR_ERROR, "sorry, joystick %i (%s) is inadequate!",
		which + 1, device );
      break;
    }
    return 0;

  case JSBadValue:
    ui_error( UI_ERROR_ERROR, "failed to initialise joystick %i: %s",
	      which + 1, "invalid parameter/value");
    break;

  case JSNoAccess:

    /* FIXME: why is this commented out? */
/*
    ui_error (UI_ERROR_ERROR,
	      "failed to initialise joystick %i: %s",
	      which + 1, "cannot access device");
*/
    break;

  case JSNoBuffers:
    ui_error( UI_ERROR_ERROR, "failed to initialise joystick %i: %s",
	      which + 1, "not enough memory" );
    break;

  default:
    ui_error( UI_ERROR_ERROR, "failed to initialise joystick %i", which + 1 );
    break;

  }

  JSClose( &jsd[which] );

  return 1;
}


int
ui_joystick_init( void )
{
  const char *home;
  char *calibration;

  home = utils_get_home_path(); if( !home ) return 1;

  /* Default calibration file is ~/.joystick */
  calibration = malloc( strlen( home ) + strlen( JSDefaultCalibration ) + 2 );

  if( !calibration ) {
    ui_error( UI_ERROR_ERROR, "failed to initialise joystick: %s",
	      "not enough memory" );
    return 1;
  }

  sprintf( calibration, "%s/%s", home, JSDefaultCalibration );

  /* If we can't init the first, don't try the second */
  if( init_stick( 0, "/dev/js0", calibration ) ) return 0;
  if( init_stick( 1, "/dev/js1", calibration ) ) return 1;
  return 2;
}


int
ui_joystick_end( void )
{
  int i;
  for( i = 0; i < joysticks_supported; i++ ) JSClose( &jsd[i] );
  return 0;
}


libspectrum_byte
ui_joystick_read( libspectrum_word port, libspectrum_byte which )
{
  libspectrum_byte ret = 0;

  JSUpdate( &jsd[which] );

         if( jsd[which].axis[0]->cur >
	     jsd[which].axis[0]->cen + jsd[which].axis[0]->nz ) {
    ret = 1; /* right */
  } else if( jsd[which].axis[0]->cur <
	     jsd[which].axis[0]->cen - jsd[which].axis[0]->nz ) {
    ret = 2; /* left */
  }

       if( jsd[which].axis[1]->cur >
	   jsd[which].axis[1]->cen + jsd[which].axis[1]->nz ) {
    ret |= 4; /* down */
  else if( jsd[which].axis[1]->cur <
	   jsd[which].axis[1]->cen - jsd[which].axis[1]->nz ) {
    ret |= 8; /* up */
  }

  if( jsd[which].button[0]->state == JSButtonStateOn ) ret |= 16; /* fire */

  return ret;
}

#else			/* #if defined USE_JOYSTICK && defined HAVE_JSW_H */

/* No joystick library */

int
ui_joystick_init( void )
{
  return 0;
}

int
ui_joystick_end( void )
{
  return 0;
}

libspectrum_byte
ui_joystick_read(libspectrum_word port, libspectrum_byte which)
{
  return joystick_default_read( port, which );
}

#endif			/* #if defined USE_JOYSTICK && defined HAVE_JSW_H */
