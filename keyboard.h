/* keyboard.h: Routines for dealing with the Spectrum's keyboard
   Copyright (c) 1999-2000 Philip Kendall

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_KEYBOARD_H
#define FUSE_KEYBOARD_H

#ifndef FUSE_TYPES_H
#include "types.h"
#endif			/* #ifndef FUSE_TYPES_H */

extern BYTE keyboard_default_value;
extern BYTE keyboard_return_values[8];

/* A numeric identifier for each Spectrum key. Chosen to map to ASCII in
   most cases */
typedef enum keyboard_key_name {

  KEYBOARD_NONE = 0x00,		/* No key */

  KEYBOARD_space = 0x20,

  KEYBOARD_0 = 0x30,
  KEYBOARD_1,
  KEYBOARD_2,
  KEYBOARD_3,
  KEYBOARD_4,
  KEYBOARD_5,
  KEYBOARD_6,
  KEYBOARD_7,
  KEYBOARD_8,
  KEYBOARD_9,

  KEYBOARD_a = 0x61,
  KEYBOARD_b,
  KEYBOARD_c,
  KEYBOARD_d,
  KEYBOARD_e,
  KEYBOARD_f,
  KEYBOARD_g,
  KEYBOARD_h,
  KEYBOARD_i,
  KEYBOARD_j,
  KEYBOARD_k,
  KEYBOARD_l,
  KEYBOARD_m,
  KEYBOARD_n,
  KEYBOARD_o,
  KEYBOARD_p,
  KEYBOARD_q,
  KEYBOARD_r,
  KEYBOARD_s,
  KEYBOARD_t,
  KEYBOARD_u,
  KEYBOARD_v,
  KEYBOARD_w,
  KEYBOARD_x,
  KEYBOARD_y,
  KEYBOARD_z,

  KEYBOARD_Enter = 0x100,
  KEYBOARD_Caps,
  KEYBOARD_Symbol,

  /* Below here, keys don't actually correspond to Spectrum keys, but
     are used by the widget code */

  KEYBOARD_PageUp = 0x200,
  KEYBOARD_PageDown,
  KEYBOARD_Home,
  KEYBOARD_End,

} keyboard_key_name;

void fuse_keyboard_init(void);
BYTE keyboard_read(BYTE porth);
void keyboard_press(keyboard_key_name key);
void keyboard_release(keyboard_key_name key);

#endif			/* #ifndef FUSE_KEYBOARD_H */
