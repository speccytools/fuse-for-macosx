/* sdlkeyboard.c: routines for dealing with the SDL keyboard
   Copyright (c) 2000-2002 Philip Kendall, Matan Ziv-Av, Fredrick Meunier

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

*/

#include <config.h>

#ifdef UI_SDL			/* Use this iff we're using SDL */

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

#include "display.h"
#include "fuse.h"
#include "keysyms.h"
#include "machine.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"
#ifdef USE_WIDGET
#include "widget/widget.h"
#endif				/* #ifdef USE_WIDGET */
#include "sdlkeyboard.h"

void
sdlkeyboard_keypress( SDL_KeyboardEvent *keyevent )
{
  const keysyms_key_info *ptr;

  ptr=keysyms_get_data( keyevent->keysym.sym );

  if( ptr ) {
    if( widget_level >= 0 ) {
      widget_keyhandler( ptr->key1, ptr->key2 );
    } else {
      if( ptr->key1 != KEYBOARD_NONE ) keyboard_press( ptr->key1 );
      if( ptr->key2 != KEYBOARD_NONE ) keyboard_press( ptr->key2 );
    }
    return;
  }

  if( widget_level >= 0 ) return;
  
  /* Now deal with the non-Speccy keys */
  switch(keyevent->keysym.sym) {
  case SDLK_F1:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_MENU, &widget_menu_main );
    fuse_emulation_unpause();
    break;
  case SDLK_F2:
    fuse_emulation_pause();
    snapshot_write( "snapshot.z80" );
    fuse_emulation_unpause();
    break;
  case SDLK_F3:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_FILESELECTOR, NULL );
    if( widget_filesel_name ) {
      snapshot_read( widget_filesel_name );
      free( widget_filesel_name );
      display_refresh_all();
    }
    fuse_emulation_unpause();
    break;
  case SDLK_F4:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_GENERAL, NULL );
    fuse_emulation_unpause();
    break;
  case SDLK_F5:
    machine_reset();
    break;
  case SDLK_F6:
    fuse_emulation_pause();
    tape_write( "tape.tzx" );
    fuse_emulation_unpause();
    break;
  case SDLK_F7:
    fuse_emulation_pause();
    widget_apply_to_file( tape_open_default_autoload );
    fuse_emulation_unpause();
    break;
  case SDLK_F8:
    tape_toggle_play();
    break;
  case SDLK_F9:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_SELECT, NULL );
    fuse_emulation_unpause();
    break;
  case SDLK_F10:
    fuse_exiting = 1;
    break;
  default:
    break;
  }

  return;

}

void
sdlkeyboard_keyrelease( SDL_KeyboardEvent *keyevent )
{
  const keysyms_key_info *ptr;

  ptr = keysyms_get_data( keyevent->keysym.sym );

  if(ptr) {
    if( ptr->key1 != KEYBOARD_NONE ) keyboard_release( ptr->key1 );
    if( ptr->key2 != KEYBOARD_NONE ) keyboard_release( ptr->key2 );
  }
  
  return;

}

#endif			/* #ifdef UI_SDL */
