/* keyboard.c: Routines for dealing with the Spectrum's keyboard
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

   E-mail: pak@ast.cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include "keyboard.h"
#include "types.h"

/* What to return if no keys are pressed; depends on the last byte
   output to the ULA; see CSS FAQ | Technical Information | Port #FE
   for full details */
BYTE keyboard_default_value;

/* Bit masks for each of the eight keyboard half-rows; `AND' the selected
   ones of these with `keyboard_default_value' to get the value to return
*/
BYTE keyboard_return_values[8];

/* When each Spectrum key is pressed, twiddle this {port,bit} pair
   in `keyboard_return_values'. 
*/
typedef struct keyboard_key_info {
  keyboard_key_name key;
  int port; BYTE bit;
} keyboard_key_info;

static keyboard_key_info keyboard_data[] = {

  { KEYBOARD_1      , 3, 0x01 },
  { KEYBOARD_2      , 3, 0x02 },
  { KEYBOARD_3      , 3, 0x04 },
  { KEYBOARD_4      , 3, 0x08 },
  { KEYBOARD_5      , 3, 0x10 },
  { KEYBOARD_6      , 4, 0x10 },
  { KEYBOARD_7      , 4, 0x08 },
  { KEYBOARD_8      , 4, 0x04 },
  { KEYBOARD_9      , 4, 0x02 },
  { KEYBOARD_0      , 4, 0x01 },

  { KEYBOARD_q      , 2, 0x01 },
  { KEYBOARD_w      , 2, 0x02 },
  { KEYBOARD_e      , 2, 0x04 },
  { KEYBOARD_r      , 2, 0x08 },
  { KEYBOARD_t      , 2, 0x10 },
  { KEYBOARD_y      , 5, 0x10 },
  { KEYBOARD_u      , 5, 0x08 },
  { KEYBOARD_i      , 5, 0x04 },
  { KEYBOARD_o      , 5, 0x02 },
  { KEYBOARD_p      , 5, 0x01 },

  { KEYBOARD_a      , 1, 0x01 },
  { KEYBOARD_s      , 1, 0x02 },
  { KEYBOARD_d      , 1, 0x04 },
  { KEYBOARD_f      , 1, 0x08 },
  { KEYBOARD_g      , 1, 0x10 },
  { KEYBOARD_h      , 6, 0x10 },
  { KEYBOARD_j      , 6, 0x08 },
  { KEYBOARD_k      , 6, 0x04 },
  { KEYBOARD_l      , 6, 0x02 },
  { KEYBOARD_Enter  , 6, 0x01 },

  { KEYBOARD_Caps   , 0, 0x01 },
  { KEYBOARD_z      , 0, 0x02 },
  { KEYBOARD_x      , 0, 0x04 },
  { KEYBOARD_c      , 0, 0x08 },
  { KEYBOARD_v      , 0, 0x10 },
  { KEYBOARD_b      , 7, 0x10 },
  { KEYBOARD_n      , 7, 0x08 },
  { KEYBOARD_m      , 7, 0x04 },
  { KEYBOARD_Symbol , 7, 0x02 },
  { KEYBOARD_space  , 7, 0x01 },

  { 0               , 0, 0x00 } /* End marker: DO NOT MOVE! */

};

void keyboard_init(void)
{
  int i;
  
  keyboard_default_value=0xff;
  for(i=0;i<8;i++) keyboard_return_values[i]=0xff;
}

BYTE keyboard_read(BYTE porth)
{
  BYTE data=keyboard_default_value; int i;

  for( i=0; i<8; i++,porth>>=1 ) {
    if(! (porth&0x01) ) {
      data &= keyboard_return_values[i];
    }
  }

  return data;

}

void keyboard_press(keyboard_key_name key)
{
  keyboard_key_info *ptr;

  if(key==KEYBOARD_NONE) return;

  for( ptr=keyboard_data; ptr->key; ptr++ ) {
    if( key == ptr->key ) {
      keyboard_return_values[ptr->port] &= ~(ptr->bit);
      return;
    }
  }
  return;
}

void keyboard_release(keyboard_key_name key)
{
  keyboard_key_info *ptr;

  if(key==KEYBOARD_NONE) return;

  for( ptr=keyboard_data; ptr->key; ptr++ ) {
    if( key == ptr->key ) {
      keyboard_return_values[ptr->port] |= ptr->bit;
      return;
    }
  }
  return;
}
