/* options.c: Options dialog box
   Copyright (c) 2001 Philip Kendall

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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "display.h"
#include "keyboard.h"
#include "settings.h"
#include "ui.h"
#include "uidisplay.h"
#include "widget.h"

static settings_info settings;

static int widget_options_show_all( settings_info *settings );
static int widget_options_print_option( int number, const char* string,
					int value );
static int widget_options_print_value( int number, int value );
static void widget_options_keyhandler( int key );

int widget_options( void )
{
  int error;

  error = widget_timer_init();
  if( error ) return error;

  /* Set up the key handler */
  widget_keyhandler = widget_options_keyhandler;

  /* Get a copy of the current settings */
  error = settings_copy( &settings, &settings_current );
  if( error ) {
    widget_timer_end();
    return error;
  }

  /* Draw the dialog box */
  widget_dialog_with_border( 1, 2, 30, 3 );
  error = widget_options_show_all( &settings );
  if( error ) {
    widget_timer_end();
    return error;
  }

  /* And note we're in a widget */
  widget_active = 1;
  widget_finished = 0;

  while( ! widget_finished ) { 

    /* Go to sleep for a bit */
    pause();

    /* Process any events */
    ui_event();

  }

  /* We've finished with the widget */
  widget_active = 0;

  /* Deactivate the widget's timer */
  widget_timer_end();

  /* Force the normal screen to be redisplayed */
  display_refresh_all();

  /* If we exited normally, actually set the options */
  if( widget_finished == WIDGET_FINISHED_OK ) {
    error = settings_copy( &settings_current, &settings );
    if( error ) return error;
  }

  return 0;
}

static int widget_options_show_all( settings_info *settings )
{
  int error;

  /* Blank the main display area */
  widget_dialog( 1, 2, 30, 3 );

  error = widget_options_print_option( 0, "Issue (2) emulation",
				       settings->issue2 );
  if( error ) return error;

  error = widget_options_print_option( 1, "(F)ast tape loading",
				       settings->tape_traps );
  if( error ) return error;

  error = widget_options_print_option( 2, "(A)Y stereo separation",
				       settings->stereo_ay );
  if( error ) return error;

  uidisplay_lines( DISPLAY_BORDER_HEIGHT + 16,
		   DISPLAY_BORDER_HEIGHT + 16 + 16 );

  return 0;
}

static int widget_options_print_option( int number, const char* string,
					int value )
{
  char buffer[29];

  snprintf( buffer, 23, "%-22s", string );
  strcat( buffer, " : " );
  strcat( buffer, value ? " On" : "Off" );

  widget_printstring( 2, number+2, WIDGET_COLOUR_FOREGROUND, buffer );

  uidisplay_lines( DISPLAY_BORDER_HEIGHT + (number+2)*8,
		   DISPLAY_BORDER_HEIGHT + (number+3)*8  );

  return 0;
}

static int widget_options_print_value( int number, int value )
{
  widget_rectangle( 27*8, (number+2)*8, 24, 8, WIDGET_COLOUR_BACKGROUND );
  widget_printstring( 27, number+2, WIDGET_COLOUR_FOREGROUND,
		      value ? " On" : "Off" );
  uidisplay_lines( DISPLAY_BORDER_HEIGHT + (number+2)*8,
		   DISPLAY_BORDER_HEIGHT + (number+3)*8  );
  return 0;
}


static void widget_options_keyhandler( int key )
{
  int error;

  switch( key ) {
    
  case KEYBOARD_1: /* 1 used as `Escape' generates `Edit', which is Caps + 1 */
    widget_finished = WIDGET_FINISHED_CANCEL;
    break;

  case KEYBOARD_2:
    settings.issue2 = ! settings.issue2;
    error = widget_options_print_value( 0, settings.issue2 );
    if( error ) return;
    break;

  case KEYBOARD_f:
    settings.tape_traps = ! settings.tape_traps;
    error = widget_options_print_value( 1, settings.tape_traps );
    if( error ) return;
    break;

  case KEYBOARD_a:
    settings.stereo_ay = ! settings.stereo_ay;
    error = widget_options_print_value( 2, settings.stereo_ay );
    if( error ) return;
    break;

  case KEYBOARD_Enter:
    widget_finished = WIDGET_FINISHED_OK;
    break;

  }
}
