/* keystate.h: keyboard input state machine - prevents problems with pressing
               one "special" key and releasing another (because more than one
               were pressed or shift was released before the key)
   Copyright (c) 2010 Fredrick Meunier

   $Id$

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

   E-mail: fredm@spamcop.net

*/

#ifndef KEYSTATE_H
#define KEYSTATE_H

enum events {
  PRESS_NORMAL,
  RELEASE_NORMAL,
  PRESS_SPECIAL,
  RELEASE_SPECIAL,
  MAX_EVENTS
};

void process_keyevent( enum events event, input_key keysym );
void reset_keystate( void );

#endif			/* #ifndef KEYSTATE_H */
