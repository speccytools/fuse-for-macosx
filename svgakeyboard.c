/* svgakeyboard.c: svgalib routines for dealing with the keyboard
   Copyright (c) 2000-2001 Philip Kendall, Matan Ziv-Av

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
#include "keysyms.h"
#include "machine.h"
#include "settings.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"

void svgakeyboard_keystroke(int scancode, int press);
int svgakeyboard_keypress(int keysym);
void svgakeyboard_keyrelease(int keysym);


int svgakeyboard_init(void)
{
  keyboard_init();
  keyboard_seteventhandler(svgakeyboard_keystroke);
  return 0;
}

void svgakeyboard_keystroke(int scancode, int press)  {
  if(press) {
    svgakeyboard_keypress(scancode);
  } else {
    svgakeyboard_keyrelease(scancode);
  }
}

int svgakeyboard_keypress(int keysym)
{
  keysyms_key_info *ptr;

  ptr=keysyms_get_data(keysym);

  if(ptr) {
    if(ptr->key1 != KEYBOARD_NONE) keyboard_press(ptr->key1);
    if(ptr->key2 != KEYBOARD_NONE) keyboard_press(ptr->key2);
    return 0;
  }

  /* Now deal with the non-Speccy keys */
  switch(keysym) {
  case SCANCODE_F2:
    fuse_emulation_pause();
    snapshot_write( "snapshot.z80" );
    fuse_emulation_unpause();
    break;
  case SCANCODE_F3:
    fuse_emulation_pause();
    snapshot_read( "snapshot.z80" );
    display_refresh_all();
    fuse_emulation_unpause();
    break;
  case SCANCODE_F5:
    machine_current->reset();
    break;
  case SCANCODE_F7:
    fuse_emulation_pause();
    tape_open( "tape.tap" );
    fuse_emulation_unpause();
    break;
  case SCANCODE_F8:
    /* If tape traps active, do nothing */
    if( settings_current.tape_traps ) return 0;

    /* Otherwise, toggle whether the tape is playing or not */
    if( tape_playing ) {
      tape_stop();
    } else {
      tape_play();
    }
    break;
  case SCANCODE_F9:
    machine_select_next();
    break;
  case SCANCODE_F10:
    fuse_exiting=1;
    return 1;
  }

  return 0;

}

void svgakeyboard_keyrelease(int keysym)
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
