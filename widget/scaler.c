/* scaler.c: scaler selection widget
   Copyright (c) 2001-2004 Philip Kendall

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

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libspectrum.h>

#include "display.h"
#include "fuse.h"
#include "settings.h"
#include "ui/scaler/scaler.h"
#include "ui/uidisplay.h"
#include "widget_internals.h"

/* Data for drawing the cursor */
static int highlight_line;
static char descriptions[SCALER_NUM][40];

/* The available scalers */
static ui_scaler_available available;
static int scaler_count;
static int scaler[SCALER_NUM];

/* Scaler type we're going to switch to */
scaler_type widget_scaler;

int
widget_scaler_draw( void *data )
{
  scaler_type i; int j;

  /* Store our scaler availiablity function */
  if( data ) available = data;

  scaler_count = 0;
  for( i = 0; i < SCALER_NUM; i++ ) {
    if( available( i ) ) {

      scaler[ scaler_count ] = i;

      snprintf( descriptions[ scaler_count ], 40, "(%c): %s",
		((char)scaler_count)+'A', scaler_name( i ) );
      descriptions[ scaler_count ][ 28 ] = '\0';

      scaler_count++;
    }
    
  }

  /* Blank the main display area */
  widget_dialog_with_border( 1, 2, 30, scaler_count + 2 );

  widget_printstring( 8, 2, WIDGET_COLOUR_FOREGROUND, "Select Scaler" );

  j = 0;

  for( i=0; i<SCALER_NUM; i++ ) {
    if( !available( i ) ) continue;

    if( current_scaler == i ) {
      highlight_line = j;
      widget_rectangle( 2*8, (j+4)*8, 28*8, 1*8, WIDGET_COLOUR_FOREGROUND );
      widget_printstring( 2, j+4, WIDGET_COLOUR_BACKGROUND, descriptions[j] );
    } else {
      widget_printstring( 2, j+4, WIDGET_COLOUR_FOREGROUND, descriptions[j] );
    }

    j++;
  }

  widget_display_lines( 2, scaler_count + 2 );

  return 0;
}

void
widget_scaler_keyhandler( input_key key )
{
  switch( key ) {

#if 0
  case INPUT_KEY_Resize:	/* Fake keypress used on window resize */
    widget_scaler_draw( NULL );
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
      key - INPUT_KEY_a < (ptrdiff_t)scaler_count ) {
    
    /* Remove the old highlight */
    widget_rectangle( 2*8, (highlight_line+4)*8, 28*8, 1*8,
		      WIDGET_COLOUR_BACKGROUND );
    widget_printstring( 2, highlight_line+4, WIDGET_COLOUR_FOREGROUND,
			descriptions[ highlight_line ] );

    /*  draw the new one */
    highlight_line = key - INPUT_KEY_a;

    widget_rectangle( 2*8, (highlight_line+4)*8, 28*8, 1*8,
		      WIDGET_COLOUR_FOREGROUND );
    widget_printstring( 2, highlight_line+4, WIDGET_COLOUR_BACKGROUND,
			descriptions[ highlight_line ] );

    widget_display_lines( 2, scaler_count + 2 );

    /* And set this as the new scaler */
    widget_scaler = scaler[highlight_line];
  }
}

int widget_scaler_finish( widget_finish_state finished )
{
  if( finished == WIDGET_FINISHED_OK ) {
    widget_end_all( WIDGET_FINISHED_OK );
  } else {
    widget_scaler = SCALER_NUM;
  }

  return 0;
}

#endif				/* #ifdef USE_WIDGET */
