/* text.c: simple text entry widget
   Copyright (c) 2002 Philip Kendall

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

#include <string.h>

#include "widget_internals.h"

char *widget_text_text;		/* What we return the text in */

static const char *title;	/* The window title */
static char text[40];		/* The current entry text */

static void delete_character( void );
static void append_character( char c );

int
widget_text_draw( void *data )
{
  char buffer[32];
  widget_text_t* text_data = data;

  if( data ) {
    title = text_data->title;
    strncpy( text, text_data->text, 39 ); text[39] = '\0';
  }

  widget_dialog_with_border( 1, 2, 30, 3 );

  widget_printstring( 15 - strlen( title ) / 2, 2, WIDGET_COLOUR_FOREGROUND,
		      title );

  strcpy( buffer, "[ " );
  strncpy( &buffer[2], text, 23 ); buffer[23] = '\0';
  strcat( buffer, "_" );

  widget_printstring(  2, 4, WIDGET_COLOUR_FOREGROUND, buffer );
  widget_printstring( 29, 4, WIDGET_COLOUR_FOREGROUND, "]" );

  widget_display_lines( 2, 3 );

  return 0;
}

void
widget_text_keyhandler( keyboard_key_name key, keyboard_key_name key2 )
{
  switch( key ) {

  case KEYBOARD_Resize:		/* Fake keypress used on window resize */
    widget_text_draw( NULL );
    return;
      
  case KEYBOARD_0:		/* Backspace generates DEL which is Caps + 0 */
    if( key2 == KEYBOARD_Caps ) {
      delete_character(); widget_text_draw( NULL );
      return;
    }
    break;

  case KEYBOARD_1:		/* `Esc' generates EDIT which is Caps + 1 */
    if( key2 == KEYBOARD_Caps ) {
      widget_end_widget( WIDGET_FINISHED_CANCEL );
      return;
    }
    break;

  case KEYBOARD_Enter:
    widget_end_widget( WIDGET_FINISHED_OK );
    return;

  default:			/* Keep gcc happy */
    break;

  }

  if( key >= KEYBOARD_0 && key <= KEYBOARD_9 ) {
    append_character( key );
  } else if( key >= KEYBOARD_a && key <= KEYBOARD_z ) {
    append_character( key );
  }

  widget_text_draw( NULL );

}

static void 
delete_character( void )
{
  size_t length = strlen( text );

  if( length ) text[ length - 1 ] = '\0';
}

static void
append_character( char c )
{
  size_t length = strlen( text );

  if( length < 23 ) {
    text[ length ] = c; text[ length + 1 ] = '\0';
  }
}

int
widget_text_finish( widget_finish_state finished )
{
  if( finished == WIDGET_FINISHED_OK ) {

    widget_text_text = malloc( strlen( text ) + 1 );
    if( !widget_text_text ) {
      ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
      return 1;
    }

    strcpy( widget_text_text, text );
  } else {
    widget_text_text = NULL;
  }

  return 0;
}
