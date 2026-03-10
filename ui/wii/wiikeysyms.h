/* wiikeysyms.h: routines for dealing with the Wii USB keyboard
   Copyright (c) 2008-2009 Bjoern Giesler, Philip Kendall
   Copyright (c) 2023 Miroslav Ďurčík

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

   E-mail: bjoern@giesler.de

*/

#ifndef FUSE_WIIKEYSYMS_H
#define FUSE_WIIKEYSYMS_H

/* definitions taken from http://wiibrew.org/wiki//dev/usb/kbd */

enum {

  WII_KEY_A = 0x04,
  WII_KEY_B,
  WII_KEY_C,
  WII_KEY_D,
  WII_KEY_E,
  WII_KEY_F,
  WII_KEY_G,
  WII_KEY_H,
  WII_KEY_I,
  WII_KEY_J,
  WII_KEY_K,
  WII_KEY_L,
  WII_KEY_M,
  WII_KEY_N,
  WII_KEY_O,
  WII_KEY_P,
  WII_KEY_Q,
  WII_KEY_R,
  WII_KEY_S,
  WII_KEY_T,
  WII_KEY_U,
  WII_KEY_V,
  WII_KEY_W,
  WII_KEY_X,
  WII_KEY_Y,
  WII_KEY_Z,
  WII_KEY_0 = 0x1E,
  WII_KEY_1,
  WII_KEY_2,
  WII_KEY_3,
  WII_KEY_4,
  WII_KEY_5,
  WII_KEY_6,
  WII_KEY_7,
  WII_KEY_8,
  WII_KEY_9,
  WII_KEY_RETURN = 0x28,
  WII_KEY_ESCAPE,
  WII_KEY_BACKSPACE,
  WII_KEY_TAB,
  WII_KEY_SPACE,
  WII_KEY_MINUS,
  WII_KEY_EQUAL,
  WII_KEY_BRACKETLEFT,
  WII_KEY_BRACKETRIGHT,
  /* Unknown */
  /* Right Backslash */
  WII_KEY_SEMICOLON = 0x33,
  WII_KEY_APOSTROPHE,
  WII_KEY_ASCIITILDE,
  WII_KEY_COMMA = 0x36,
  WII_KEY_PERIOD,
  WII_KEY_SLASH,
  WII_KEY_CAPS_LOCK = 0x39,
  WII_KEY_F1,
  WII_KEY_F2,
  WII_KEY_F3,
  WII_KEY_F4,
  WII_KEY_F5,
  WII_KEY_F6,
  WII_KEY_F7,
  WII_KEY_F8,
  WII_KEY_F9,
  WII_KEY_F10,
  WII_KEY_F11,
  WII_KEY_F12,
  /* Print screen (0x46) */
  /* Scroll lock */
  /* Pause/Break */
  WII_KEY_INSERT = 0x49,
  WII_KEY_HOME,
  WII_KEY_PAGE_UP,
  WII_KEY_DELETE,
  WII_KEY_END,
  WII_KEY_PAGE_DOWN,
  WII_KEY_RIGHT,
  WII_KEY_LEFT,
  WII_KEY_DOWN,
  WII_KEY_UP,
  /* Num lock (0x53) */
  /* Numpad / */
  WII_KEY_ASTERISK = 0x55, /* Numpad * */
  /* Numpad - */
  WII_KEY_PLUS = 0x57, /* Numpad + */
  /* Numpad Enter */
  /* Numpad 1 (0x59) */
  /* Numpad 2 */
  /* Numpad 3 */
  /* Numpad 4 */
  /* Numpad 5 */
  /* Numpad 6 */
  /* Numpad 7 */
  /* Numpad 8 */
  /* Numpad 9 */
  /* Numpad 0 */
  /* Numpad Period (0x63) */
  /* Left Backslash */
  /* Menu Key (0x65) */

  /* impossible mapping */
  WII_KEY_AMPERSAND,
  WII_KEY_ASCIICIRCUM,
  WII_KEY_AT,
  WII_KEY_BACKSLASH,
  WII_KEY_BAR,
  WII_KEY_BRACELEFT,
  WII_KEY_BRACERIGHT,
  WII_KEY_COLON,
  WII_KEY_DEAD_CIRCUMFLEX,
  WII_KEY_DOLLAR,
  WII_KEY_EXCLAM,
  WII_KEY_GREATER,
  WII_KEY_LESS,
  WII_KEY_PERCENT,
  WII_KEY_PARENLEFT,
  WII_KEY_PARENRIGHT,
  WII_KEY_QUESTION,
  WII_KEY_QUOTEDBL,
  WII_KEY_UNDERSCORE,

};

#endif			/* #ifndef FUSE_WIIKEYSYMS_H */
