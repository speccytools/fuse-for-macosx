/* query.c: The query widgets
   Copyright (c) 2004 Darren Salt

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: linux@youmustbejoking.demon.co.uk

*/

#include <config.h>

#ifdef USE_WIDGET

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "fuse.h"
#include "widget_internals.h"

widget_query_t widget_query;

static const char *title = "Fuse - Confirm";

static int
internal_query_draw( int save, const char *data )
{
  char **lines;
  size_t i, count;

  if( split_message( data, &lines, &count, 28 ) )
    return 1;

  widget_dialog_with_border( 1, 2, 30, count + 3 );
  widget_print_title( 16, WIDGET_COLOUR_FOREGROUND, title );
  for( i = 0; i < count; ++i ) {
    widget_printstring( 17, i*8+28, WIDGET_COLOUR_FOREGROUND, lines[i] );
    free( lines[i] );
  }
  free( lines );
  
  widget_printstring_right(
    240, i*8+28, 5, save ? "\012S\011ave  \012D\011on't save  \012C\011ancel"
                         : "\012Y\011es  \012N\011o"
  );

  widget_display_lines( 2, count + 5 );

  return 0;
}

int
widget_query_draw( void *data )
{
  return internal_query_draw( 0, (const char *) data );
}

int
widget_query_save_draw( void *data )
{
  return internal_query_draw( 1, (const char *) data );
}

void
widget_query_keyhandler( input_key key )
{
  switch( key ) {
  case INPUT_KEY_Return:
  case INPUT_KEY_y:
    widget_query.confirm = 1;
    widget_end_widget( WIDGET_FINISHED_OK );
    break;
  case INPUT_KEY_Escape:
  case INPUT_KEY_n:
    widget_query.confirm = 0;
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    break;
  default:;
  }
}

void
widget_query_save_keyhandler( input_key key )
{
  switch( key ) {
  case INPUT_KEY_Return:
  case INPUT_KEY_s:
    widget_query.save = UI_CONFIRM_SAVE_SAVE;
    widget_end_widget( WIDGET_FINISHED_OK );
    break;
  case INPUT_KEY_Escape:
  case INPUT_KEY_c:
    widget_query.save = UI_CONFIRM_SAVE_CANCEL;
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    break;
  case INPUT_KEY_d:
    widget_query.save = UI_CONFIRM_SAVE_DONTSAVE;
    widget_end_widget( WIDGET_FINISHED_OK );
    break;
  default:;
  }
}

#endif				/* #ifdef USE_WIDGET */
