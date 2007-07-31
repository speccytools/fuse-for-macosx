/* sdl.c: SDL speed routines for Fuse
   Copyright (c) 1999-2007 Philip Kendall, Marek Januszewski, Fredrick Meunier

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include "timer.h"

int
timer_get_real_time( timer_type *real_time )
{
  *real_time = SDL_GetTicks();

  return 0;
}

float
timer_get_time_difference( timer_type *a, timer_type *b )
{
  return ( (long)*a - (long)*b ) / 1000.0;
}

void
timer_add_time_difference( timer_type *a, long msec )
{
  *a += msec;
}

void
timer_sleep_ms( int ms )
{
  SDL_Delay( ms );
}

