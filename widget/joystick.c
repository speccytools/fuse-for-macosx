/* joystick.c: joystick selection widget
   Copyright (c) 2004 Philip Kendall

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

#include "joystick.h"
#include "widget_internals.h"

/* Data for drawing the cursor */
static joystick_type_t current_joystick;
static char descriptions[ JOYSTICK_TYPE_COUNT ][40];

int *setting;

int
widget_joystick_draw( void *data )
{
  joystick_type_t i;

  if( data ) {

    int which = *(int*)data;

    switch( which ) {

    case 0: setting = &( settings_current.joystick_1_output ); break;
    case 1: setting = &( settings_current.joystick_2_output ); break;
    case JOYSTICK_KEYBOARD:
      setting = &( settings_current.joystick_keyboard_output ); break;

    }

  }

  for( i = 0; i < JOYSTICK_TYPE_COUNT; i++ ) {
    snprintf( descriptions[i], 40, "(%c): %s", ((char)i)+'A',
	      joystick_name[i] );
    descriptions[i][ 28 ] = '\0';
  }
    
  /* Blank the main display area */
  widget_dialog_with_border( 1, 2, 30, JOYSTICK_TYPE_COUNT + 2 );

  widget_printstring( 8, 2, WIDGET_COLOUR_FOREGROUND, "Select joystick" );

  for( i = 0; i < JOYSTICK_TYPE_COUNT; i++ ) {
    if( current_joystick == i ) {
      widget_rectangle( 2*8, (i+4)*8, 28*8, 1*8, WIDGET_COLOUR_FOREGROUND );
      widget_printstring( 2, i+4, WIDGET_COLOUR_BACKGROUND, descriptions[i] );
    } else {
      widget_printstring( 2, i+4, WIDGET_COLOUR_FOREGROUND, descriptions[i] );
    }
  }

  widget_display_lines( 2, JOYSTICK_TYPE_COUNT + 2 );

  return 0;
}

void
widget_joystick_keyhandler( input_key key )
{
  switch( key ) {

#if 0
  case INPUT_KEY_Resize:	/* Fake keypress used on window resize */
    widget_joystick_draw( NULL );
    break;
#endif

  case INPUT_KEY_Escape:
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    return;

  case INPUT_KEY_Return:
    widget_end_widget( WIDGET_FINISHED_OK );
    return;

  default:	/* Keep gcc happy */
    break;

  }

  if( key >= INPUT_KEY_a && key <= INPUT_KEY_z &&
      key - INPUT_KEY_a < (ptrdiff_t)JOYSTICK_TYPE_COUNT ) {
    
    /* Remove the old highlight */
    widget_rectangle( 2 * 8, ( current_joystick + 4 ) * 8, 28 * 8, 1 * 8,
		      WIDGET_COLOUR_BACKGROUND );
    widget_printstring( 2, current_joystick + 4, WIDGET_COLOUR_FOREGROUND,
			descriptions[ current_joystick ] );

    /*  draw the new one */
    current_joystick = key - INPUT_KEY_a;

    widget_rectangle( 2 * 8, ( current_joystick + 4 ) * 8, 28 * 8, 1 * 8,
		      WIDGET_COLOUR_FOREGROUND );
    widget_printstring( 2, current_joystick + 4, WIDGET_COLOUR_BACKGROUND,
			descriptions[ current_joystick ] );

    widget_display_lines( 2, JOYSTICK_TYPE_COUNT + 2 );
  }
}

int
widget_joystick_finish( widget_finish_state finished )
{
  if( finished == WIDGET_FINISHED_OK ) {
    *setting = current_joystick;
    widget_end_all( WIDGET_FINISHED_OK );
  }

  return 0;
}

#endif				/* #ifdef USE_WIDGET */
