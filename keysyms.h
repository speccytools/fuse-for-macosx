/* keysyms.h: keysym to Spectrum key mappings for both Xlib and GDK
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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_KEYSYMS_H
#define FUSE_KEYSYMS_H

#ifndef FUSE_KEYBOARD_H
#include "keyboard.h"
#endif			/* #ifndef FUSE_KEYBOARD_H */

typedef struct keysyms_key_info {
  unsigned long keysym;
  keyboard_key_name key1,key2;
} keysyms_key_info;

keysyms_key_info* keysyms_get_data(unsigned keysym);

#endif			/* #ifndef FUSE_KEYSYMS_H */
