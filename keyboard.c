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

#include <stdio.h>

#include "types.h"

BYTE keyboard_return_values[8];

void keyboard_init(void)
{
  int i;
  
  for(i=0;i<8;i++) keyboard_return_values[i]=0xff;
}

BYTE read_keyboard(BYTE porth)
{
  BYTE data=0xff; int i;

  for(i=0;i<8;i++) {
    if(! (porth&0x01) ) data &= keyboard_return_values[i];
    porth >>= 1;
  }

  return data;

}

