/* keystate.c: keyboard input state machine - prevents problems with pressing
               one "special" key and releasing another (because more than one
               were pressed or shift was released before the key)
   Copyright (c) 2010 Fredrick Meunier

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

   E-mail: fredm@spamcop.net

*/

#include <stdio.h>

#include "input.h"
#include "keyboard.h"
#include "keystate.h"
#include "settings.h"

enum states {
  NONE,
  NORMAL,
  SPECIAL,
  MAX_STATES
} current_state = NONE;

static void action_sNONE_ePRESS_NORMAL( input_key );
static void action_sNONE_ePRESS_SPECIAL( input_key );
static void action_sNORMAL_ePRESS_NORMAL( input_key );
static void action_sNORMAL_eRELEASE_NORMAL( input_key );
static void action_sSPECIAL_ePRESS_NORMAL( input_key );
static void action_sSPECIAL_ePRESS_SPECIAL( input_key );
static void action_sSPECIAL_eRELEASE_SPECIAL( input_key );
static void action_ignore( input_key );

void (*const state_table[MAX_STATES][MAX_EVENTS]) (input_key keysym) = {

  { action_sNONE_ePRESS_NORMAL, action_ignore, action_sNONE_ePRESS_SPECIAL,
    action_ignore }, /* procedures for state NONE */

  { action_sNORMAL_ePRESS_NORMAL, action_sNORMAL_eRELEASE_NORMAL,
    action_ignore, action_ignore }, /* procedures for state NORMAL */

  { action_sSPECIAL_ePRESS_NORMAL, action_ignore,
    action_sSPECIAL_ePRESS_SPECIAL,
    action_sSPECIAL_eRELEASE_SPECIAL }, /* procedures for state SPECIAL */

};

static input_key current_special;
static int normal_count = 0;

static void
press_key( input_key keysym )
{
  input_event_t fuse_event;
  fuse_event.type = INPUT_EVENT_KEYPRESS;
  //fuse_event.types.key.native_key = fuse_keysym;
  fuse_event.types.key.spectrum_key = keysym;
  input_event( &fuse_event );
}

static void
release_key( input_key keysym )
{
  input_event_t fuse_event;
  fuse_event.type = INPUT_EVENT_KEYRELEASE;
  //fuse_event.types.key.native_key = fuse_keysym;
  fuse_event.types.key.spectrum_key = keysym;
  input_event( &fuse_event );
}

void action_ignore( input_key keysym ) {}

void
action_sNONE_ePRESS_NORMAL( input_key keysym )
{
  normal_count++;
  press_key(keysym);
  current_state = NORMAL;
}

void
action_sNONE_ePRESS_SPECIAL( input_key keysym )
{
  current_special = keysym;
  /* override caps and symbol shifts if the user is pressing a special key that
   * may require specific settings of these
   */
  keyboard_release( KEYBOARD_Caps );
  keyboard_release( KEYBOARD_Symbol );
  press_key(keysym);
  current_state = SPECIAL;
}

void
action_sNORMAL_ePRESS_NORMAL( input_key keysym )
{
  normal_count++;
  // track depth, increment no of pressed NORMALs
  press_key(keysym);
}

void
action_sNORMAL_eRELEASE_NORMAL( input_key keysym )
{
  if( normal_count > 0 )
    normal_count--;
  // track depth, if depth is 0 switch back to NONE
  if( !normal_count ) current_state = NONE;
  release_key(keysym);
}

void
action_sSPECIAL_ePRESS_NORMAL( input_key keysym )
{
  normal_count++;
  release_key(current_special);
  current_state = NORMAL;
}

void
action_sSPECIAL_ePRESS_SPECIAL( input_key keysym )
{
  // release old special key, press new one
  release_key(current_special);
  current_special = keysym;
  press_key(current_special);
}

void
action_sSPECIAL_eRELEASE_SPECIAL( input_key keysym )
{
  current_state = NONE;
  release_key(current_special);
}

void
process_keyevent( enum events event, input_key keysym )
{
  if( settings_current.recreated_spectrum ) {
    switch( event ) {
      case PRESS_NORMAL:
      case PRESS_SPECIAL:
        press_key( keysym );
        break;
      case RELEASE_NORMAL:
      case RELEASE_SPECIAL:
        release_key( keysym );
        break;
      default:
        // do nothing
        break;
    }
  } else if(event < MAX_EVENTS && current_state < MAX_STATES) {
    state_table[current_state][event]( keysym ); /* call the action procedure */
  } else {
    /* invalid event/state - shouldn't happen, just ignore for now */
  }
}

void
reset_keystate( void )
{
  current_state = NONE;
  normal_count = 0;
}
