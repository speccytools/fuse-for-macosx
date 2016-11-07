/* cocoajoystick.c: routines for dealing with the Cocoa joystick
   Copyright (c) 2006 Fredrick Meunier

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

*/

#include <config.h>

#include <libspectrum.h>

#include "SDL_joystick/SDL_sysjoystick.h"

#include "compat.h"
#include "input.h"
#include "settings.h"
#include "ui.h"
#include "ui/uijoystick.h"

#define MAX_BUTTONS 15

typedef struct {
    SDL_Joystick *joystick;
    int num_axes;
    Uint8 button_states[MAX_BUTTONS];
} cocoajoystick_info_t;

typedef struct {
    cocoajoystick_info_t joystick1;
    cocoajoystick_info_t joystick2;
} cocoajoysticks_info_t;

static cocoajoysticks_info_t cocoajoysticks_info;

static void
cocoajoystickinfo_reset( cocoajoystick_info_t* joystick )
{
  int i;
  joystick->joystick = NULL;
  joystick->num_axes = 0;
  for( i=0; i<MAX_BUTTONS; i++) {
    joystick->button_states[i] = 0;
  }
}

static void
cocoajoystick_info_init( int* joystick_number,
                         cocoajoystick_info_t* joystick_info )
{
  if( *joystick_number ) {
    if( ( joystick_info->joystick =
            SDL_JoystickOpen( *joystick_number - 1) ) == NULL ) {
      return;
    }

    joystick_info->num_axes = SDL_JoystickNumAxes( joystick_info->joystick );
  }
}

int
ui_joystick_init( void )
{
  int error = SDL_JoystickInit();
  if ( error ) {
    ui_error( UI_ERROR_ERROR, "failed to initialise joystick subsystem" );
    return 0;
  }

  cocoajoystickinfo_reset( &cocoajoysticks_info.joystick1 );
  cocoajoystickinfo_reset( &cocoajoysticks_info.joystick2 );

  cocoajoystick_info_init( &settings_current.joy1_number,
                           &cocoajoysticks_info.joystick1 );
  cocoajoystick_info_init( &settings_current.joy2_number,
                           &cocoajoysticks_info.joystick2 );

  return SDL_NumJoysticks();
}

static void
do_axis( int which, Sint16 value,
	 input_key negative, input_key positive )
{
  input_event_t event1, event2;

  event1.types.joystick.which = event2.types.joystick.which = which;

  event1.types.joystick.button = negative;
  event2.types.joystick.button = positive;

  if( value > 16384 ) {
    event1.type = INPUT_EVENT_JOYSTICK_RELEASE;
    event2.type = INPUT_EVENT_JOYSTICK_PRESS;
  } else if( value < -16384 ) {
    event1.type = INPUT_EVENT_JOYSTICK_PRESS;
    event2.type = INPUT_EVENT_JOYSTICK_RELEASE;
  } else {
    event1.type = INPUT_EVENT_JOYSTICK_RELEASE;
    event2.type = INPUT_EVENT_JOYSTICK_RELEASE;
  }

  input_event( &event1 );
  input_event( &event2 );
}

static void
poll_joystick(  int which, cocoajoystick_info_t* joystick_info,
                int xaxis, int yaxis )
{
  Sint16 status;
  Uint8 fire;
  int buttons, i;
  input_event_t event;

  if( SDL_JoystickNumAxes( joystick_info->joystick ) <= xaxis ||
      SDL_JoystickNumAxes( joystick_info->joystick ) <= yaxis ) {
    return;
  }
  
  status = SDL_JoystickGetAxis( joystick_info->joystick, xaxis );
  do_axis( which, status, INPUT_JOYSTICK_LEFT, INPUT_JOYSTICK_RIGHT );

  status = SDL_JoystickGetAxis( joystick_info->joystick, yaxis );
  do_axis( which, status, INPUT_JOYSTICK_UP, INPUT_JOYSTICK_DOWN );

  event.types.joystick.which = which;

  buttons = SDL_JoystickNumButtons( joystick_info->joystick );
  /* We support 'only' MAX_BUTTONS fire buttons */
  if( buttons > MAX_BUTTONS ) buttons = MAX_BUTTONS;

  for( i = 0; i < buttons; i++ ) {
    fire = SDL_JoystickGetButton( joystick_info->joystick, i );
    if( fire != joystick_info->button_states[i] ) {
      joystick_info->button_states[i] = fire;
      event.type = fire ? INPUT_EVENT_JOYSTICK_PRESS :
                          INPUT_EVENT_JOYSTICK_RELEASE;
      event.types.joystick.button = INPUT_JOYSTICK_FIRE_1 + i;
      input_event( &event );
    }
  }
}

void
ui_joystick_poll( void )
{
  if( settings_current.joy1_number || settings_current.joy2_number ) {
    SDL_JoystickUpdate();
    if( cocoajoysticks_info.joystick1.joystick != NULL )
      poll_joystick( 0, &cocoajoysticks_info.joystick1,
                     settings_current.joy1_xaxis, settings_current.joy1_yaxis );
    if( cocoajoysticks_info.joystick2.joystick != NULL )
      poll_joystick( 1, &cocoajoysticks_info.joystick2,
                     settings_current.joy2_xaxis, settings_current.joy2_yaxis );
  }
}

static void
cocoajoystick_info_end( cocoajoystick_info_t* joystick_info )
{
  if( joystick_info->joystick != NULL ) {
    SDL_JoystickClose( joystick_info->joystick );
    cocoajoystickinfo_reset( joystick_info );
  }
}

void
ui_joystick_end( void )
{
  cocoajoystick_info_end( &cocoajoysticks_info.joystick1 );
  cocoajoystick_info_end( &cocoajoysticks_info.joystick2 );

  SDL_JoystickQuit();
}
