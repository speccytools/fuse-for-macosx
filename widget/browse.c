/* browse.c: tape browser widget
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
#include <stdlib.h>

#include "display.h"
#include "fuse.h"
#include "keyboard.h"
#include "tape.h"
#include "ui/uidisplay.h"
#include "widget.h"

/* The descriptions of the blocks */
static char ***block_descriptions;
static size_t blocks;

/* Which block is shown on the top line of the widget */
static int top_line;

/* Which block is currently highlighted */
static int highlight;

static void show_blocks( void );

int
widget_browse_draw( void *data GCC_UNUSED )
{
  int error;

  error = tape_get_block_list( &block_descriptions, &blocks );
  if( error ) return 1;

  widget_dialog_with_border( 1, 2, 30, 20 );

  widget_printstring( 10, 2, WIDGET_COLOUR_FOREGROUND, "Browse Tape" );
  uidisplay_lines( DISPLAY_BORDER_HEIGHT + 16, DISPLAY_BORDER_HEIGHT + 23 );

  highlight = tape_get_current_block();
  top_line = highlight - 8; if( top_line < 0 ) top_line = 0;

  show_blocks();

  return 0;
}

static void
show_blocks( void )
{
  int i; char buffer[30];

  widget_rectangle( 2*8, 4*8, 28*8, 18*8, WIDGET_COLOUR_BACKGROUND );

  for( i=0; i<18 && top_line+i<blocks; i++ ) {
    snprintf( buffer, 29, "%2lu: %s", (unsigned long)(top_line+i+1),
	      block_descriptions[top_line+i][0] );
    if( top_line+i == highlight ) {
      widget_rectangle( 2*8, (i+4)*8, 28*8, 1*8, WIDGET_COLOUR_FOREGROUND );
      widget_printstring( 2, i+4, WIDGET_COLOUR_BACKGROUND, buffer );
    } else {
      widget_printstring( 2, i+4, WIDGET_COLOUR_FOREGROUND, buffer );
    }
  }

  uidisplay_lines( DISPLAY_BORDER_HEIGHT + 32,
		   DISPLAY_BORDER_HEIGHT + 32 + 18 * 8 );
}

void
widget_browse_keyhandler( keyboard_key_name key )
{
  switch( key ) {

  case KEYBOARD_Resize:		/* Fake keypress used on window resize */
    widget_browse_draw( NULL );
    break;

  case KEYBOARD_1: /* 1 used as `Escape' generates `Edit', which is Caps + 1 */
    widget_return[ widget_level ].finished = WIDGET_FINISHED_CANCEL;
    return;

  case KEYBOARD_6:
  case KEYBOARD_j:
    if( highlight < blocks - 1 ) {
      highlight++;
      if( highlight >= top_line + 18 ) top_line += 18;
      show_blocks();
    }
    break;
    
  case KEYBOARD_7:
  case KEYBOARD_k:
    if( highlight > 0 ) { 
      highlight--;
      if( highlight < top_line )
	{
	  top_line -= 18;
	  if( top_line < 0 ) top_line = 0;
	}
      show_blocks();
    }
    break;

  case KEYBOARD_PageUp:
    highlight -= 18; if( highlight < 0 ) highlight = 0;
    top_line  -= 18; if( top_line  < 0 ) top_line = 0;
    show_blocks();
    break;

  case KEYBOARD_PageDown:
    highlight += 18; if( highlight >= blocks ) highlight = blocks - 1;
    top_line  += 18;
    if( top_line  >= blocks ) {
      top_line = blocks - 18;
      if( top_line < 0 ) top_line = 0;
    }
    show_blocks();
    break;

  case KEYBOARD_Home:
    highlight = top_line = 0;
    show_blocks();
    break;

  case KEYBOARD_End:
    highlight = blocks - 1;
    top_line = blocks - 18; if( top_line < 0 ) top_line = 0;
    show_blocks();
    break;

  case KEYBOARD_Enter:
    widget_return[ widget_level ].finished = WIDGET_FINISHED_OK;
    return;

  default:	/* Keep gcc happy */
    break;

  }
}

int
widget_browse_finish( widget_finish_state finished )
{
  tape_free_block_list( block_descriptions, blocks );

  if( finished == WIDGET_FINISHED_OK ) {
    tape_select_block( highlight );
    widget_end_all( WIDGET_FINISHED_OK );
  }
    
  return 0;
}
