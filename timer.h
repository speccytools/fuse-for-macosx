/* timer.h: Speed routines for Fuse
   Copyright (c) 1999-2003 Philip Kendall

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

#ifndef FUSE_TIMER_H
#define FUSE_TIMER_H

#ifndef WIN32

#include <sys/time.h>
#include <time.h>

typedef struct timeval timer_type;

#else				/* #ifndef WIN32 */

#include <windows.h>

typedef DWORD timer_type;

#endif				/* #ifndef WIN32 */

int timer_estimate_reset( void );
int timer_estimate_speed( void );
int timer_get_real_time( timer_type *real_time );
float timer_get_time_difference( timer_type *a, timer_type *b );

extern volatile float timer_count;

int timer_init(void);
void timer_sleep(void);
int timer_end(void);

typedef enum timer_function_type {

  TIMER_FUNCTION_WAKE,
  TIMER_FUNCTION_TICK,

} timer_function_type;

int timer_push( int msec, timer_function_type which );
int timer_pop( void );
void timer_pause( void );

#endif			/* #ifndef FUSE_TIMER_H */
