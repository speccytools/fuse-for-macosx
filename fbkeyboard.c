/* fbkeyboard.c: routines for dealing with the switches interface
   for use with the linux fb driver (on SA1110).
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

#ifdef UI_FB			/* Use this iff we're using fbdev */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/switches.h>

#include "display.h"
#include "fuse.h"
#include "keyboard.h"
#include "keysyms.h"
#include "machine.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"

void fbkeyboard_keystroke(int scancode, int press);
int fbkeyboard_keypress(int keysym);
void fbkeyboard_keyrelease(int keysym);

static int fd;

int fbkeyboard_init(void)
{
/* device name should be fixed */
    fd=open("/dev/t2",O_RDONLY | O_NONBLOCK);

  return 0;
}

void fbkeyboard_keystroke(int scancode, int press)  {
  if(press) {
    fbkeyboard_keypress(scancode);
  } else {
    fbkeyboard_keyrelease(scancode);
  }
}

int fbkeyboard_keypress(int keysym)
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
  case -1:
    snapshot_write( "snapshot.z80" );
    break;
  case 1<<13:
    snapshot_read( "snapshot.z80" );
    display_refresh_all();
    break;
  case -3:
    machine_current->reset();
    break;
  case -4:
    tape_open( "tape.tap" );
    break;
  case 1<<14:
    machine_select_next();
    break;
  case 1<<15:
    fuse_exiting=1;
    return 1;
  }

  return 0;

}

void fbkeyboard_keyrelease(int keysym)
{
  keysyms_key_info *ptr;

  ptr=keysyms_get_data(keysym);

  if(ptr) {
    if(ptr->key1 != KEYBOARD_NONE) keyboard_release(ptr->key1);
    if(ptr->key2 != KEYBOARD_NONE) keyboard_release(ptr->key2);
  }

  return;

}

int fbkeyboard_end(void)
{
  close(fd);
  return 0;
}

void keyboard_update(void) {
    switches_mask_t s;
    int i;
    i=read(fd, &s, sizeof(switches_mask_t));
    if(i==sizeof(switches_mask_t)) {
        fbkeyboard_keystroke(s.events[0]&0xffff,s.events[0]&s.states[0]&0xffff);
    }
}

#endif			/* #ifdef UI_FB */
