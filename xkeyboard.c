/* xkeyboard.c: X routines for dealing with the keyboard
   Copyright (c) 2000 Philip Kendall

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

/* The mappings from X keysyms to Spectrum keys. The keysym `keysym'
   maps to one or two Spectrum keys, specified by {port,bit}{1,2}. 
   Unused second keys are specified by having `bit2' being zero */

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>

#include "display.h"
#include "keyboard.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"

typedef struct xkeyboard_key_info {
  unsigned long keysym;
  int port1; BYTE bit1;
  int port2; BYTE bit2;
} xkeyboard_key_info;

/* The actual mappings; Ordered like a standard (English) PC keyboard,
   top to bottom, left to right.
   Increment xkeyboard_data_size below if you increase the size of this table!  */

static xkeyboard_key_info xkeyboard_data[] = {

  { XK_1         , 3, 0x01, 0, 0x00 },
  { XK_2         , 3, 0x02, 0, 0x00 },
  { XK_3	 , 3, 0x04, 0, 0x00 },
  { XK_4	 , 3, 0x08, 0, 0x00 },
  { XK_5	 , 3, 0x10, 0, 0x00 },
  { XK_6	 , 4, 0x10, 0, 0x00 },
  { XK_7	 , 4, 0x08, 0, 0x00 },
  { XK_8	 , 4, 0x04, 0, 0x00 },
  { XK_9	 , 4, 0x02, 0, 0x00 },
  { XK_0	 , 4, 0x01, 0, 0x00 },

  { XK_minus     , 7, 0x02, 6, 0x08 }, /* SS + J */
  { XK_equal     , 7, 0x02, 6, 0x02 }, /* SS + L */
  { XK_BackSpace , 0, 0x01, 4, 0x01 }, /* CS + 0 */
  { XK_q	 , 2, 0x01, 0, 0x00 },
  { XK_w	 , 2, 0x02, 0, 0x00 },
  { XK_e	 , 2, 0x04, 0, 0x00 },
  { XK_r	 , 2, 0x08, 0, 0x00 },
  { XK_t	 , 2, 0x10, 0, 0x00 },
  { XK_y	 , 5, 0x10, 0, 0x00 },
  { XK_u	 , 5, 0x08, 0, 0x00 },
  { XK_i	 , 5, 0x04, 0, 0x00 },
  { XK_o	 , 5, 0x02, 0, 0x00 },
  { XK_p	 , 5, 0x01, 0, 0x00 },

  { XK_Caps_Lock , 0, 0x01, 3, 0x02 }, /* CS + 2 */
  { XK_a	 , 1, 0x01, 0, 0x00 },
  { XK_s	 , 1, 0x02, 0, 0x00 },
  { XK_d	 , 1, 0x04, 0, 0x00 },
  { XK_f	 , 1, 0x08, 0, 0x00 },
  { XK_g	 , 1, 0x10, 0, 0x00 },
  { XK_h	 , 6, 0x10, 0, 0x00 },
  { XK_j	 , 6, 0x08, 0, 0x00 },
  { XK_k	 , 6, 0x04, 0, 0x00 },
  { XK_l	 , 6, 0x02, 0, 0x00 },
  { XK_semicolon , 7, 0x02, 5, 0x02 }, /* SS + O */
  { XK_quotedbl  , 7, 0x02, 4, 0x08 }, /* SS + 7 */
  { XK_Return    , 6, 0x01, 0, 0x00 },

  { XK_Shift_L	 , 0, 0x01, 0, 0x00 }, /* CS */
  { XK_z	 , 0, 0x02, 0, 0x00 },
  { XK_x	 , 0, 0x04, 0, 0x00 },
  { XK_c	 , 0, 0x08, 0, 0x00 },
  { XK_v	 , 0, 0x10, 0, 0x00 },
  { XK_b	 , 7, 0x10, 0, 0x00 },
  { XK_n	 , 7, 0x08, 0, 0x00 },
  { XK_m	 , 7, 0x04, 0, 0x00 },
  { XK_comma     , 7, 0x0a, 0, 0x00 }, /* SS + N */
  { XK_period    , 7, 0x06, 0, 0x00 }, /* SS + M */
  { XK_slash     , 7, 0x02, 0, 0x10 }, /* SS + V */
  { XK_Shift_R	 , 0, 0x01, 0, 0x00 }, /* CS */

  { XK_Control_L , 7, 0x02, 0, 0x00 }, /* SS */
  { XK_Alt_L	 , 7, 0x02, 0, 0x00 }, /* SS */
  { XK_Meta_L	 , 7, 0x02, 0, 0x00 }, /* SS */
  { XK_space	 , 7, 0x01, 0, 0x00 },
  { XK_Control_R , 7, 0x02, 0, 0x00 }, /* SS */
  { XK_Alt_R     , 7, 0x02, 0, 0x00 }, /* SS */
  { XK_Meta_R    , 7, 0x02, 0, 0x00 }, /* SS */
};
  
static int xkeyboard_data_size=55;

int xkeyboard_keypress(XKeyEvent *event)
{
  KeySym keysym; xkeyboard_key_info *ptr; int i;
  int foundKey=0;

  keysym=XLookupKeysym(event,0);

  for(i=0;i<xkeyboard_data_size && !foundKey;i++)
    if(keysym==xkeyboard_data[i].keysym) {
      foundKey=1;
      ptr=&xkeyboard_data[i];
      keyboard_return_values[ptr->port1] &= ~(ptr->bit1);
      keyboard_return_values[ptr->port2] &= ~(ptr->bit2);
    }

  /* Now deal with the non-Speccy keys */
  switch(keysym) {
  case XK_F2:
    snapshot_write();
    break;
  case XK_F3:
    snapshot_read();
    display_refresh_all();
    break;
  case XK_F5:
    machine.reset();
    break;
  case XK_F7:
    tape_open();
    break;
  case XK_F9:
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
  case XK_F10:
    return 1;
  }

  return 0;

}

void xkeyboard_keyrelease(XKeyEvent *event)
{
  KeySym keysym; xkeyboard_key_info *ptr; int i;
  int foundKey=0;

  keysym=XLookupKeysym(event,0);

  for(i=0;i<xkeyboard_data_size && !foundKey;i++)
    if(keysym==xkeyboard_data[i].keysym) {
      foundKey=1;
      ptr=&xkeyboard_data[i];
      keyboard_return_values[ptr->port1] |= ptr->bit1;
      keyboard_return_values[ptr->port2] |= ptr->bit2;
    }
}
