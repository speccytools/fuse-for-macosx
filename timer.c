/* timer.c: Speed routines for Fuse
   Copyright (c) 1999-2003 Philip Kendall, Marek Januszewski

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

int
timer_estimate_reset( void )
{
  samples = 0;
  next_stored_time = 0;
  frames_until_update = 0;
  return 0;
}

int
timer_estimate_speed( void )
{
  timer_type current_time;
  float difference, current_speed;
  int error;

  if( frames_until_update-- ) return 0;

  error = timer_get_real_time( &current_time ); if( error ) return error;

  if( samples == 0 ) {

    current_speed = 0;

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

#ifndef WIN32

int
timer_get_real_time( timer_type *time )
{
  int error;

  error = gettimeofday( time, NULL );
  if( error ) {
    ui_error( UI_ERROR_INFO, "error getting time: %s", strerror( errno ) );
    return 1;
  }

  return 0;
}

float
timer_get_time_difference( timer_type *a, timer_type *b )
{
  return ( a->tv_sec - b->tv_sec ) + ( a->tv_usec - b->tv_usec ) / 1000000.0;
}

#else				/* #ifndef WIN32 */

int
timer_get_real_time( timer_type *time )
{
  *time = GetTickCount();
  return 0;
}

float
timer_get_time_difference( timer_type *a, timer_type *b )
{
  return ( *a - *b ) / 1000000.0;
}

#endif				/* #ifndef WIN32 */

/*
 * Routines for speed control; used either when sound is not in use, or
 * when the SDL sound routines are being used
 */

volatile float timer_count;

int
timer_init( void )
{
  int error;

  error = timer_push( 20, TIMER_FUNCTION_TICK ); if( error ) return error;

  timer_count = 0.0;

  return 0;
}

int
timer_end( void )
{
  int error;

  error = timer_pop(); return error;

  return 0;
}

void
timer_sleep( void )
{
  int speed;
  /* Go to sleep iff we're emulating things fast enough */
  while( timer_count <= 0.0 ) timer_pause();

  /* And note that we've done a frame's worth of instructions */
  speed = settings_current.emulation_speed < 1 ?
          100                                  :
          settings_current.emulation_speed;

  timer_count -= 100.0 / speed;
}

static void
timer_tick( void )
{
  /* If the emulator is running, note that time has passed. Don't
     allow too big a 'backlog' to build up though or things will run
     too fast if the emulator suddenly receives more time */
  if( !fuse_emulation_paused && timer_count < 10.0 ) timer_count += 1.0;
}

#ifdef DEBUG_MODE

int timer_push( long usec, timer_function_type which ) { return 0; }
int timer_pop( void ) { return 0; }
void timer_pause( void ) { }

#else				/* #ifdef DEBUG_MODE */

#ifdef HAVE_LIB_GLIB
#include <glib.h>
#else				/* #ifdef HAVE_LIB_GLIB */
#include <libspectrum.h>
#endif				/* #ifdef HAVE_LIB_GLIB */

#include "fuse.h"
#include "timer.h"
#include "ui/ui.h"

#ifndef WIN32

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "compat.h"

/* Stacks for the old timers and signal handlers */
static GSList *old_timers = NULL, *old_handlers = NULL;

static void signal_wake( int signo );
static void signal_tick( int signo );

int
timer_push( int msec, timer_function_type which )
{
  void (*func)( int );
  struct sigaction handler, *old_handler;
  struct itimerval timer, *old_timer;
  int error;

  switch( which ) {

  case TIMER_FUNCTION_WAKE: func = signal_wake; break;
  case TIMER_FUNCTION_TICK: func = signal_tick; break;

  default:
    ui_error( UI_ERROR_ERROR, "attempt to install unknown timer function %d",
	      which );
    return 1;
  }

  old_handler = malloc( sizeof( struct sigaction ) );
  if( !old_handler ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  old_timer = malloc( sizeof( struct itimerval ) );
  if( !old_timer ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    free( old_handler );
    return 1;
  }

  handler.sa_handler = func;
  sigemptyset( &handler.sa_mask );
  handler.sa_flags = 0;

  error = sigaction( SIGALRM, &handler, old_handler );
  if( error ) {
    ui_error( UI_ERROR_ERROR, "error setting signal handler: %s",
	      strerror( errno ) );
    free( old_handler ); free( old_timer );
    return error;
  }

  timer.it_interval.tv_sec  = 0;
  timer.it_interval.tv_usec = msec * 1000;
  timer.it_value.tv_sec     = 0;
  timer.it_value.tv_usec    = msec * 1000;

  error = setitimer( ITIMER_REAL, &timer, old_timer );
  if( error ) {
    ui_error( UI_ERROR_ERROR, "error setting interval timer: %s",
	      strerror( errno ) );
    sigaction( SIGALRM, old_handler, NULL );
    free( old_handler ); free( old_timer );
    return error;
  }

  old_handlers = g_slist_prepend( old_handlers, old_handler );
  old_timers   = g_slist_prepend( old_timers,   old_timer   );

  return 0;
}

int
timer_pop( void )
{
  struct sigaction *old_handler;
  struct itimerval *old_timer;
  int error1, error2;

  if( old_handlers ) {

    old_handler = old_handlers->data;
    old_handlers = g_slist_remove( old_handlers, old_handler );

    old_timer = old_timers->data;
    old_timers = g_slist_remove( old_timers, old_timer );

    error1 = sigaction( SIGALRM, old_handler, NULL );
    if( error1 )
      ui_error( UI_ERROR_ERROR, "error restoring old signal handler: %s",
		strerror( errno ) );

    error2 = setitimer( ITIMER_REAL, old_timer, NULL );
    if( error2 )
      ui_error( UI_ERROR_ERROR, "error restoring old interval timer: %s",
		strerror( errno ) );

    free( old_handler ); free( old_timer );

    return error1 || error2;

  } else {

    return 0;

  }
}

void
timer_pause( void )
{
  pause();
}

static void signal_wake( int signo GCC_UNUSED ) { }

static void
signal_tick( int signo GCC_UNUSED )
{
  timer_tick();
}

#else				/* #ifndef WIN32 */

/*
 * Win32-specific timing routines below here
 */

#include <windows.h>

static GSList *timer_ids;

static void CALLBACK signal_wake( UINT wTimerID, UINT msg, DWORD dwUser,
				  DWORD dw1, DWORD dw2 );
static void CALLBACK signal_tick( UINT wTimerID, UINT msg, DWORD dwUser,
				  DWORD dw1, DWORD dw2 );

#define TARGET_RESOLUTION 1	/* 1-millisecond target resolution */

int
timer_push( int msec, timer_function_type which )
{
  void CALLBACK (*func)( UINT, UINT, DWORD, DWORD, DWORD );
  MMRESULT *wTimerID;
  TIMECAPS tc;
  UINT wTimerRes;

  switch( which ) {

  case TIMER_FUNCTION_WAKE: func = signal_wake; break;
  case TIMER_FUNCTION_TICK: func = signal_tick; break;

  default:
    ui_error( UI_ERROR_ERROR, "attempt to install unknown timer function %d",
	      which );
    return 1;
  }

  wTimerID = malloc( sizeof( MMRESULT ) );
  if( !wTimerID ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }
    
  if( timeGetDevCaps( &tc, sizeof( TIMECAPS ) ) != TIMERR_NOERROR ) {
    ui_error( UI_ERROR_ERROR, "error getting timer capabilities" );
    free( wTimerID );
    return 1;
  }
    
  wTimerRes = min( max( tc.wPeriodMin, TARGET_RESOLUTION ), tc.wPeriodMax );

  timeBeginPeriod( wTimerRes );

  *wTimerID = timeSetEvent( msec, wTimerRes, func, (DWORD)NULL,
			    TIME_PERIODIC );
  if( !(*wTimerID) ) {
    ui_error( UI_ERROR_ERROR, "error setting timer" );
    free( wTimerID );
    return 1;
  }

  timer_ids = g_slist_prepend( timer_ids, wTimerID );

  return 0;
}

int
timer_pop( void )
{
  MMRESULT *wTimerID;

  if( timer_ids ) {

    wTimerID = timer_ids->data;

    timer_ids = g_slist_remove( timer_ids, wTimerID );

    timeKillEvent( *wTimerID );	/* cancel the event */

    free( wTimerID );
  }

  return 0;
}

void
timer_pause( void )
{
  Sleep( 0 );
}

static void CALLBACK
signal_wake( UINT wTimerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2 ) { }

static void CALLBACK
signal_tick( UINT wTimerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2 )
{
  timer_tick();
}

#endif				/* #ifndef WIN32 */

#endif				/* #ifndef DEBUG_MODE */
