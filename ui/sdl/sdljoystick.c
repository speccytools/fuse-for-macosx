/* sdljoystick.c: routines for dealing with the SDL joystick
   Copyright (c) 2003-2004 Darren Salt, Fredrick Meunier, Philip Kendall

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
   Foundation, Inc., 49 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

   Fred: fredm@spamcop.net
*/

#include <config.h>

#ifdef UI_SDL			/* Use this iff we're using SDL */

#if !defined USE_JOYSTICK || defined HAVE_JSW_H
/* Fake joystick, or override UI-specific handling */
#include "../uijoystick.c"

#else			/* #if !defined USE_JOYSTICK || defined HAVE_JSW_H */

#include <SDL.h>

#include "compat.h"
#include "joystick.h"
#include "sdljoystick.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uijoystick.h"

static SDL_Joystick *joystick1 = NULL;
static SDL_Joystick *joystick2 = NULL;

int
ui_joystick_init( void )
{
  int retval = SDL_NumJoysticks();

  SDL_InitSubSystem( SDL_INIT_JOYSTICK );

  if( retval >= 2 ) {

    retval = 2;

    if( ( joystick2 = SDL_JoystickOpen( 1 ) ) == NULL ) {
      ui_error( UI_ERROR_ERROR, "failed to initialise joystick 2" );
      return 0;
    }

    if( SDL_JoystickNumAxes( joystick2 ) < 2    ||
        SDL_JoystickNumButtons( joystick2 ) < 1    ) {
      ui_error( UI_ERROR_ERROR, "sorry, joystick 2 is inadequate!" );
      return 0;
    }

  }

  if( retval > 0 ) {

    if( ( joystick1 = SDL_JoystickOpen( 0 ) ) == NULL ) {
      ui_error( UI_ERROR_ERROR, "failed to initialise joystick 1" );
      return 0;
    }
 
    if( SDL_JoystickNumAxes( joystick1 ) < 2    ||
        SDL_JoystickNumButtons( joystick1 ) < 1    ) {
      ui_error( UI_ERROR_ERROR, "sorry, joystick 1 is inadequate!" );
      return 0;
    }
  }

  SDL_JoystickEventState( SDL_ENABLE );

  return retval;
}

void
sdljoystick_buttonpress( SDL_JoyButtonEvent *buttonevent )
{
  joystick_press( buttonevent->which, JOYSTICK_BUTTON_FIRE, 1 );
}

void
sdljoystick_buttonrelease( SDL_JoyButtonEvent *buttonevent )
{
  joystick_press( buttonevent->which, JOYSTICK_BUTTON_FIRE, 0 );
}

void
sdljoystick_axismove( SDL_JoyAxisEvent *axisevent )
{
  int which, axis;

  which = axisevent->which;
  axis = axisevent->axis;

  if( axis == 0 ) {

    if( axisevent->value > 16384 ) { /* right */
      joystick_press( which, JOYSTICK_BUTTON_LEFT,  0 );
      joystick_press( which, JOYSTICK_BUTTON_RIGHT, 1 );
    } else if( axisevent->value < -16384 ) { /* left */
      joystick_press( which, JOYSTICK_BUTTON_LEFT,  1 );
      joystick_press( which, JOYSTICK_BUTTON_RIGHT, 0 );
    } else { /* centered */
      joystick_press( which, JOYSTICK_BUTTON_LEFT,  0 );
      joystick_press( which, JOYSTICK_BUTTON_RIGHT, 0 );
    }

  } else if( axis == 1 ) {

    if( axisevent->value > 16384 ) { /* down */
      joystick_press( which, JOYSTICK_BUTTON_UP,   0 );
      joystick_press( which, JOYSTICK_BUTTON_DOWN, 1 );
    } else if( axisevent->value < -16384 ) { /* up */
      joystick_press( which, JOYSTICK_BUTTON_UP,   1 );
      joystick_press( which, JOYSTICK_BUTTON_DOWN, 0 );
    } else { /* centered */
      joystick_press( which, JOYSTICK_BUTTON_UP,   0 );
      joystick_press( which, JOYSTICK_BUTTON_DOWN, 0 );
    }

  }
}

void
ui_joystick_end( void )
{
  if( joystick1 != NULL || joystick2 != NULL ) {

    SDL_JoystickEventState( SDL_IGNORE );
    if( joystick1 != NULL ) SDL_JoystickClose( joystick1 );
    if( joystick2 != NULL ) SDL_JoystickClose( joystick2 );
    joystick1 = NULL;
    joystick2 = NULL;

  }

  SDL_QuitSubSystem( SDL_INIT_JOYSTICK );
}

#endif			/* #if !defined USE_JOYSTICK || defined HAVE_JSW_H */

#endif			/* #ifdef UI_SDL */
