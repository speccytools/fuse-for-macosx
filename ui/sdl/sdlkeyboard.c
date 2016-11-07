/* sdlkeyboard.c: routines for dealing with the SDL keyboard
   Copyright (c) 2000-2005 Philip Kendall, Matan Ziv-Av, Fredrick Meunier

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

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

#include "display.h"
#include "fuse.h"
#include "keyboard.h"
#include "machine.h"
#include "sdlkeyboard.h"
#include "sdlui.h"
#include "settings.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"
#include "ui/ui.h"
#include "utils.h"

#include "FuseMenus.h"

extern keysyms_map_t modifier_keysyms_map[];
extern keysyms_map_t unicode_keysyms_map[];

static GHashTable *modifier_keysyms_hash;
static GHashTable *unicode_keysyms_hash;

static input_key
other_keysyms_remap( libspectrum_dword ui_keysym, GHashTable *hash )
{
  const input_key *ptr;

  ptr = g_hash_table_lookup( hash, &ui_keysym );

  return ptr ? *ptr : INPUT_KEY_NONE;
}

void
sdlkeyboard_init(void)
{
  keysyms_map_t *ptr3;

  modifier_keysyms_hash = g_hash_table_new( g_int_hash, g_int_equal );

  for( ptr3 = modifier_keysyms_map; ptr3->ui; ptr3++ )
        g_hash_table_insert( modifier_keysyms_hash, &( ptr3->ui ),
                             &( ptr3->fuse ) );

  unicode_keysyms_hash = g_hash_table_new( g_int_hash, g_int_equal );

  for( ptr3 = (keysyms_map_t *)unicode_keysyms_map; ptr3->ui; ptr3++ )
    g_hash_table_insert( unicode_keysyms_hash, &( ptr3->ui ),
                         &( ptr3->fuse ) );

  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
}

void
sdlkeyboard_end(void)
{
  g_hash_table_destroy( unicode_keysyms_hash );
  g_hash_table_destroy( modifier_keysyms_hash );
}

static int sdlkeyboard_caps_shift_pressed = 0;
static int sdlkeyboard_symbol_shift_pressed = 0;
static input_key unicode_keysym = INPUT_KEY_NONE;

void
sdlkeyboard_keypress( SDL_KeyboardEvent *keyevent )
{
  input_key fuse_keysym;
  input_event_t fuse_event;
  
  /* Deal with the non-Speccy keys */
  if( keyevent->keysym.sym == SDLK_ESCAPE && settings_current.full_screen ) {
    settings_current.full_screen = 0;
    return;
  }

  fuse_keysym = keysyms_remap( keyevent->keysym.sym );

  /* Currently unicode_keysyms_map contains ASCII character keys */
  if( ( keyevent->keysym.unicode & 0xFF80 ) == 0 ) 
    unicode_keysym = unicode_keysyms_remap( keyevent->keysym.unicode );
  else
    unicode_keysym = INPUT_KEY_NONE;

  if( fuse_keysym == INPUT_KEY_NONE && unicode_keysym == INPUT_KEY_NONE )
    return;
  }

  fuse_keysym = other_keysyms_remap( keyevent->keysym.sym,
                                     modifier_keysyms_hash );
  if( fuse_keysym == INPUT_KEY_NONE ) 
    fuse_keysym = keysyms_remap( keyevent->keysym.scancode );
  if( fuse_keysym == INPUT_KEY_NONE ) {
    fuse_keysym = other_keysyms_remap( keyevent->keysym.unicode,
                                       unicode_keysyms_hash );
    if( fuse_keysym != INPUT_KEY_NONE ) {
      unicode_keysym = fuse_keysym;

      /* record current values of caps and symbol shift. We will temoprarily
       * override these for the duration of the unicoded simulated keypresses
       */
      if( ( sdlkeyboard_caps_shift_pressed = keyboard_state( KEYBOARD_Caps ) ) )
      {
        keyboard_release( KEYBOARD_Caps );
      }
      if( ( sdlkeyboard_symbol_shift_pressed =
            keyboard_state( KEYBOARD_Symbol ) ) ) {
        keyboard_release( KEYBOARD_Symbol );
      }
    }
  }
  
  if( fuse_keysym == INPUT_KEY_NONE ) return;
  
  fuse_event.type = INPUT_EVENT_KEYPRESS;
  if( unicode_keysym == INPUT_KEY_NONE )
    fuse_event.types.key.native_key = fuse_keysym;
  else
    fuse_event.types.key.native_key = unicode_keysym;
  fuse_event.types.key.spectrum_key = fuse_keysym;

  input_event( &fuse_event );
}

void
sdlkeyboard_keyrelease( SDL_KeyboardEvent *keyevent )
{
  input_key fuse_keysym;
  input_event_t fuse_event;

  fuse_keysym = other_keysyms_remap( keyevent->keysym.sym,
                                     modifier_keysyms_hash );
  if( fuse_keysym == INPUT_KEY_NONE ) {
    fuse_keysym = keysyms_remap( keyevent->keysym.scancode );
  }
  if( fuse_keysym == INPUT_KEY_NONE ) {
    fuse_keysym = other_keysyms_remap( keyevent->keysym.unicode,
                                         unicode_keysyms_hash );
    if( fuse_keysym != INPUT_KEY_NONE && unicode_keysym != INPUT_KEY_NONE ) {
        input_key temp = fuse_keysym;
        fuse_keysym = unicode_keysym;
        unicode_keysym = temp;
        sdlkeyboard_caps_shift_pressed = 0;
        sdlkeyboard_symbol_shift_pressed = 0;
    } else {
      /* Unicode key released as normal */
      unicode_keysym = INPUT_KEY_NONE;
    }
  }

  if( fuse_keysym == INPUT_KEY_NONE ) return;

  fuse_event.type = INPUT_EVENT_KEYRELEASE;
  /* SDL doesn't provide key release information for UNICODE, assuming that
     the values will just be used for dialog boxes et. al. so put in SDL keysym
     equivalent and hope for the best */
  fuse_event.types.key.native_key = fuse_keysym;
  fuse_event.types.key.spectrum_key = fuse_keysym;

  input_event( &fuse_event );

  if( sdlkeyboard_caps_shift_pressed )
  {
    sdlkeyboard_caps_shift_pressed = 0;
    keyboard_press( KEYBOARD_Caps );
  }
  if( sdlkeyboard_symbol_shift_pressed ) {
    sdlkeyboard_symbol_shift_pressed = 0;
    keyboard_press( KEYBOARD_Symbol );
  }
  if( unicode_keysym != INPUT_KEY_NONE ) {
    fuse_event.types.key.spectrum_key = unicode_keysym;
    input_event( &fuse_event );
    unicode_keysym = INPUT_KEY_NONE;
  }
}
