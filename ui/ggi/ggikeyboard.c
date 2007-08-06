/* ggikeyboard.c: Routines for dealing with the GGI keyboard
   Copyright (c) 2003 Catalin Mihaila <catalin@idgrup.ro>

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

   Catalin: catalin@idgrup.ro

*/

#include <config.h>

#include <ggi/ggi.h>

#include "ggi_internals.h"
#include "fuse.h"
#include "keysyms.h"
#include "machine.h"
#include "settings.h"
#include "snapshot.h"
#include "tape.h"
#include "utils.h"
#ifdef USE_WIDGET
#include "widget/widget.h"
#endif                          /* #ifdef USE_WIDGET */

void
ggikeyboard_keypress( int keysym )
{
  const keysyms_key_info *ptr;

  ptr=keysyms_get_data( keysym );
  
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

  switch(keysym) {

    case GIIK_F1:
      fuse_emulation_pause();
      widget_do( WIDGET_TYPE_MENU, &widget_menu_main );
      fuse_emulation_unpause();
      break;

    case GIIK_F2:
      fuse_emulation_pause();
      snapshot_write( "snapshot.z80" );
      fuse_emulation_unpause();
      break;

    case GIIK_F3:
      fuse_emulation_pause();
      widget_do( WIDGET_TYPE_FILESELECTOR, NULL );
      if( widget_filesel_name ) {
	utils_open_file( widget_filesel_name, settings_current.auto_load,
			 NULL );
	free( widget_filesel_name );
	display_refresh_all();
      }
      fuse_emulation_unpause();
      break;

    case GIIK_F4:
      fuse_emulation_pause();
      widget_do( WIDGET_TYPE_GENERAL, NULL );
      fuse_emulation_unpause();
      break;

    case GIIK_F5:
      machine_reset();
      break;

    case GIIK_F6:
      fuse_emulation_pause();
      tape_write( "tape.tzx" );
      fuse_emulation_unpause();
      break;

    case GIIK_F7:
      fuse_emulation_pause();
      widget_apply_to_file( tape_open_default_autoload );
      fuse_emulation_unpause();
      break;

    case GIIK_F8:
      tape_toggle_play();
      break;

    case GIIK_F9:
      fuse_emulation_pause();
      widget_do( WIDGET_TYPE_SELECT, NULL );
      fuse_emulation_unpause();
      break;

    case GIIK_F10:
      fuse_exiting = 1;
      break;
  }	    
}

void
ggikeyboard_keyrelease( int keysym )
{
  const keysyms_key_info *ptr;

  ptr=keysyms_get_data( keysym );
  
  if( ptr ) {
    if( ptr->key1 != KEYBOARD_NONE ) keyboard_release( ptr->key1 );
    if( ptr->key2 != KEYBOARD_NONE ) keyboard_release( ptr->key2 );
  }

}
