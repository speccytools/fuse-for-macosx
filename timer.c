/* timer.c: Speed routines for Fuse
   Copyright (c) 1999-2004 Philip Kendall, Marek Januszewski, Fredrick Meunier

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

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "fuse.h"
#include "settings.h"
#include "timer.h"
#include "ui/ui.h"

/*
 * Routines for estimating emulation speed
 */

/* The actual time at the end of each of the last 10 emulated seconds */
static timer_type stored_times[10];

/* Which is the next entry in 'stored_times' that we will update */
static size_t next_stored_time;

/* The number of frames until we next update 'stored_times' */
static int frames_until_update;

/* The number of time samples we have for estimating speed */
static int samples;

static timer_type speccy_time;

float current_speed = 100.0;

int
timer_estimate_reset( void )
{
  int error = timer_get_real_time( &speccy_time ); if( error ) return error;
  samples = 0;
  next_stored_time = 0;
  frames_until_update = 0;
  return 0;
}

int
timer_estimate_speed( void )
{
  timer_type current_time;
  float difference;
  int error;

  if( frames_until_update-- ) return 0;

  error = timer_get_real_time( &current_time ); if( error ) return error;

  if( samples == 0 ) {

    /* If we don't have any data, assume we're running at the desired
       speed :-) */
    current_speed = settings_current.emulation_speed;

  } else if( samples < 10 ) {

    difference = timer_get_time_difference( &current_time, &stored_times[0] );
    current_speed = 100 * ( samples / difference );

  } else {

    difference =
      timer_get_time_difference( &current_time,
				 &stored_times[ next_stored_time ] );
      current_speed = 100 * ( 10.0 / difference );

  }

  ui_statusbar_update_speed( current_speed );

  stored_times[ next_stored_time ] = current_time;

  next_stored_time = ( next_stored_time + 1 ) % 10;
  frames_until_update = 49;

  samples++;

  return 0;
}

#ifdef UI_SDL

int
timer_get_real_time( timer_type *real_time )
{
  *real_time = SDL_GetTicks();

  return 0;
}

float
timer_get_time_difference( timer_type *a, timer_type *b )
{
  return ( *a - *b ) / 1000.0;
}

void
timer_add_time_difference( timer_type *a, long usec )
{
  *a += usec / 1000;
}

void
timer_sleep_ms( int ms )
{
  SDL_Delay( ms );
}

#else            /* #ifdef UI_SDL */

int
timer_get_real_time( timer_type *real_time )
{
  int error;

  error = gettimeofday( real_time, NULL );
  if( error ) {
    ui_error( UI_ERROR_ERROR, "error getting time: %s", strerror( errno ) );
    return 1;
  }

  return 0;
}

float
timer_get_time_difference( timer_type *a, timer_type *b )
{
  return ( a->tv_sec - b->tv_sec ) + ( a->tv_usec - b->tv_usec ) / 1000000.0;
}

void
timer_add_time_difference( timer_type *a, long usec )
{
  a->tv_usec += usec;
  if( a->tv_usec >= 1000000 ) {
    a->tv_usec -= 1000000;
    a->tv_sec += 1;
  }
}

void
timer_sleep_ms( int ms )
{
  usleep( ms * 1000 );
}

#endif            /* #ifdef UI_SDL */

/*
 * Routines for speed control; used either when sound is not in use, or
 * when the SDL sound routines are being used
 */

int
timer_init( void )
{
  int error;

  error = timer_get_real_time( &speccy_time ); if( error ) return error;

  return 0;
}

int
timer_end( void )
{
  return 0;
}

void
timer_sleep( void )
{
  int error;
  timer_type current_time;
  float difference;
  int speed = settings_current.emulation_speed < 1 ?
              100                                  :
              settings_current.emulation_speed;

  int speccy_frame_time_usec = 20000.0 * 100.0 / speed;

start:
  /* Get current time */
  error = timer_get_real_time( &current_time ); if( error ) return;

  /* Compare current time with emulation time */
  difference = timer_get_time_difference( &current_time, &speccy_time );

  /* If speccy is less than 2 frames ahead of real time don't sleep */
  /* If the next frame is due in less than 5ms just do it now */
  /* (linear interpolation) */
  /* FIXME Can now have interrupts at true 48k 50.04Hz etc. */
  if( difference < ( ( 2 * speccy_frame_time_usec / 1000000.0 ) - 0.005 ) ) {
    /* Check at or around 100Hz (usual timer resolution on *IX) */
    timer_sleep_ms( 10 );
    goto start;
  }

  /* And note that we've done a frame's worth of Speccy time */
  timer_add_time_difference( &speccy_time, speccy_frame_time_usec );
}
