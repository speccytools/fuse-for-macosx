/* timer.h: Speed routines for Fuse
   Copyright (c) 1999-2004 Philip Kendall

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#ifndef FUSE_TIMER_H
#define FUSE_TIMER_H

#ifdef UI_SDL

#include "SDL.h"

typedef Uint32 timer_type;

#elif defined(WIN32)		/* #ifdef UI_SDL */

#include <windows.h>

typedef DWORD timer_type;

#else				/* #ifdef UI_SDL */

#include <sys/time.h>
#include <time.h>

typedef struct timeval timer_type;

#endif				/* #ifdef UI_SDL */

int timer_estimate_reset( void );
int timer_estimate_speed( void );
int timer_get_real_time( timer_type *real_time );
float timer_get_time_difference( timer_type *a, timer_type *b );

int timer_init(void);
void timer_sleep_ms( int ms );
int timer_frame( libspectrum_dword last_tstates );
int timer_end(void);

extern float current_speed;

#endif			/* #ifndef FUSE_TIMER_H */
