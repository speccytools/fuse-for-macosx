/* timer.c: Speed routines for Fuse
   Copyright (c) 1999-2000 Philip Kendall

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

   E-mail: pak@ast.cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

#include "spectrum.h"
#include "timer.h"

volatile int timer_count;

void timer_init(void);
static void timer_setup_timer(void);
static void timer_setup_handler(void);
void timer_signal(int signal);

void timer_init(void)
{
  timer_count=0;
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
  sigaction(SIGALRM,&handler,NULL);
}  

void timer_signal(int signal)
{
  timer_count++;
}

void timer_sleep(void)
{
  if(timer_count<=0) pause();
}
