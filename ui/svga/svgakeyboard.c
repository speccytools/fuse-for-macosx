/* svgakeyboard.c: svgalib routines for dealing with the keyboard
   Copyright (c) 2000-2002 Philip Kendall, Matan Ziv-Av

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

#ifdef UI_SVGA			/* Use this iff we're using svgalib */

#include <stdio.h>

#include <vga.h>
#include <vgakeyboard.h>

#include "display.h"
#include "fuse.h"
#include "keyboard.h"
#include "machine.h"
#include "settings.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"
#include "widget/widget.h"

static void svgakeyboard_keystroke(int scancode, int press);
static void svgakeyboard_keypress(int keysym);
static void svgakeyboard_keyrelease(int keysym);

int svgakeyboard_init(void)
{
  keyboard_init();
  keyboard_seteventhandler(svgakeyboard_keystroke);
  return 0;
}

static void svgakeyboard_keystroke(int scancode, int press)  {
  if(press) {
    svgakeyboard_keypress(scancode);
  } else {
    svgakeyboard_keyrelease(scancode);
  }
}

static void svgakeyboard_keypress(int keysym)
{
  keysyms_key_info *ptr;

  ptr=keysyms_get_data(keysym);

  if( ptr ) {

    if( widget_level >= 0 ) {
      widget_keyhandler( ptr->key1, ptr2->key2 );
    } else {
      if(ptr->key1 != KEYBOARD_NONE) keyboard_press(ptr->key1);
      if(ptr->key2 != KEYBOARD_NONE) keyboard_press(ptr->key2);
    }
    return;
  }

  if( widget_level >= 0 ) return;

  /* Now deal with the non-Speccy keys */
  switch(keysym) {
  case SCANCODE_F1:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_MENU, &widget_menu_main );
    fuse_emulation_unpause();
    break;
  case SCANCODE_F2:
    fuse_emulation_pause();
    snapshot_write( "snapshot.z80" );
    fuse_emulation_unpause();
    break;
  case SCANCODE_F3:
    fuse_emulation_pause();
    widget_apply_to_file( snapshot_read );
    fuse_emulation_unpause();
    break;
  case SCANCODE_F4:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_GENERAL, NULL );
    fuse_emulation_unpause();
    break;
  case SCANCODE_F5:
    machine_current->reset();
    break;
  case SCANCODE_F6:
    fuse_emulation_pause();
    tape_write( "tape.tzx" );
    fuse_emulation_unpause();
    break;
  case SCANCODE_F7:
    fuse_emulation_pause();
    widget_apply_to_file( tape_open );
    fuse_emulation_unpause();
    break;
  case SCANCODE_F8:
    tape_toggle_play();
    break;
  case SCANCODE_F9:
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_SELECT, NULL );
    fuse_emulation_unpause();
    break;
  case SCANCODE_F10:
    fuse_exiting=1;
    break;
  }

  return;
}

static void svgakeyboard_keyrelease(int keysym)
{
  keysyms_key_info *ptr;

  ptr=keysyms_get_data(keysym);

  if(ptr) {
    if(ptr->key1 != KEYBOARD_NONE) keyboard_release(ptr->key1);
    if(ptr->key2 != KEYBOARD_NONE) keyboard_release(ptr->key2);
  }

  return;

}

int svgakeyboard_end(void)
{
  keyboard_close();
  return 0;
}

#endif				/* #ifdef UI_SVGA */
