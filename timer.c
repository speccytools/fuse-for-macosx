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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "fuse.h"
#include "event.h"
#include "settings.h"
#include "sound.h"
#include "sound/lowlevel.h"
#include "tape.h"
#include "timer.h"
#include "ui/ui.h"
#include "ula.h"

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

float current_speed = 100.0;

static timer_type start_time;

static const int TEN_MS = 10;

int
timer_estimate_speed( void )
{
  timer_type current_time;
  float difference;
  int error;

  if( frames_until_update-- ) return 0;

  error = timer_get_real_time( &current_time ); if( error ) return error;

  if( samples < 10 ) {

    /* If we don't have enough data, assume we're running at the desired
       speed :-) */
    current_speed = settings_current.emulation_speed;

  } else {

    difference =
      timer_get_time_difference( &current_time,
				 &stored_times[ next_stored_time ] );
    current_speed = 100 * ( 10.0 / difference );

  }

  ui_statusbar_update_speed( current_speed );

  stored_times[ next_stored_time ] = current_time;

  next_stored_time = ( next_stored_time + 1 ) % 10;
  frames_until_update = 
    ( machine_current->timings.processor_speed /
    machine_current->timings.tstates_per_frame ) - 1;

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

#elif defined(WIN32)            /* #ifdef UI_SDL */

int
timer_get_real_time( timer_type *real_time )
{
  *real_time = GetTickCount();

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
  Sleep( ms );
}

#else                           /* #ifdef UI_SDL */

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
timer_add_time_difference( timer_type *a, long msec )
{
  a->tv_usec += msec * 1000;
  if( a->tv_usec >= 1000000 ) {
    a->tv_usec -= 1000000;
    a->tv_sec += 1;
  } else if( a->tv_usec < 0 ) {
    a->tv_usec += 1000000;
    a->tv_sec -= 1;
  }
}

void
timer_sleep_ms( int ms )
{
  usleep( ms * 1000 );
}

#endif            /* #ifdef UI_SDL */

int
timer_estimate_reset( void )
{
  int error = timer_get_real_time( &start_time ); if( error ) return error;
  samples = 0;
  next_stored_time = 0;
  frames_until_update = 0;

  return 0;
}

int
timer_init( void )
{
  int error = timer_get_real_time( &start_time ); if( error ) return error;

  error = event_add( 0, EVENT_TYPE_TIMER );
  if( error ) return error;

  return 0;
}

int
timer_end( void )
{
  return event_remove_type( EVENT_TYPE_TIMER );
}

#if defined SOUND_SDL || defined SOUND_COREAUDIO

/* Callback-style sound based timer */
#include "sound/sfifo.h"

extern sfifo_t sound_fifo;

int
timer_frame_callback_sound( libspectrum_dword last_tstates )
{
  for(;;) {

    /* Sleep while fifo is full */
    if( sfifo_space( &sound_fifo ) < sound_framesiz ) {
      timer_sleep_ms( TEN_MS );
    } else {
      break;
    }

  }

  if( event_add( last_tstates + machine_current->timings.tstates_per_frame,
                 EVENT_TYPE_TIMER ) )
    return 1;

  return 0;
}

#else                           /* #if defined SOUND_SDL || defined SOUND_COREAUDIO */

/* Blocking socket-style sound based timer */
int
timer_frame_callback_sound( libspectrum_dword last_tstates )
{
  if( event_add( last_tstates + machine_current->timings.tstates_per_frame,
                 EVENT_TYPE_TIMER ) )
    return 1;

  return 0;
}
  
#endif                          /* #if defined SOUND_SDL || defined SOUND_COREAUDIO */

int
timer_frame( libspectrum_dword last_tstates )
{
  int error;
  timer_type current_time;
  float difference;
  float speed = ( settings_current.emulation_speed < 1 ?
		  100                                  :
		  settings_current.emulation_speed ) / 100.0;
  long tstates;

  if( sound_enabled )
    return timer_frame_callback_sound( last_tstates );

  /* If we're fastloading, just schedule another check in a frame's time
     and do nothing else */
  if( settings_current.fastload && tape_is_playing() ) {

    libspectrum_dword next_check_time =
      last_tstates + machine_current->timings.tstates_per_frame;

    if( event_add( next_check_time, EVENT_TYPE_TIMER ) )
      return 1;

  } else {

    while( 1 ) {

      error = timer_get_real_time( &current_time ); if( error ) return 1;
      
      difference = timer_get_time_difference( &current_time, &start_time );

      /* Sleep while we are still 10ms ahead */
      if( difference < 0 ) {
	timer_sleep_ms( TEN_MS );
      } else {
	break;
      }

    }

    error = timer_get_real_time( &current_time ); if( error ) return 1;
    
    difference = timer_get_time_difference( &current_time, &start_time );
  
    tstates = ( ( difference + TEN_MS / 1000.0 ) *
		machine_current->timings.processor_speed
		) * speed + 0.5;
  
    if( event_add( last_tstates + tstates, EVENT_TYPE_TIMER ) ) return 1;

    start_time = current_time;
    timer_add_time_difference( &start_time, TEN_MS );

  }

  return 0;
}
