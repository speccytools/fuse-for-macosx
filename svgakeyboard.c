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

   E-mail: pak@ast.cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#ifdef UI_SVGA			/* Use this iff we're using svgalib */

#include <stdio.h>

#include <vga.h>
#include <vgakeyboard.h>

#include "display.h"
#include "keyboard.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"

static int exitcode=0;

int svgakeyboard_init(void)
{
  keyboard_init();
  keyboard_seteventhandler(svgakeyboard_keystroke);
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

  ptr=keysyms_get_info(keysym);

  if(ptr) {
    if(ptr->key1 != KEYBOARD_NONE) keyboard_press(ptr->key1);
    if(ptr->key2 != KEYBOARD_NONE) keyboard_press(ptr->key2);
    return 0;
  }

  /* Now deal with the non-Speccy keys */
  switch(keysym) {
  case SCANCODE_F2:
    snapshot_write();
    break;
  case SCANCODE_F3:
    snapshot_read();
    display_refresh_all();
    break;
  case SCANCODE_F5:
    machine.reset();
    break;
  case SCANCODE_F7:
    tape_open();
    break;
  case SCANCODE_F9:
    switch(machine.machine) {
    case SPECTRUM_MACHINE_48:
      machine.machine=SPECTRUM_MACHINE_128;
      break;
    case SPECTRUM_MACHINE_128:
      machine.machine=SPECTRUM_MACHINE_PLUS2;
      break;
    case SPECTRUM_MACHINE_PLUS2:
      machine.machine=SPECTRUM_MACHINE_PLUS3;
      break;
    case SPECTRUM_MACHINE_PLUS3:
      machine.machine=SPECTRUM_MACHINE_48;
      break;
    }
    spectrum_init(); machine.reset();
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
}

#endif				/* #ifdef UI_SVGA */
