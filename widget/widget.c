/* widget.c: Simple dialog boxes for all user interfaces.
   Copyright (c) 2001-2004 Matan Ziv-Av, Philip Kendall, Russell Marks

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

#ifdef USE_WIDGET

#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "fuse.h"
#include "display.h"
#include "machine.h"
#include "ui/uidisplay.h"
#include "keyboard.h"
#include "options.h"
#include "periph.h"
#include "screenshot.h"
#include "timer.h"
#include "utils.h"
#include "widget_internals.h"

#ifdef WIN32
#include <windows.h>
#endif

static void printchar(int x, int y, int col, int ch);
static void widget_putpixel( int x, int y, int colour );

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
  int fd;
  utils_file file;
  int error;

  fd = utils_find_auxiliary_file( filename, UTILS_AUXILIARY_ROM );
  if( fd == -1 ) {
    ui_error( UI_ERROR_ERROR, "couldn't find ROM '%s'", filename );
    return 1;
  }

  error = utils_read_fd( fd, filename, &file );
  if( error ) return error;

  memcpy( widget_font, file.buffer + offset - 1, 768 );

  error = utils_close_file( &file );
  if( error ) return error;

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
            if( b & 0x80 ) widget_putpixel( mx + 8 * x, my + 8 * y, col );
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
    
    for( my = 0; my < h; my++ )
      for( mx = 0; mx < w; mx++ )
        widget_putpixel( x + mx, y + my, col );
}

/* Force screen lines y to (y+h) inclusive to be redrawn */
void
widget_display_lines( int y, int h )
{
  int scale = machine_current->timex ? 2 : 1;

  uidisplay_area( 0, scale * ( DISPLAY_BORDER_HEIGHT + 8 * y ),
		  scale * DISPLAY_ASPECT_WIDTH, scale * 8 * h );
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

    error = timer_push( 100, TIMER_FUNCTION_WAKE );
    if( error ) { widget_level--; return error; }
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
    timer_pause();

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
    error = timer_pop(); if( error ) return error;

    /* Refresh the Spectrum's display, including the border */
    display_refresh_all();
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
    widget_putpixel( i, 8 *   y            - 4, WIDGET_COLOUR_FOREGROUND );
    widget_putpixel( i, 8 * ( y + height ) + 3, WIDGET_COLOUR_FOREGROUND );
  }

  for( i=(8*y)-1; i<(8*(y+height))+1; i++ ) {
    widget_putpixel( 8 *   x           - 4, i, WIDGET_COLOUR_FOREGROUND );
    widget_putpixel( 8 * ( x + width ) + 3, i, WIDGET_COLOUR_FOREGROUND );
  }

  for( i=0; i<2; i++ ) {
    widget_putpixel( 8 *   x           - 3 + i, 8 *   y            - 2 - i,
		     WIDGET_COLOUR_FOREGROUND );
    widget_putpixel( 8 * ( x + width ) + 2 - i, 8 *   y            - 2 - i,
		     WIDGET_COLOUR_FOREGROUND );
    widget_putpixel( 8 *   x           - 3 + i, 8 * ( y + height ) + 1 + i,
		     WIDGET_COLOUR_FOREGROUND );
    widget_putpixel( 8 * ( x + width ) + 2 - i, 8 * ( y + height ) + 1 + i,
		     WIDGET_COLOUR_FOREGROUND );
  }

  widget_display_lines( y-1, height+2 );

  return 0;
}

static void
widget_putpixel( int x, int y, int colour )
{
  display_putpixel( x + DISPLAY_BORDER_ASPECT_WIDTH, y + DISPLAY_BORDER_HEIGHT,
		    colour );
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

    /* Bring the peripherals list into sync with the new options */
    periph_update();
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
  { widget_scaler_draw,   widget_scaler_finish,  widget_scaler_keyhandler   },
  { widget_debugger_draw, NULL,			 widget_debugger_keyhandler },
  { widget_roms_draw,     widget_roms_finish,	 widget_roms_keyhandler     },

};

/* The statusbar handling functions */
/* TODO: make these do something useful */
int
ui_statusbar_update( ui_statusbar_item item, ui_statusbar_state state )
{
  return 0;
}

#ifndef UI_SDL
int
ui_statusbar_update_speed( float speed )
{
  return 0;
}
#endif                          /* #ifndef UI_SDL */

/* Tape browser update function. The dialog box is created every time it
   is displayed, so no need to do anything here */
int
ui_tape_browser_update( void )
{
  return 0;
}

/* FIXME: make this do something useful */
ui_confirm_save_t
ui_confirm_save( const char *message )
{
  return UI_CONFIRM_SAVE_DONTSAVE;
}

#endif				/* #ifdef USE_WIDGET */
