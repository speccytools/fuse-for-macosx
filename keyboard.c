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

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <libspectrum.h>

#include "ui/ui.h"
#include "keyboard.h"
#include "scld.h"

/* What to return if no keys are pressed; depends on the last byte
   output to the ULA; see CSS FAQ | Technical Information | Port #FE
   for full details */
libspectrum_byte keyboard_default_value;

/* Bit masks for each of the eight keyboard half-rows; `AND' the selected
   ones of these with `keyboard_default_value' to get the value to return
*/
libspectrum_byte keyboard_return_values[8];

/* Which Spectrum keys should be pressed for each key passed from the
   Fuse input layer */
const static keysyms_key_info keysyms_data[] = {

  { INPUT_KEY_Escape,      KEYBOARD_1,     KEYBOARD_Caps   },
  { INPUT_KEY_1,           KEYBOARD_1,     KEYBOARD_NONE   },
  { INPUT_KEY_2,           KEYBOARD_2,     KEYBOARD_NONE   },
  { INPUT_KEY_3,           KEYBOARD_3,     KEYBOARD_NONE   },
  { INPUT_KEY_4,           KEYBOARD_4,     KEYBOARD_NONE   },
  { INPUT_KEY_5,           KEYBOARD_5,     KEYBOARD_NONE   },
  { INPUT_KEY_6,           KEYBOARD_6,     KEYBOARD_NONE   },
  { INPUT_KEY_7,           KEYBOARD_7,     KEYBOARD_NONE   },
  { INPUT_KEY_8,           KEYBOARD_8,     KEYBOARD_NONE   },
  { INPUT_KEY_9,           KEYBOARD_9,     KEYBOARD_NONE   },
  { INPUT_KEY_0,           KEYBOARD_0,     KEYBOARD_NONE   },
  { INPUT_KEY_minus,       KEYBOARD_j,     KEYBOARD_Symbol },
  { INPUT_KEY_equal,       KEYBOARD_l,     KEYBOARD_Symbol },
  { INPUT_KEY_BackSpace,   KEYBOARD_0,     KEYBOARD_Caps   },

  { INPUT_KEY_q,           KEYBOARD_q,     KEYBOARD_NONE   },
  { INPUT_KEY_w,           KEYBOARD_w,     KEYBOARD_NONE   },
  { INPUT_KEY_e,           KEYBOARD_e,     KEYBOARD_NONE   },
  { INPUT_KEY_r,           KEYBOARD_r,     KEYBOARD_NONE   },
  { INPUT_KEY_t,           KEYBOARD_t,     KEYBOARD_NONE   },
  { INPUT_KEY_y,           KEYBOARD_y,     KEYBOARD_NONE   },
  { INPUT_KEY_u,           KEYBOARD_u,     KEYBOARD_NONE   },
  { INPUT_KEY_i,           KEYBOARD_i,     KEYBOARD_NONE   },
  { INPUT_KEY_o,           KEYBOARD_o,     KEYBOARD_NONE   },
  { INPUT_KEY_p,           KEYBOARD_p,     KEYBOARD_NONE   },

  { INPUT_KEY_Caps_Lock,   KEYBOARD_2,     KEYBOARD_Caps   },
  { INPUT_KEY_a,           KEYBOARD_a,     KEYBOARD_NONE   },
  { INPUT_KEY_s,           KEYBOARD_s,     KEYBOARD_NONE   },
  { INPUT_KEY_d,           KEYBOARD_d,     KEYBOARD_NONE   },
  { INPUT_KEY_f,           KEYBOARD_f,     KEYBOARD_NONE   },
  { INPUT_KEY_g,           KEYBOARD_g,     KEYBOARD_NONE   },
  { INPUT_KEY_h,           KEYBOARD_h,     KEYBOARD_NONE   },
  { INPUT_KEY_j,           KEYBOARD_j,     KEYBOARD_NONE   },
  { INPUT_KEY_k,           KEYBOARD_k,     KEYBOARD_NONE   },
  { INPUT_KEY_l,           KEYBOARD_l,     KEYBOARD_NONE   },
  { INPUT_KEY_semicolon,   KEYBOARD_o,     KEYBOARD_Symbol },
  { INPUT_KEY_apostrophe,  KEYBOARD_7,     KEYBOARD_Symbol },
  { INPUT_KEY_numbersign,  KEYBOARD_3,     KEYBOARD_Symbol },
  { INPUT_KEY_Return,      KEYBOARD_Enter, KEYBOARD_NONE   },

  { INPUT_KEY_Shift_L,     KEYBOARD_NONE,  KEYBOARD_Caps   },
  { INPUT_KEY_z,           KEYBOARD_z,     KEYBOARD_NONE   },
  { INPUT_KEY_x,           KEYBOARD_x,     KEYBOARD_NONE   },
  { INPUT_KEY_c,           KEYBOARD_c,     KEYBOARD_NONE   },
  { INPUT_KEY_v,           KEYBOARD_v,     KEYBOARD_NONE   },
  { INPUT_KEY_b,           KEYBOARD_b,     KEYBOARD_NONE   },
  { INPUT_KEY_n,           KEYBOARD_n,     KEYBOARD_NONE   },
  { INPUT_KEY_m,           KEYBOARD_m,     KEYBOARD_NONE   },
  { INPUT_KEY_comma,       KEYBOARD_n,     KEYBOARD_Symbol },
  { INPUT_KEY_period,      KEYBOARD_m,     KEYBOARD_Symbol },
  { INPUT_KEY_slash,       KEYBOARD_v,     KEYBOARD_Symbol },
  { INPUT_KEY_Shift_R,     KEYBOARD_NONE,  KEYBOARD_Caps   },

  { INPUT_KEY_Control_L,   KEYBOARD_NONE,  KEYBOARD_Symbol },
  { INPUT_KEY_Alt_L,       KEYBOARD_NONE,  KEYBOARD_Symbol },
  { INPUT_KEY_Meta_L,      KEYBOARD_NONE,  KEYBOARD_Symbol },
  { INPUT_KEY_Super_L,     KEYBOARD_NONE,  KEYBOARD_Symbol },
  { INPUT_KEY_Hyper_L,     KEYBOARD_NONE,  KEYBOARD_Symbol },
  { INPUT_KEY_space,       KEYBOARD_space, KEYBOARD_NONE   },
  { INPUT_KEY_Hyper_R,     KEYBOARD_NONE,  KEYBOARD_Symbol },
  { INPUT_KEY_Super_R,     KEYBOARD_NONE,  KEYBOARD_Symbol },
  { INPUT_KEY_Meta_R,      KEYBOARD_NONE,  KEYBOARD_Symbol },
  { INPUT_KEY_Alt_R,       KEYBOARD_NONE,  KEYBOARD_Symbol },
  { INPUT_KEY_Control_R,   KEYBOARD_NONE,  KEYBOARD_Symbol },
  { INPUT_KEY_Mode_switch, KEYBOARD_NONE,  KEYBOARD_Symbol },

  { INPUT_KEY_Left,        KEYBOARD_5,     KEYBOARD_Caps   },
  { INPUT_KEY_Down,        KEYBOARD_6,     KEYBOARD_Caps   },
  { INPUT_KEY_Up,          KEYBOARD_7,     KEYBOARD_Caps   },
  { INPUT_KEY_Right,       KEYBOARD_8,     KEYBOARD_Caps   },

  { 0, 0, 0 }		/* End marker */

};

/* When each Spectrum key is pressed, twiddle this {port,bit} pair
   in `keyboard_return_values'. 
*/
typedef struct keyboard_key_info {
  keyboard_key_name key;
  int port; libspectrum_byte bit;
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

/* Called `fuse_keyboard_init' as svgalib pollutes the global namespace
   with keyboard_init... */
void fuse_keyboard_init(void)
{
  keyboard_default_value=0xff;
  keyboard_release_all();
}

libspectrum_byte
keyboard_read( libspectrum_byte porth )
{
  libspectrum_byte data = keyboard_default_value; int i;

  for( i=0; i<8; i++,porth>>=1 ) {
    if(! (porth&0x01) ) data &= keyboard_return_values[i];
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

int keyboard_release_all( void )
{
  int i;

  for( i=0; i<8; i++ ) keyboard_return_values[i] = 0xff;

  return 0;
}

const keysyms_key_info*
keysyms_get_data( input_key keysym )
{
  const keysyms_key_info *ptr;

  for( ptr=keysyms_data; ptr->keysym; ptr++ ) {
    if( keysym == ptr->keysym ) {
      return ptr;
    }
  }

  return NULL;
}

input_key
keysyms_remap( libspectrum_dword ui_keysym )
{
  const keysyms_map_t *ptr;

  for( ptr = keysyms_map; ptr->ui; ptr++ )
    if( ui_keysym == ptr->ui )
      return ptr->fuse;

  return INPUT_KEY_NONE;
}

    
