/* select.c: machine selection widget
   Copyright (c) 2001-2004 Philip Kendall, Witold Filipczyk

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

#include <string.h>

#include "widget_internals.h"

/* Data for drawing the cursor */
static size_t highlight_line;
static char **descriptions;
static char *buffer;

const char *title;
const char **options;
static size_t count;
static int *result;

int
widget_select_draw( void *data )
{
  int i;

  if( data ) {

    widget_select_t *ptr = data;

    title = ptr->title;
    options = ptr->options;
    count = ptr->count;
    result = &( ptr->result );

    highlight_line = ptr->current;

    descriptions = malloc( count * sizeof( char* ) );
    if( !descriptions ) {
      ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
      return 1;
    }

    buffer = malloc( count * 40 );
    if( !buffer ) {
      ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
      free( descriptions );
      return 1;
    }

  }

  /* Blank the main display area */
  widget_dialog_with_border( 1, 2, 30, count + 2 );

  widget_printstring( 15 - strlen( title ) / 2, 2, WIDGET_COLOUR_FOREGROUND,
		      title );

  for( i = 0; i < count; i++ ) {
    descriptions[i] = &buffer[ i * 40 ];
    snprintf( descriptions[i], 40, "(%c): %s", ((char)i)+'A', options[ i ] );
    descriptions[i][ 28 ] = '\0';
    
    if( i == highlight_line ) {
      widget_rectangle( 2*8, (i+4)*8, 28*8, 1*8, WIDGET_COLOUR_FOREGROUND );
      widget_printstring( 2, i+4, WIDGET_COLOUR_BACKGROUND, descriptions[i] );
    } else {
      widget_printstring( 2, i+4, WIDGET_COLOUR_FOREGROUND, descriptions[i] );
    }
  }

  widget_display_lines( 2, count + 2 );

  return 0;
}

void
widget_select_keyhandler( input_key key )
{
  int new_highlight_line = 0;
  int cursor_pressed = 0;
  switch( key ) {

#if 0
  case INPUT_KEY_Resize:	/* Fake keypress used on window resize */
    widget_select_draw( NULL );
    break;
#endif

  case INPUT_KEY_Escape:
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    return;

  case INPUT_KEY_Return:
    widget_end_widget( WIDGET_FINISHED_OK );
    return;

  case INPUT_KEY_Up:
  case INPUT_KEY_7:
    if ( highlight_line ) {
      new_highlight_line = highlight_line - 1;
      cursor_pressed = 1;
    }
    break;

  case INPUT_KEY_Down:
  case INPUT_KEY_6:
    if ( highlight_line + 1 < (ptrdiff_t)machine_count ) {
      new_highlight_line = highlight_line + 1;
      cursor_pressed = 1;
    }
    break;

  default:	/* Keep gcc happy */
    break;

  }

  if( cursor_pressed                                  ||
      ( key >= INPUT_KEY_a && key <= INPUT_KEY_z &&
	key - INPUT_KEY_a < (ptrdiff_t)count        )
    ) {
    
    /* Remove the old highlight */
    widget_rectangle( 2*8, (highlight_line+4)*8, 28*8, 1*8,
		      WIDGET_COLOUR_BACKGROUND );
    widget_printstring( 2, highlight_line+4, WIDGET_COLOUR_FOREGROUND,
			descriptions[ highlight_line ] );

    /*  draw the new one */
    if( cursor_pressed ) {
      highlight_line = new_highlight_line;
    } else {
      highlight_line = key - INPUT_KEY_a;
    }
    
    widget_rectangle( 2*8, (highlight_line+4)*8, 28*8, 1*8,
		      WIDGET_COLOUR_FOREGROUND );
    widget_printstring( 2, highlight_line+4, WIDGET_COLOUR_BACKGROUND,
			descriptions[ highlight_line ] );
    
    widget_display_lines( 2, count + 2 );
  }
}

int widget_select_finish( widget_finish_state finished )
{
  free( buffer );
  free( descriptions );

  if( finished == WIDGET_FINISHED_OK ) {
    *result = highlight_line;
    widget_end_all( WIDGET_FINISHED_OK );
  } else {
    *result = -1;
  }

  return 0;
}

#endif				/* #ifdef USE_WIDGET */
