/* widget.c: Simple dialog boxes for all user interfaces.
   Copyright (c) 2001 Matan Ziv-Av, Philip Kendall, Russell Marks

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
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "fuse.h"
#include "display.h"
#include "machine.h"
#include "ui.h"
#include "uidisplay.h"
#include "keyboard.h"
#include "widget.h"

static void printchar(int x, int y, int col, int ch);

static char widget_font[768];

#define ERROR_MESSAGE_MAX_LENGTH 1024

static int widget_read_font( const char *filename, size_t offset )
{
  int fd; struct stat file_info; unsigned char *buffer;

  char error_message[ ERROR_MESSAGE_MAX_LENGTH ];

  fd = machine_find_rom( filename );
  if( fd == -1 ) {
    fprintf( stderr,"%s: couldn't find ROM `%s'\n", fuse_progname, filename );
    return 1;
  }

  if( fstat( fd, &file_info) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't stat `%s'", fuse_progname, filename );
    perror( error_message );
    close(fd);
    return errno;
  }

  buffer = mmap( 0, file_info.st_size, PROT_READ, MAP_SHARED, fd, 0 );
  if( buffer == (void*)-1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't mmap `%s'", fuse_progname, filename );
    perror( error_message );
    close(fd);
    return errno;
  }

  if( close(fd) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't close `%s'", fuse_progname, filename );
    perror( error_message );
    munmap( buffer, file_info.st_size );
    return errno;
  }

  memcpy( widget_font, buffer+offset-1, 768 );

  if( munmap( buffer, file_info.st_size ) == -1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't munmap `%s'", fuse_progname, filename );
    perror( error_message );
    return errno;
  }

  return 0;

}

static void printchar(int x, int y, int col, int ch) {
    
    int mx, my;

    if((ch<32)||(ch>127)) ch=33;
    ch-=32;
    
    for(my=0; my<8; my++) {
        int b;
        b=widget_font[ch*8+my];
        for(mx=0; mx<8; mx++) {
            if(b&128) uidisplay_putpixel(mx+DISPLAY_BORDER_WIDTH+8*x, my+DISPLAY_BORDER_HEIGHT+8*y, col);
            b<<=1;
        }
    }
}

void widget_printstring(int x, int y, int col, char *s)
{
    int i;
    i=0;
    if(s) {
        while((x<32) && s[i]) {
            printchar(x,y,col,s[i]);
            i++;
            x++;
        }
    }
}

void widget_rectangle( int x, int y, int w, int h, int col )
{
    int mx, my;
    for(my=0;my<h;my++)for(mx=0;mx<w;mx++)
        uidisplay_putpixel(mx+DISPLAY_BORDER_WIDTH+x, my+DISPLAY_BORDER_HEIGHT+y, col);
}

/* Are we in a widget at the moment? (Used by the key handling routines
   to know whether to call the Spectrum or widget key handler) */
int widget_active;

/* The keyhandler to use for the current widget */
widget_keyhandler_fn widget_keyhandler;

/* Have we finished with this widget, and if so in which way? */
widget_finish_state widget_finished;

/* Global initialisation/end routines */

int widget_init( void )
{
  int error;

  error = widget_read_font( "48.rom", 15617 );
  if( error ) return error;

  widget_filenames = NULL;
  widget_numfiles = 0;

  return 0;
}

int widget_end( void )
{
  int i;

  if( widget_filenames ) {
    for( i=0; i<widget_numfiles; i++) {
      free( widget_filenames[i]->name );
      free( widget_filenames[i] );
    }
    free( widget_filenames );
  }

  return 0;
}

/* Widget timer routines */

static struct sigaction widget_timer_old_handler;
static struct itimerval widget_timer_old_timer;

static void widget_timer_signal( int signo );

int widget_timer_init( void )
{
  struct sigaction handler;
  struct itimerval timer;

  handler.sa_handler = widget_timer_signal;
  sigemptyset( &handler.sa_mask );
  handler.sa_flags = 0;
  sigaction( SIGALRM, &handler, &widget_timer_old_handler );

  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 100000UL;
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 100000UL;
  setitimer( ITIMER_REAL, &timer, &widget_timer_old_timer );

  return 0;
}

/* The signal handler: just wake up */
static void widget_timer_signal( int signo )
{
  return;
}

int widget_timer_end( void )
{
  /* Restore the old timer */
  setitimer( ITIMER_REAL, &widget_timer_old_timer, NULL);

  /* And the old signal handler */
  sigaction( SIGALRM, &widget_timer_old_handler, NULL);

  return 0;
}

/* End of timer functions */

int widget_dialog( int x, int y, int width, int height )
{
  widget_rectangle( 8*x, 8*y, 8*width, 8*height, WIDGET_COLOUR_BACKGROUND );
  return 0;
}

int widget_dialog_with_border( int x, int y, int width, int height )
{
  int i;

  widget_rectangle( 8*(x-1), 8*(y-1), 8*(width+2), 8*(height+2),
		    WIDGET_COLOUR_BACKGROUND );
  
  for( i=(8*x)-1; i<(8*(x+width))+1; i++ ) {
    uidisplay_putpixel( i                + DISPLAY_BORDER_WIDTH,
			(8* y        )-4 + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( i                + DISPLAY_BORDER_WIDTH,
			(8*(y+height))+3 + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
  }

  for( i=(8*y)-1; i<(8*(y+height))+1; i++ ) {
    uidisplay_putpixel( (8* x        )-4 + DISPLAY_BORDER_WIDTH,
			i                + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( (8*(x+width ))+3 + DISPLAY_BORDER_WIDTH,
			i                + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
  }

  for( i=0; i<2; i++ ) {
    uidisplay_putpixel( (8* x        )-3+i + DISPLAY_BORDER_WIDTH,
			(8* y        )-2-i + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( (8*(x+width) )+2-i + DISPLAY_BORDER_WIDTH,
			(8* y        )-2-i + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( (8* x        )-3+i + DISPLAY_BORDER_WIDTH,
			(8*(y+height))+1+i + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( (8*(x+width) )+2-i + DISPLAY_BORDER_WIDTH,
			(8*(y+height))+1+i + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
  }

  uidisplay_lines( DISPLAY_BORDER_HEIGHT + 8*(y       -1),
		   DISPLAY_BORDER_HEIGHT + 8*(y+height+1)  );

  return 0;
}
