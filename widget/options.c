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
#include "ui/uidisplay.h"
#include "widget.h"

static settings_info settings;

static int widget_options_show_all( settings_info *settings );
static int widget_options_print_option( int number, const char* string,
					int value );
static int widget_options_print_value( int number, int value );

int widget_options_draw( void )
{
  int error;

  /* Get a copy of the current settings */
  error = settings_copy( &settings, &settings_current );
  if( error ) return error;

  /* Draw the dialog box */
  widget_dialog_with_border( 1, 2, 30, 6 );
  error = widget_options_show_all( &settings );
  if( error ) return error;

  uidisplay_lines( DISPLAY_BORDER_HEIGHT + 16,
		   DISPLAY_BORDER_HEIGHT + 16 + 48 );

  return 0;

}

int widget_options_finish( widget_finish_state finished )
{
  int error;

  /* If we exited normally, actually set the options */
  if( finished == WIDGET_FINISHED_OK ) {
    error = settings_copy( &settings_current, &settings );
    if( error ) return error;
  }

  return 0;
}

static int widget_options_show_all( settings_info *settings )
{
  int error;

  widget_printstring( 9, 2, WIDGET_COLOUR_FOREGROUND, "General Options" );

  error = widget_options_print_option( 0, "Issue (2) emulation",
				       settings->issue2 );
  if( error ) return error;

  error = widget_options_print_option( 1, "(K)empston joystick",
				       settings->joy_kempston );
  if( error ) return error;

  error = widget_options_print_option( 2, "(F)ast tape loading",
				       settings->tape_traps );
  if( error ) return error;

  error = widget_options_print_option( 3, "(A)Y stereo separation",
				       settings->stereo_ay );
  if( error ) return error;

  return 0;
}

static int widget_options_print_option( int number, const char* string,
					int value )
{
  char buffer[29];

  snprintf( buffer, 23, "%-22s", string );
  strcat( buffer, " : " );
  strcat( buffer, value ? " On" : "Off" );

  widget_printstring( 2, number+4, WIDGET_COLOUR_FOREGROUND, buffer );

  uidisplay_lines( DISPLAY_BORDER_HEIGHT + (number+4)*8,
		   DISPLAY_BORDER_HEIGHT + (number+5)*8  );

  return 0;
}

static int widget_options_print_value( int number, int value )
{
  widget_rectangle( 27*8, (number+4)*8, 24, 8, WIDGET_COLOUR_BACKGROUND );
  widget_printstring( 27, number+4, WIDGET_COLOUR_FOREGROUND,
		      value ? " On" : "Off" );
  uidisplay_lines( DISPLAY_BORDER_HEIGHT + (number+4)*8,
		   DISPLAY_BORDER_HEIGHT + (number+5)*8  );
  return 0;
}


void widget_options_keyhandler( int key )
{
  int error;

  switch( key ) {
    
  case KEYBOARD_1: /* 1 used as `Escape' generates `Edit', which is Caps + 1 */
    widget_return[ widget_level ].finished = WIDGET_FINISHED_CANCEL;
    break;

  case KEYBOARD_2:
    settings.issue2 = ! settings.issue2;
    error = widget_options_print_value( 0, settings.issue2 );
    if( error ) return;
    break;

  case KEYBOARD_k:
    settings.joy_kempston = ! settings.joy_kempston;
    error = widget_options_print_value( 1, settings.joy_kempston );
    if( error ) return;
    break;

  case KEYBOARD_f:
    settings.tape_traps = ! settings.tape_traps;
    error = widget_options_print_value( 2, settings.tape_traps );
    if( error ) return;
    break;

  case KEYBOARD_a:
    settings.stereo_ay = ! settings.stereo_ay;
    error = widget_options_print_value( 3, settings.stereo_ay );
    if( error ) return;
    break;

  case KEYBOARD_Enter:
    widget_return[ widget_level ].finished = WIDGET_FINISHED_OK;
    break;

  }
}
