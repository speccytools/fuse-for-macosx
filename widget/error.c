/* error.c: The error reporting widget
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

#include <stdio.h>
#include <string.h>

#include "display.h"
#include "keyboard.h"
#include "ui/uidisplay.h"
#include "widget.h"

static int split_message( const char *message, char ***lines, size_t *count,
			  const size_t line_length );

int widget_error_draw( void *data )
{
  char *message = (char*)data;
  char **lines; size_t count;
  size_t i;

  if( split_message( message, &lines, &count, 28 ) ) return 1;
  
  widget_dialog_with_border( 1, 2, 30, count );

  for( i=0; i<count; i++ ) {
    widget_printstring( 2, i+2, WIDGET_COLOUR_FOREGROUND, lines[i] );
    free( lines[i] );
  }

  free( lines );

  uidisplay_lines( DISPLAY_BORDER_HEIGHT + 16,
		   DISPLAY_BORDER_HEIGHT + 15 + count*8 );

  return 0;
}

static int
split_message( const char *message, char ***lines, size_t *count,
	       const size_t line_length )
{
  const char *ptr = message;
  int position;

  /* Setup so we'll allocate the first line as soon as we get the first
     word */
  *lines = NULL; *count = 0; position = line_length;

  while( *ptr ) {

    /* Skip any whitespace */
    while( *ptr == ' ' ) ptr++; message = ptr;

    /* Find end of word */
    while( *ptr && *ptr != ' ' ) ptr++;

    /* message now points to a word of length (ptr-message); if
       that's longer than an entire line (most likely filenames), just
       take the last bit */
    if( ptr - message > line_length ) message = ptr - line_length;

    /* Check we've got room for the word, plus the prefixing space */
    if( position + ( ptr - message + 1 ) > line_length ) {
      char **new_lines; int i;

      /* If we've filled the screen, stop */
      if( *count == 20 ) return 0;

      new_lines = (char**)realloc( (*lines), (*count + 1) * sizeof( char** ) );
      if( new_lines == NULL ) {
	for( i=0; i<*count; i++ ) free( (*lines)[i] );
	if(*lines) free( (*lines) );
	return 1;
      }
      (*lines) = new_lines;

      (*lines)[*count] = (char*)malloc( (line_length+1) * sizeof(char) );
      if( (*lines)[*count] == NULL ) {
	for( i=0; i<*count; i++ ) free( (*lines)[i] );
	free( (*lines) );
	return 1;
      }
      
      strncpy( (*lines)[*count], message, ptr - message );
      position = ptr - message;
      (*lines)[*count][position] = '\0';

      (*count)++;

    } else {		/* Enough room on this line */

      strcat( (*lines)[*count-1], " " );
      strncat( (*lines)[*count-1], message, ptr - message );
      position += ptr - message + 1;
      (*lines)[*count-1][position] = '\0';

    }

    message = ptr;
    
  }

  return 0;
}

void widget_error_keyhandler( keyboard_key_name key )
{
  switch( key ) {
    
  case KEYBOARD_1: /* 1 used as `Escape' generates `Edit', which is Caps + 1 */
    widget_return[ widget_level ].finished = WIDGET_FINISHED_CANCEL;
    return;

  case KEYBOARD_Enter:
    widget_return[ widget_level ].finished = WIDGET_FINISHED_OK;
    return;

  default:	/* Keep gcc happy */
    break;

  }
}



