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

#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

#include "compat.h"
#include "fuse.h"
#include "timer.h"

volatile float timer_count;

#ifndef WIN32

#ifndef DEBUG_MODE
/* Just places to store the old timer and signal handlers; restored
   on exit */
static struct itimerval timer_old_timer;
static struct sigaction timer_old_handler;

static void timer_setup_timer(void);
static void timer_setup_handler(void);
#endif				/* #ifndef DEBUG_MODE */
void timer_signal( int signo );

int timer_init(void)
{
#ifndef DEBUG_MODE
  timer_count = 0.0;
  timer_setup_handler();
  timer_setup_timer();
#endif				/* #ifndef DEBUG_MODE */
  return 0;
}

#ifndef DEBUG_MODE
static void timer_setup_timer(void)
{
  struct itimerval timer;
  timer.it_interval.tv_sec=0;
  timer.it_interval.tv_usec=20000;
  timer.it_value.tv_sec=0;
  timer.it_value.tv_usec=20000;
  setitimer(ITIMER_REAL,&timer,&timer_old_timer);
}

static void timer_setup_handler(void)
{
  struct sigaction handler; 
  handler.sa_handler=timer_signal;
  sigemptyset(&handler.sa_mask);
  handler.sa_flags=0;
  sigaction(SIGALRM,&handler,&timer_old_handler);
}  
#endif				/* #ifndef DEBUG_MODE */

void
timer_signal( int signo GCC_UNUSED )
{
#ifndef DEBUG_MODE
  /* If the emulator is running, note that time has passed */
  if( !fuse_emulation_paused ) timer_count += 1.0;
#endif				/* #ifndef DEBUG_MODE */
}

void timer_sleep(void)
{
#ifndef DEBUG_MODE
  /* Go to sleep iff we're emulating things fast enough */
  while( timer_count <= 0.0 ) pause();
#endif				/* #ifndef DEBUG_MODE */
}

int timer_end(void)
{
#ifndef DEBUG_MODE
  /* Restore the old timer */
  setitimer(ITIMER_REAL,&timer_old_timer,NULL);

  /* And the old signal handler */
  sigaction(SIGALRM,&timer_old_handler,NULL);

#endif				/* #ifndef DEBUG_MODE */

  return 0;

}

#else				/* #ifndef WIN32 */

/*
 * Win32-specific timing routines below here
 */

#include <windows.h>

#ifndef DEBUG_MODE

static MMRESULT wTimerID;
UINT wTimerRes;
void CALLBACK timer_signal( UINT wTimerID, UINT msg, DWORD dwUser, DWORD dw1,
			    DWORD dw2 );

#define TARGET_RESOLUTION 1	/* 1-millisecond target resolution */

#endif				/* #ifndef DEBUG_MODE */

int
timer_init( void )
{
#ifndef DEBUG_MODE

  TIMECAPS tc;
    
  timer_count = 0.0;

  if( timeGetDevCaps( &tc, sizeof( TIMECAPS ) ) != TIMERR_NOERROR ) return 1;
    
  wTimerRes = min( max( tc.wPeriodMin, TARGET_RESOLUTION ), tc.wPeriodMax );

  timeBeginPeriod(wTimerRes);

  wTimerID = timeSetEvent( 20, wTimerRes, timer_signal, (DWORD)NULL,
			   TIME_PERIODIC );
  if( !wTimerID ) return 1;

#endif				/* #ifndef DEBUG_MODE */

  return 0;
}

void
timer_sleep( void )
{
#ifndef DEBUG_MODE
  while( timer_count <= 0.0 ) Sleep(0);
#endif				/* #ifndef DEBUG_MODE */
}

int
timer_end( void )
{
#ifndef DEBUG_MODE
  timeKillEvent(wTimerID);	/* cancel the event */
  wTimerID = 0;
#endif				/* #ifndef DEBUG_MODE */
  return 0;
}

void CALLBACK
timer_signal( UINT wTimerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2 )
{
#ifndef DEBUG_MODE
  if( !fuse_emulation_paused ) timer_count += 1.0;
#endif				/* #ifndef DEBUG_MODE */
}

#endif   /* #ifndef WIN32 */
