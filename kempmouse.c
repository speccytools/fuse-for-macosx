/* kempmouse.c: Kempston mouse emulation
   Copyright (c) 2004 Darren Salt

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

   E-mail: linux@youmustbejoking.demon.co.uk

*/

#include <config.h>

#include <libspectrum.h>

#include "kempmouse.h"
#include "periph.h"
#include "settings.h"
#include "ui/ui.h"

static struct {
  struct { libspectrum_byte x, y; } pos;
  libspectrum_byte buttons;
} kempmouse = { {0, 0}, 255 };

#define READ(name,item) \
  static libspectrum_byte \
  read_##name( libspectrum_word port GCC_UNUSED, int *attached ) \
  { \
    if( !settings_current.kempston_mouse ) return 0xff; \
    *attached = 1; \
    return kempmouse.item; \
  }

READ( buttons, buttons );
READ( x_pos, pos.x );
READ( y_pos, pos.y );

const periph_t kempmouse_peripherals[] = {
  /* _we_ require b0 set */
  { 0x0121, 0x0001, read_buttons, NULL },
  { 0x0521, 0x0101, read_x_pos, NULL },
  { 0x0521, 0x0501, read_y_pos, NULL },
};

const size_t kempmouse_peripherals_count =
  sizeof( kempmouse_peripherals ) / sizeof( periph_t );

void
kempmouse_update( int dx, int dy, int btn, int down )
{
  kempmouse.pos.x += dx;
  kempmouse.pos.y -= dy;
  if( btn != -1 ) {
    if( down )
      kempmouse.buttons &= ~(1 << btn);
    else
      kempmouse.buttons |= 1 << btn;
  }
}
