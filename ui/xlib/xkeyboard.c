/* xkeyboard.c: X routines for dealing with the keyboard
   Copyright (c) 2000-2002 Philip Kendall

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

#ifdef UI_X			/* Use this iff we're using Xlib */

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "display.h"
#include "fuse.h"
#include "keyboard.h"
#include "keysyms.h"
#include "machine.h"
#include "settings.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"
#include "widget/widget.h"

int xkeyboard_keypress(XKeyEvent *event)
{
  KeySym keysym; keysyms_key_info *ptr;

  keysym=XLookupKeysym(event,0);

  ptr=keysyms_get_data(keysym);

  if( ptr ) {

    if( widget_level >= 0 ) {
      widget_keyhandler( ptr->key1 );
    } else {
      if(ptr->key1 != KEYBOARD_NONE) keyboard_press(ptr->key1);
      if(ptr->key2 != KEYBOARD_NONE) keyboard_press(ptr->key2);
    }

    return 0;
  }

  if( widget_level >= 0 ) return 0;

  /* Now deal with the non-Speccy keys */
  switch(keysym) {
  case XK_F1:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_MENU, &widget_menu_main );
    fuse_emulation_unpause();
    break;
  case XK_F2:
    fuse_emulation_pause();
    snapshot_write( "snapshot.z80" );
    fuse_emulation_unpause();
    break;
  case XK_F3:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_FILESELECTOR, NULL );
    if( widget_filesel_name ) {
      snapshot_read( widget_filesel_name );
      display_refresh_all();
    }
    fuse_emulation_unpause();
    break;
  case XK_F4:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_GENERAL, NULL );
    fuse_emulation_unpause();
    break;
  case XK_F5:
    machine_current->reset();
    break;
  case XK_F6:
    fuse_emulation_pause();
    tape_write( "tape.tzx" );
    fuse_emulation_unpause();
    break;
  case XK_F7:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_FILESELECTOR, NULL );
    if( widget_filesel_name ) tape_open( widget_filesel_name );
    fuse_emulation_unpause();
    break;
  case XK_F8:
    tape_toggle_play();
    break;
  case XK_F9:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_SELECT, NULL );
    fuse_emulation_unpause();
    break;
  case XK_F10:
    return 1;
  }

  return 0;

}

void xkeyboard_keyrelease(XKeyEvent *event)
{
  KeySym keysym; keysyms_key_info *ptr;

  keysym=XLookupKeysym(event,0);

  ptr=keysyms_get_data(keysym);

  if(ptr) {
    if(ptr->key1 != KEYBOARD_NONE) keyboard_release(ptr->key1);
    if(ptr->key2 != KEYBOARD_NONE) keyboard_release(ptr->key2);
  }

  return;

}

#endif				/* #ifdef UI_X */
