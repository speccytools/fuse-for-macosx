/* win32keyboard.c: routines for dealing with the Win32 keyboard
   Copyright (c) 2003 Marek Januszewski

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

#ifdef UI_WIN32			/* Use this iff we're using UI_WIN32 */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "display.h"
#include "fuse.h"
#include "keysyms.h"
#include "machine.h"
#include "settings.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"
#include "utils.h"
#ifdef USE_WIDGET
#include "widget/widget.h"
#endif				/* #ifdef USE_WIDGET */
#include "win32keyboard.h"

void
win32keyboard_keypress( WPARAM wParam, LPARAM lParam )
{
  const keysyms_key_info *ptr;

  ptr=keysyms_get_data( wParam );

  if( ptr ) {
#ifdef USE_WIDGET
    if( widget_level >= 0 ) {
      widget_keyhandler( ptr->key1, ptr->key2 );
    } else {
#endif
      if( ptr->key1 != KEYBOARD_NONE ) keyboard_press( ptr->key1 );
      if( ptr->key2 != KEYBOARD_NONE ) keyboard_press( ptr->key2 );
#ifdef USE_WIDGET
    }
#endif
    return;
  }

#ifdef USE_WIDGET

 if( widget_level >= 0 ) return;
  
  /* Now deal with the non-Speccy keys */

  switch( wParam ) {
  case VK_F1:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_MENU, &widget_menu_main );
    fuse_emulation_unpause();
    break;
  case VK_F2:
    fuse_emulation_pause();
    snapshot_write( "snapshot.z80" );
    fuse_emulation_unpause();
    break;
  case VK_F3:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_FILESELECTOR, NULL );
    if( widget_filesel_name ) {
      utils_open_file( widget_filesel_name, settings_current.auto_load, NULL );
      free( widget_filesel_name );
      display_refresh_all();
    }
    fuse_emulation_unpause();
    break;
  case VK_F4:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_GENERAL, NULL );
    fuse_emulation_unpause();
    break;
  case VK_F5:
    machine_reset();
    break;
  case VK_F6:
    fuse_emulation_pause();
    tape_write( "tape.tzx" );
    fuse_emulation_unpause();
    break;
  case VK_F7:
    fuse_emulation_pause();
    widget_apply_to_file( tape_open_default_autoload );
    fuse_emulation_unpause();
    break;
  case VK_F8:
    tape_toggle_play();
    break;
  case VK_F9:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_SELECT, NULL );
    fuse_emulation_unpause();
    break;
  case VK_F10:
    fuse_exiting = 1;
    break;
  default:
    break;
  }

#endif

  return;
}

void
win32keyboard_keyrelease( WPARAM wParam, LPARAM lParam )
{
  const keysyms_key_info *ptr;

  ptr = keysyms_get_data( wParam );

  if(ptr) {
    if( ptr->key1 != KEYBOARD_NONE ) keyboard_release( ptr->key1 );
    if( ptr->key2 != KEYBOARD_NONE ) keyboard_release( ptr->key2 );
  }
  
  return;

}

#endif			/* #ifdef UI_WIN32 */
