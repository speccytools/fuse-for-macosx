/* select.c: machine selection widget
   Copyright (c) 2001,2002 Philip Kendall

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
#include "machine.h"
#include "ui/uidisplay.h"
#include "widget.h"

/* Data for drawing the cursor */
static int highlight_line;
static char descriptions[10][40];

/* Machine type we're going to switch to */
int new_machine;

int widget_select_draw( void* data )
{
  int i;

  /* Blank the main display area */
  widget_dialog_with_border( 1, 2, 30, machine_count+2 );

  widget_printstring( 8, 2, WIDGET_COLOUR_FOREGROUND, "Select Machine" );

  for( i=0; i<machine_count; i++ ) {
    snprintf( descriptions[i], 40, "(%c): %s",
	      ((char)i)+'A', machine_types[i]->description );
    descriptions[i][ 28 ] = '\0';
    
    if( machine_current->machine == machine_types[i]->machine ) {
      highlight_line = i;
      widget_rectangle( 2*8, (i+4)*8, 28*8, 1*8, WIDGET_COLOUR_FOREGROUND );
      widget_printstring( 2, i+4, WIDGET_COLOUR_BACKGROUND, descriptions[i] );
    } else {
      widget_printstring( 2, i+4, WIDGET_COLOUR_FOREGROUND, descriptions[i] );
    }
  }

  uidisplay_lines( DISPLAY_BORDER_HEIGHT + 16,
		   DISPLAY_BORDER_HEIGHT + 16 + (machine_count+2)*8 );

  return 0;
}

void widget_select_keyhandler( keyboard_key_name key )
{
  switch( key ) {

  case KEYBOARD_1: /* 1 used as `Escape' generates `Edit', which is Caps + 1 */
    widget_return[ widget_level ].finished = WIDGET_FINISHED_CANCEL;
    return;

  case KEYBOARD_Enter:
    widget_return[ widget_level ].finished = WIDGET_FINISHED_OK;
    return;

  }

  if( key >= KEYBOARD_a && key <= KEYBOARD_z &&
      key - KEYBOARD_a < machine_count ) {
    
    /* Remove the old highlight */
    widget_rectangle( 2*8, (highlight_line+4)*8, 28*8, 1*8,
		      WIDGET_COLOUR_BACKGROUND );
    widget_printstring( 2, highlight_line+4, WIDGET_COLOUR_FOREGROUND,
			descriptions[ highlight_line ] );

    /*  draw the new one */
    highlight_line = key - KEYBOARD_a;

    widget_rectangle( 2*8, (highlight_line+4)*8, 28*8, 1*8,
		      WIDGET_COLOUR_FOREGROUND );
    widget_printstring( 2, highlight_line+4, WIDGET_COLOUR_BACKGROUND,
			descriptions[ highlight_line ] );

    uidisplay_lines( DISPLAY_BORDER_HEIGHT + 16,
		     DISPLAY_BORDER_HEIGHT + 16 + (machine_count+2)*8 );

    /* And set this as the new machine type */
    new_machine = machine_types[highlight_line]->machine;
  }
}

int widget_select_finish( widget_finish_state finished )
{
  /* If we exited normally and we're not on the same machine type as
     we started on, force a reset */
  if( finished == WIDGET_FINISHED_OK &&
      machine_current->machine != new_machine )
    machine_select( new_machine );

  if( finished == WIDGET_FINISHED_OK ) widget_end_all( WIDGET_FINISHED_OK );

  return 0;
}

