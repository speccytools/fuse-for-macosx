/* widget.c: Simple dialog boxes for all user interfaces.
   Copyright (c) 2001,2002 Matan Ziv-Av, Philip Kendall, Russell Marks

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
#include "ui/uidisplay.h"
#include "keyboard.h"
#include "options.h"
#include "screenshot.h"
#include "timer.h"
#include "widget_internals.h"

static void printchar(int x, int y, int col, int ch);

static char widget_font[768];

/* The current widget keyhandler */
widget_keyhandler_fn widget_keyhandler;

/* The data used for recursive widgets */
typedef struct widget_recurse_t {

  widget_type type;		/* Which type of widget are we? */
  void *data;			/* What data were we passed? */

  int finished;			/* Have we finished this widget yet? */

} widget_recurse_t;

static widget_recurse_t widget_return[10]; /* The stack to recurse on */

/* The settings used whilst playing with an options dialog box */
settings_info widget_options_settings;

static int widget_read_font( const char *filename, size_t offset )
{
  int fd; struct stat file_info; unsigned char *buffer;

  fd = machine_find_rom( filename );
  if( fd == -1 ) {
    ui_error( UI_ERROR_ERROR, "couldn't find ROM '%s'", filename );
    return 1;
  }

  if( fstat( fd, &file_info) ) {
    ui_error( UI_ERROR_ERROR, "Couldn't stat '%s': %s", filename,
	      strerror( errno ) );
    close(fd);
    return errno;
  }

  buffer = mmap( 0, file_info.st_size, PROT_READ, MAP_SHARED, fd, 0 );
  if( buffer == (void*)-1 ) {
    ui_error( UI_ERROR_ERROR, "Couldn't mmap '%s': %s", filename,
	      strerror( errno ) );
    close(fd);
    return errno;
  }

  if( close(fd) ) {
    ui_error( UI_ERROR_ERROR, "Couldn't close '%s': %s", filename,
	      strerror( errno ) );
    munmap( buffer, file_info.st_size );
    return errno;
  }

  memcpy( widget_font, buffer+offset-1, 768 );

  if( munmap( buffer, file_info.st_size ) == -1 ) {
    ui_error( UI_ERROR_ERROR, "Couldn't munmap '%s': %s", filename,
	      strerror( errno ) );
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
            if(b&128) {
	      uidisplay_putpixel((mx<<1)+DISPLAY_BORDER_WIDTH+16*x, my+DISPLAY_BORDER_HEIGHT+8*y, col);
	      uidisplay_putpixel((mx<<1)+1+DISPLAY_BORDER_WIDTH+16*x, my+DISPLAY_BORDER_HEIGHT+8*y, col);
	    }
            b<<=1;
        }
    }
}

void widget_printstring(int x, int y, int col, const char *s)
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
    
    for(my=0;my<h;my++)for(mx=0;mx<w;mx++) {
        uidisplay_putpixel( DISPLAY_BORDER_WIDTH  + ( (x+mx) << 1 )    ,
			    DISPLAY_BORDER_HEIGHT +    y+my, col );
        uidisplay_putpixel( DISPLAY_BORDER_WIDTH  + ( (x+mx) << 1 ) + 1,
			    DISPLAY_BORDER_HEIGHT +    y+my, col );
    }
}

/* Force screen lines y to (y+h) inclusive to be redrawn */
void
widget_display_lines( int y, int h )
{
  uidisplay_lines( DISPLAY_BORDER_HEIGHT + 8 *  y,
		   DISPLAY_BORDER_HEIGHT + 8 * (y+h) + 7 );
  uidisplay_frame_end();
}

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
  size_t i;

  if( widget_filenames ) {
    for( i=0; i<widget_numfiles; i++) {
      free( widget_filenames[i]->name );
      free( widget_filenames[i] );
    }
    free( widget_filenames );
  }

  return 0;
}

/* General widget routine */

/* We don't start in a widget */
int widget_level = -1;

int widget_do( widget_type which, void *data )
{
  int error;

  /* If we don't have a UI yet, we can't output widgets */
  if( !display_ui_initialised ) return 1;

  /* We're now one widget level deeper */
  widget_level++;

  /* If we're the top-level widget, save the screen and set up the timer */
  if( ! widget_level ) {

#ifdef USE_LIBPNG
    error = screenshot_save(); if( error ) { widget_level--; return error; }
#endif				/* #ifdef USE_LIBPNG */

    error = widget_timer_init(); if( error ) { widget_level--; return error; }
  }

  /* Store what type of widget we are and what data we were given */
  widget_return[widget_level].type = which;
  widget_return[widget_level].data = data;

  /* Draw this widget */
  error = widget_data[ which ].draw( data );

  /* Set up the keyhandler for this widget */
  widget_keyhandler = widget_data[which].keyhandler;

  /* Process this widget until it returns */
  widget_return[widget_level].finished = 0;
  while( ! widget_return[widget_level].finished ) {

    /* Go to sleep for a bit */
    pause();

    /* Process any events */
    ui_event();
  }

  /* Do any post-widget processing if it exists */
  if( widget_data[which].finish ) {
    widget_data[which].finish( widget_return[widget_level].finished );
  }

  /* Now return to the previous widget level */
  widget_level--;
    
  if( widget_level >= 0 ) {

    /* If we're going back to another widget, set up its keyhandler and
       draw it again, unless it's already finished */
    if( ! widget_return[widget_level].finished ) {
      widget_keyhandler =
	widget_data[ widget_return[widget_level].type ].keyhandler;
      widget_data[ widget_return[widget_level].type ].draw(
        widget_return[widget_level].data
      );
    }

  } else {

    /* Restore the old keyboard handler */
    error = widget_timer_end();
    if( error ) return error;

    /* Refresh the Spectrum's display, including the border */
    display_refresh_all();
    display_dirty_border();

  }

  return 0;
}

/* End the currently running widget */
int
widget_end_widget( widget_finish_state state )
{
  widget_return[ widget_level ].finished = state;
  return 0;
}

/* End all currently running widgets */
int widget_end_all( widget_finish_state state )
{
  int i;

  for( i=0; i<=widget_level; i++ )
    widget_return[i].finished = state;

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

  /* FIXME: Is this really necessary? Without this on Solaris, we
     often get called such taht widget_timer_old_timer.it_value is zero
     which then means we die when we reactivate this timer as it is
     disabled. Reread Stevens. */
  if( widget_timer_old_timer.it_value.tv_sec == 0 &&
      widget_timer_old_timer.it_value.tv_usec == 0 )
    {
      widget_timer_old_timer.it_value.tv_sec =
	widget_timer_old_timer.it_interval.tv_sec;

      widget_timer_old_timer.it_value.tv_usec = 
	widget_timer_old_timer.it_interval.tv_usec;
    }

  return 0;
}

/* The signal handler: just wake up */
static void widget_timer_signal( int signo GCC_UNUSED )
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
    uidisplay_putpixel( (i<<1)           + DISPLAY_BORDER_WIDTH,
			(8* y        )-4 + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( (i<<1)+1         + DISPLAY_BORDER_WIDTH,
			(8* y        )-4 + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( (i<<1)           + DISPLAY_BORDER_WIDTH,
			(8*(y+height))+3 + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( (i<<1)+1         + DISPLAY_BORDER_WIDTH,
			(8*(y+height))+3 + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
  }

  for( i=(8*y)-1; i<(8*(y+height))+1; i++ ) {
    uidisplay_putpixel( (16* x       )-4 + DISPLAY_BORDER_WIDTH,
			i                + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( (16* x       )-3 + DISPLAY_BORDER_WIDTH,
			i                + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( (16*(x+width))+3 + DISPLAY_BORDER_WIDTH,
			i                + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( (16*(x+width))+4 + DISPLAY_BORDER_WIDTH,
			i                + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
  }

  for( i=0; i<2; i++ ) {
    uidisplay_putpixel( (16* x       )-3+i + DISPLAY_BORDER_WIDTH,
			(8* y        )-2-i + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( (16* x       )-2+i + DISPLAY_BORDER_WIDTH,
			(8* y        )-2-i + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( (16*(x+width))+2-i + DISPLAY_BORDER_WIDTH,
			(8* y        )-2-i + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( (16*(x+width))+3-i + DISPLAY_BORDER_WIDTH,
			(8* y        )-2-i + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( (16* x       )-3+i + DISPLAY_BORDER_WIDTH,
			(8*(y+height))+1+i + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( (16* x       )-2+i + DISPLAY_BORDER_WIDTH,
			(8*(y+height))+1+i + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( (16*(x+width))+2-i + DISPLAY_BORDER_WIDTH,
			(8*(y+height))+1+i + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
    uidisplay_putpixel( (16*(x+width))+3-i + DISPLAY_BORDER_WIDTH,
			(8*(y+height))+1+i + DISPLAY_BORDER_HEIGHT,
			WIDGET_COLOUR_FOREGROUND );
  }

  widget_display_lines( y-1, height+2 );

  return 0;
}

/* General functions used by the options dialogs */

int widget_options_print_option( int number, const char* string, int value )
{
  char buffer[29];

  snprintf( buffer, 29, "%-22s : %s", string, value ? " On" : "Off" );
  widget_printstring( 2, number+4, WIDGET_COLOUR_FOREGROUND, buffer );

  widget_display_lines( number + 4, 1 );

  return 0;
}

int widget_options_print_value( int number, int value )
{
  widget_rectangle( 27*8, (number+4)*8, 24, 8, WIDGET_COLOUR_BACKGROUND );
  widget_printstring( 27, number+4, WIDGET_COLOUR_FOREGROUND,
		      value ? " On" : "Off" );

  widget_display_lines( number + 4, 1 );

  return 0;
}

int
widget_options_print_entry( int number, const char *prefix, int value,
			    const char *suffix )
{
  char buffer[29];

  widget_rectangle( 2*8, (number+4)*8, 28*8, 8, WIDGET_COLOUR_BACKGROUND );
  
  snprintf( buffer, 29, "%s: %d %s", prefix, value, suffix );
  widget_printstring( 2, number + 4, WIDGET_COLOUR_FOREGROUND, buffer );

  widget_display_lines( number + 4, 1 );

  return 0;
}

int widget_options_finish( widget_finish_state finished )
{
  int error;

  /* If we exited normally, actually set the options */
  if( finished == WIDGET_FINISHED_OK ) {
    error = settings_copy( &settings_current, &widget_options_settings );
    settings_free( &widget_options_settings );
    if( error ) return error;
  }

  return 0;
}

/* The widgets actually available. Make sure the order here matches the
   order defined in enum widget_type (widget.h) */

widget_t widget_data[] = {

  { widget_filesel_draw,  widget_filesel_finish, widget_filesel_keyhandler  },
  { widget_general_draw,  widget_options_finish, widget_general_keyhandler  },
  { widget_picture_draw,  NULL,                  widget_picture_keyhandler  },
  { widget_menu_draw,	  NULL,			 widget_menu_keyhandler     },
  { widget_select_draw,   widget_select_finish,  widget_select_keyhandler   },
  { widget_sound_draw,	  widget_options_finish, widget_sound_keyhandler    },
  { widget_error_draw,	  NULL,			 widget_error_keyhandler    },
  { widget_rzx_draw,      widget_options_finish, widget_rzx_keyhandler      },
  { widget_browse_draw,   widget_browse_finish,  widget_browse_keyhandler   },
  { widget_text_draw,	  widget_text_finish,	 widget_text_keyhandler     },
  { widget_debugger_draw, NULL,			 widget_debugger_keyhandler },
  { widget_roms_draw,     widget_roms_finish,	 widget_roms_keyhandler     },

};
