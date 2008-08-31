/* wiijoystick.c: routines for dealing with the Wiimote as a joystick

   Copyright (c) 2008 Bjoern Giesler

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

   E-mail: bjoern@giesler.de

*/

#include <config.h>
#include "fuse.h"
#include "input.h"
#include "ui/ui.h"
#include "ui/uijoystick.h"

#define GEKKO
#include <wiiuse/wpad.h>

typedef enum {
  WII_JOYSTICK_SIDEWAYS_RH, /* sideways config (fire on the right) */
  WII_JOYSTICK_SIDEWAYS_LH, /* sideways config (fire on the left) */
  WII_JOYSTICK_NUNCHUK,     /* nunchuk for directions, A+B for fire */
  WII_JOYSTICK_CLASSIC,     /* classic controller */
} WiiJoystickConfig;

int
ui_joystick_init( void )
{
  return 4;
}

void
ui_joystick_end( void )
{
}

void
ui_joystick_poll( void )
{
  input_event_t fuse_event;

  if( fuse_emulation_paused ) return;

#define POST_JOYPRESS(number, pressed) do {	  \
    fuse_event.type = INPUT_EVENT_JOYSTICK_PRESS; \
    fuse_event.types.joystick.which = number; \
    fuse_event.types.joystick.button = pressed; \
    input_event(&fuse_event); \
  } while(0)
  
#define POST_JOYRELEASE(number, pressed) do {	    \
    fuse_event.type = INPUT_EVENT_JOYSTICK_RELEASE; \
    fuse_event.types.joystick.which = number; \
    fuse_event.types.joystick.button = pressed; \
    input_event(&fuse_event); \
  } while(0)

  u32 btnsdown = WPAD_ButtonsDown(0);
  u32 btnsup = WPAD_ButtonsUp(0);
  
  if( btnsdown & WPAD_BUTTON_LEFT )
    POST_JOYPRESS( 0, INPUT_JOYSTICK_DOWN );
  if( btnsdown & WPAD_BUTTON_RIGHT )
    POST_JOYPRESS( 0, INPUT_JOYSTICK_UP );
  if( btnsdown & WPAD_BUTTON_UP )
    POST_JOYPRESS( 0, INPUT_JOYSTICK_LEFT );
  if( btnsdown & WPAD_BUTTON_DOWN )
    POST_JOYPRESS( 0, INPUT_JOYSTICK_RIGHT );

  if( btnsdown & WPAD_BUTTON_1 )
    POST_JOYPRESS( 0, INPUT_JOYSTICK_FIRE_1 );
  if( btnsdown & WPAD_BUTTON_2 )
    POST_JOYPRESS( 0, INPUT_JOYSTICK_FIRE_2 );

  if( btnsup & WPAD_BUTTON_LEFT )
    POST_JOYRELEASE( 0, INPUT_JOYSTICK_DOWN );
  if( btnsup & WPAD_BUTTON_RIGHT )
    POST_JOYRELEASE( 0, INPUT_JOYSTICK_UP );
  if( btnsup & WPAD_BUTTON_UP )
    POST_JOYRELEASE( 0, INPUT_JOYSTICK_LEFT );
  if( btnsup & WPAD_BUTTON_DOWN )
    POST_JOYRELEASE( 0, INPUT_JOYSTICK_RIGHT );

  if( btnsup & WPAD_BUTTON_1 )
    POST_JOYRELEASE( 0, INPUT_JOYSTICK_FIRE_1 );
  if( btnsup & WPAD_BUTTON_2 )
    POST_JOYRELEASE( 0, INPUT_JOYSTICK_FIRE_2 );
}
