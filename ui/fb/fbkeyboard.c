/* fbkeyboard.c: routines for dealing with the linux fbdev display
   Copyright (c) 2000-2004 Philip Kendall, Matan Ziv-Av, Darren Salt

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
#include "machine.h"
#include "settings.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"
#ifdef USE_WIDGET
#include "widget/widget.h"
#endif				/* #ifdef USE_WIDGET */

#include "input.h"

static struct termios old_ts;
static int got_old_ts = 0;

static const enum input_key keymap[128] = {
/* 0 */
  0, INPUT_KEY_Escape, INPUT_KEY_1, INPUT_KEY_2,
  INPUT_KEY_3, INPUT_KEY_4, INPUT_KEY_5, INPUT_KEY_6,
  INPUT_KEY_7, INPUT_KEY_8, INPUT_KEY_9, INPUT_KEY_0,
  INPUT_KEY_minus, INPUT_KEY_equal, INPUT_KEY_BackSpace, INPUT_KEY_Tab,
/* 0x10 */
  INPUT_KEY_q, INPUT_KEY_w, INPUT_KEY_e, INPUT_KEY_r,
  INPUT_KEY_t, INPUT_KEY_y, INPUT_KEY_u, INPUT_KEY_i,
  INPUT_KEY_o, INPUT_KEY_p, 0, 0,
  INPUT_KEY_Return, INPUT_KEY_Control_L, INPUT_KEY_a, INPUT_KEY_s,
/* 0x20 */
  INPUT_KEY_d, INPUT_KEY_f, INPUT_KEY_g, INPUT_KEY_h,
  INPUT_KEY_j, INPUT_KEY_k, INPUT_KEY_l, INPUT_KEY_semicolon,
  INPUT_KEY_apostrophe, 0, INPUT_KEY_Shift_L, INPUT_KEY_numbersign,
  INPUT_KEY_z, INPUT_KEY_x, INPUT_KEY_c, INPUT_KEY_v,
/* 0x30 */
  INPUT_KEY_b, INPUT_KEY_n, INPUT_KEY_m, INPUT_KEY_comma,
  INPUT_KEY_period, INPUT_KEY_slash, INPUT_KEY_Shift_R, 0,
  INPUT_KEY_Alt_L, INPUT_KEY_space, INPUT_KEY_Caps_Lock, INPUT_KEY_F1,
  INPUT_KEY_F2, INPUT_KEY_F3, INPUT_KEY_F4, INPUT_KEY_F5,
/* 0x40 */
  INPUT_KEY_F6, INPUT_KEY_F7, INPUT_KEY_F8, INPUT_KEY_F9,
  INPUT_KEY_F10, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
/* 0x50 */
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
/* 0x60 */
  0, INPUT_KEY_Control_R, 0, 0,
  INPUT_KEY_Alt_R, 0, INPUT_KEY_Home, INPUT_KEY_Up,
  INPUT_KEY_Page_Up, INPUT_KEY_Left, INPUT_KEY_Right, INPUT_KEY_End,
  INPUT_KEY_Down, INPUT_KEY_Page_Down, 0, 0,
/* 0x70 */
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, INPUT_KEY_Meta_L, INPUT_KEY_Meta_R, INPUT_KEY_Mode_switch,
  /* logo/menu keys: somewhat arbitrary mapping for use as Symbol Shift */
};

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
      ( st.st_rdev & ~63 ) != 0x0400 ) {
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
  unsigned char keybuf[64];
  static int ignore = 0;

  while( 1 ) {
    ssize_t available, i;

    available = read( STDIN_FILENO, &keybuf, sizeof( keybuf ) );
    if( available <= 0 ) return;

    for( i = 0; i < available; i++ )
      if( ignore ) {
	ignore--;
      } else if( ( keybuf[i] & 0x7f ) == 0 ) {
	ignore = 2; /* ignore extended keysyms */
      } else if( keymap[ keybuf[i] & 0x7f ] ) {
	input_event_t fuse_event;

	fuse_event.type = ( keybuf[i] & 0x80 ) ?
                          INPUT_EVENT_KEYRELEASE :
                          INPUT_EVENT_KEYPRESS;
	fuse_event.types.key.key = keymap[ keybuf[i] & 0x7f ];

	input_event( &fuse_event );
      }
  }
}

#endif			/* #ifdef UI_FB */
