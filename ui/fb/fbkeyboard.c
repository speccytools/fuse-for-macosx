/* fbkeyboard.c: routines for dealing with the linux fbdev display
   Copyright (c) 2000-2002 Philip Kendall, Matan Ziv-Av, Darren Salt

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

#include <errno.h>
#include <linux/kd.h>
#include <termios.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

static struct termios old_ts;
static int got_old_ts = 0;

void fbkeyboard_keystroke(int scancode, int press);
static void fbkeyboard_keypress( int keysym );
void fbkeyboard_keyrelease( int keysym );

int fbkeyboard_init(void)
{
  struct termios ts;
  struct stat st;
  int i = 1;

  /* First, set up the keyboard */
  if( fstat( STDIN_FILENO, &st ) ) {
    fprintf( stderr, "%s: couldn't stat stdin: %s\n", fuse_progname,
	     strerror( errno ) );
    return 1;
  }

  /* check for character special, major 4, minor 0..63 */
  if( !isatty(STDIN_FILENO) || !S_ISCHR(st.st_mode) ||
      ( st.st_rdev & ~63 ) != 0x4000 ) {
    fprintf( stderr, "%s: stdin isn't a local tty\n", fuse_progname );
    return 1;
  }

  tcgetattr( STDIN_FILENO, &old_ts );
  got_old_ts = 1;

  /* We need non-blocking semi-cooked keyboard input */
  if( ioctl( STDIN_FILENO, FIONBIO, &i ) ) {
    fprintf( stderr, "%s: can't set stdin nonblocking: %s\n", fuse_progname,
	     strerror( errno ) );
    return 1;
  }
  if( ioctl( STDIN_FILENO, KDSKBMODE, K_MEDIUMRAW ) ) {
    fprintf( stderr, "%s: can't set keyboard into medium-raw mode: %s\n",
	     fuse_progname, strerror( errno ) );
    return 1;
  }

  /* Add in a bit of voodoo... */
  ts = old_ts;
  ts.c_cc[VTIME] = 0;
  ts.c_cc[VMIN] = 1;
  ts.c_lflag &= ~(ICANON | ECHO | ISIG);
  ts.c_iflag = 0;
  tcsetattr( STDIN_FILENO, TCSAFLUSH, &ts );
  tcsetpgrp( STDIN_FILENO, getpgrp() );

  return 0;
}

void fbkeyboard_keystroke(int scancode, int press)  {
  if(press) {
    fbkeyboard_keypress(scancode);
  } else {
    fbkeyboard_keyrelease(scancode);
  }
}

static void
fbkeyboard_keypress( int keysym )
{
  keysyms_key_info *ptr;

  ptr=keysyms_get_data(keysym);

  if( ptr ) {

    if( widget_level >= 0 ) {
      widget_keyhandler( ptr->key1 );
    } else {
      if( ptr->key1 != KEYBOARD_NONE ) keyboard_press( ptr->key1 );
      if( ptr->key2 != KEYBOARD_NONE ) keyboard_press( ptr->key2 );
    }
    return;

  }

  if( widget_level >= 0 ) return;

  /* Now deal with the non-Speccy keys */
  switch( keysym ) {
  case 0x3B:			/* F1 */
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_MENU, &widget_menu_main );
    fuse_emulation_unpause();
    break;
  case 0x3C:			/* F2 */
    fuse_emulation_pause();
    snapshot_write( "snapshot.z80" );
    fuse_emulation_unpause();
    break;
  case 0x3D:			/* F3 */
    fuse_emulation_pause();
    widget_apply_to_file( snapshot_read );
    fuse_emulation_unpause();
    break;
  case 0x3E:			/* F4 */
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_GENERAL, NULL );
    fuse_emulation_unpause();
    break;
  case 0x3F:			/* F5 */
    machine_current->reset();
    break;
  case 0x40:			/* F6 */
    fuse_emulation_pause();
    tape_write( "tape.tzx" );
    fuse_emulation_unpause();
    break;
  case 0x41:			/* F7 */
    fuse_emulation_pause();
    widget_apply_to_file( tape_open );
    fuse_emulation_unpause();
    break;
  case 0x42:			/* F8 */
    tape_toggle_play();
    break;
  case 0x43:			/* F9 */
    fuse_emulation_pause();
    widget_do( WIDGET_TYPE_SELECT, NULL );
    fuse_emulation_unpause();
    break;
  case 0x44:			/* F10 */
    fuse_exiting = 1;
    break;
  }
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
  int i = 0;

  ioctl( STDIN_FILENO, FIONBIO, &i );
  if( got_old_ts ) tcsetattr( STDIN_FILENO, TCSAFLUSH, &old_ts );
  ioctl( STDIN_FILENO, KDSKBMODE, K_XLATE );

  return 0;
}

void
keyboard_update( void )
{
  char keybuf[64];
  for (;;)
  {
    int p, i = read( STDIN_FILENO, &keybuf, sizeof( keybuf ) );
    if( i <= 0 ) return;

    for( p = 0; p < i; p++ )
      if( keybuf[p] & 0x80 ) {
	fbkeyboard_keyrelease( keybuf[p] & 0x7F );
      } else {
	fbkeyboard_keypress( keybuf[p] & 0x7F );
      }
  }
}

#endif			/* #ifdef UI_FB */
