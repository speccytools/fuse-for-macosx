/* timer.c: Speed routines for Fuse
   Copyright (c) 1999 Philip Kendall

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

   E-mail: pak21@cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include "alleg.h"
#include "spectrum.h"
#include "timer.h"

/* XWinAllegro uses setitimer(ITIMER_REAL,...) for its own purposes,
   so we have to use the CPU hogging method :-( */

#ifdef TIMER_HOGCPU

int last_retrace_count;
DWORD next_delay;

void timer_init(void)
{
  install_timer();

  last_retrace_count=retrace_count;
  next_delay=0;
}

/* If 1/70 of a second should have elapsed, wait until it has */
void timer_delay(void)
{
  if(tstates>=next_delay) {
    while(retrace_count==last_retrace_count) { rest(1); }
    last_retrace_count=retrace_count;
    next_delay+=(machine.hz)/70;
  }
}

/* If we've got setitimer(...) available (and we're not using
   XWinAllegro), we can use this to wake up again after sleeping =>
   don't hog the CPU :-) */
#else

#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

void timer_init(void);
static void timer_setup_timer(void);
static void timer_setup_handler(void);
void timer_signal(int signal);

void timer_init(void)
{
  timer_setup_handler();
  timer_setup_timer();
}

static void timer_setup_timer(void)
{
  struct itimerval timer;
  timer.it_interval.tv_sec=0;
  timer.it_interval.tv_usec=20000;
  timer.it_value.tv_sec=0;
  timer.it_value.tv_usec=20000;
  setitimer(ITIMER_REAL,&timer,0);
}

static void timer_setup_handler(void)
{
  struct sigaction handler; 
  handler.sa_handler=timer_signal;
  sigemptyset(&handler.sa_mask);
  handler.sa_flags=0;
  sigaction(SIGALRM,&handler,0);
}  

void timer_signal(int signal)
{
  return;
}

#endif
