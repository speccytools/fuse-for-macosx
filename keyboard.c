/* keyboard.c: Routines for dealing with the Spectrum's keyboard
   Copyright (c) 1999 Philip Kendall

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

   E-mail: pak21@cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <stdio.h>

#include "alleg.h"
#include "spec48.h"

/* The mappings from PC keys to Spectrum keys. The PC key with
   scancode `scancode' maps to the two Spectrum keys, specified by
   {port,bit}{1,2}. They key will be recognised only if the port being
   read from had `port' *reset* in its high byte; if this is the case,
   then the bit(s) specified in `bit' are *reset* in the returned
   value. (This is the normal Speccy behaviour) */

typedef struct keyboard_key_info {
  BYTE scancode;
  BYTE port1,bit1;
  BYTE port2,bit2;
} keyboard_key_info;

/* The actual mappings; increment keyboard_data_size below if you
   increase the size of this table! */

keyboard_key_info keyboard_data[] = {
  { KEY_1        , 0x08, ~0x01, 0x00, ~0x00 },
  { KEY_2        , 0x08, ~0x02, 0x00, ~0x00 },
  { KEY_3	 , 0x08, ~0x04, 0x00, ~0x00 },
  { KEY_4	 , 0x08, ~0x08, 0x00, ~0x00 },
  { KEY_5	 , 0x08, ~0x10, 0x00, ~0x00 },
  { KEY_6	 , 0x10, ~0x10, 0x00, ~0x00 },
  { KEY_7	 , 0x10, ~0x08, 0x00, ~0x00 },
  { KEY_8	 , 0x10, ~0x04, 0x00, ~0x00 },
  { KEY_9	 , 0x10, ~0x02, 0x00, ~0x00 },
  { KEY_0	 , 0x10, ~0x01, 0x00, ~0x00 },
  { KEY_MINUS    , 0x80, ~0x02, 0x40, ~0x08 }, /* SS + J */
  { KEY_EQUALS   , 0x80, ~0x02, 0x40, ~0x02 }, /* SS + L */
  { KEY_BACKSPACE, 0x01, ~0x01, 0x10, ~0x01 }, /* CS + 0 */
  { KEY_Q	 , 0x04, ~0x01, 0x00, ~0x00 },
  { KEY_W	 , 0x04, ~0x02, 0x00, ~0x00 },
  { KEY_E	 , 0x04, ~0x04, 0x00, ~0x00 },
  { KEY_R	 , 0x04, ~0x08, 0x00, ~0x00 },
  { KEY_T	 , 0x04, ~0x10, 0x00, ~0x00 },
  { KEY_Y	 , 0x20, ~0x10, 0x00, ~0x00 },
  { KEY_U	 , 0x20, ~0x08, 0x00, ~0x00 },
  { KEY_I	 , 0x20, ~0x04, 0x00, ~0x00 },
  { KEY_O	 , 0x20, ~0x02, 0x00, ~0x00 },
  { KEY_P	 , 0x20, ~0x01, 0x00, ~0x00 },
  { KEY_CAPSLOCK , 0x01, ~0x01, 0x08, ~0x02 }, /* CS + 2 */
  { KEY_A	 , 0x02, ~0x01, 0x00, ~0x00 },
  { KEY_S	 , 0x02, ~0x02, 0x00, ~0x00 },
  { KEY_D	 , 0x02, ~0x04, 0x00, ~0x00 },
  { KEY_F	 , 0x02, ~0x08, 0x00, ~0x00 },
  { KEY_G	 , 0x02, ~0x10, 0x00, ~0x00 },
  { KEY_H	 , 0x40, ~0x10, 0x00, ~0x00 },
  { KEY_J	 , 0x40, ~0x08, 0x00, ~0x00 },
  { KEY_K	 , 0x40, ~0x04, 0x00, ~0x00 },
  { KEY_L	 , 0x40, ~0x02, 0x00, ~0x00 },
  { KEY_COLON    , 0x80, ~0x02, 0x20, ~0x02 }, /* SS + O */
  { KEY_QUOTE    , 0x80, ~0x02, 0x10, ~0x08 }, /* SS + 7 */
  { KEY_ENTER    , 0x40, ~0x01, 0x00, ~0x00 },
  { KEY_LSHIFT	 , 0x01, ~0x01, 0x00, ~0x00 },
  { KEY_Z	 , 0x01, ~0x02, 0x00, ~0x00 },
  { KEY_X	 , 0x01, ~0x04, 0x00, ~0x00 },
  { KEY_C	 , 0x01, ~0x08, 0x00, ~0x00 },
  { KEY_V	 , 0x01, ~0x10, 0x00, ~0x00 },
  { KEY_B	 , 0x80, ~0x10, 0x00, ~0x00 },
  { KEY_N	 , 0x80, ~0x08, 0x00, ~0x00 },
  { KEY_M	 , 0x80, ~0x04, 0x00, ~0x00 },
  { KEY_COMMA    , 0x80, ~0x0a, 0x00, ~0x00 }, /* SS + N */
  { KEY_STOP     , 0x80, ~0x06, 0x00, ~0x00 }, /* SS + M */
  { KEY_SLASH    , 0x80, ~0x02, 0x01, ~0x10 }, /* SS + V */
  { KEY_RSHIFT	 , 0x01, ~0x01, 0x00, ~0x00 },
  { KEY_LCONTROL , 0x80, ~0x02, 0x00, ~0x00 },
  { KEY_ALT	 , 0x80, ~0x02, 0x00, ~0x00 },
  { KEY_SPACE	 , 0x80, ~0x01, 0x00, ~0x00 },
  { KEY_RCONTROL , 0x80, ~0x02, 0x00, ~0x00 },
  { KEY_ALTGR    , 0x80, ~0x02, 0x00, ~0x00 },
};
  
int keyboard_data_size=53;

void keyboard_init(void)
{
  install_keyboard();
}

BYTE read_keyboard(BYTE porth)
{
  int i; BYTE data=0xff;

  for(i=0;i<keyboard_data_size;i++)
    if( key[keyboard_data[i].scancode] ) {
      if( ! (porth & keyboard_data[i].port1) ) data &= keyboard_data[i].bit1;
      if( ! (porth & keyboard_data[i].port2) ) data &= keyboard_data[i].bit2;
    }

  return data;

}

