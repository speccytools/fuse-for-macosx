/* machine_widget.c: Machine control widget
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
#include "machine.h"
#include "ui/uidisplay.h"
#include "widget.h"

static int widget_machine_show_machine( void );

int widget_machine_draw( void* data )
{
  /* Blank the main display area */
  widget_dialog_with_border( 1, 2, 30, 6 );

  widget_printstring( 9, 2, WIDGET_COLOUR_FOREGROUND, "Machine" );

  widget_printstring( 2, 4, WIDGET_COLOUR_FOREGROUND, "(R)eset machine" );
  widget_printstring( 2, 6, WIDGET_COLOUR_FOREGROUND, "(S)witch machines" );

  widget_machine_show_machine();

  uidisplay_lines( DISPLAY_BORDER_HEIGHT + 16,
		   DISPLAY_BORDER_HEIGHT + 16 + 64 );

  return 0;
}

static int widget_machine_show_machine( void )
{
  char buffer[29];

  widget_rectangle( 2*8, 7*8, 28*8, 1*8, WIDGET_COLOUR_BACKGROUND );

  snprintf( buffer, 28, "Current: %s", machine_current->description );
  buffer[28]='\0';

  widget_printstring( 2, 7, WIDGET_COLOUR_FOREGROUND, buffer );

  uidisplay_lines( DISPLAY_BORDER_HEIGHT + 56 ,
		   DISPLAY_BORDER_HEIGHT + 64 );

  return 0;
}

void widget_machine_keyhandler( int key )
{
  switch( key ) {
    
  case KEYBOARD_1: /* 1 used as `Escape' generates `Edit', which is Caps + 1 */
    widget_return[ widget_level ].finished = WIDGET_FINISHED_CANCEL;
    break;

  case KEYBOARD_r:
    machine_current->reset();
    break;

  case KEYBOARD_s:
    machine_select_next();
    widget_machine_show_machine();
    break;

  case KEYBOARD_Enter:
    widget_return[ widget_level ].finished = WIDGET_FINISHED_OK;
    break;

  }
}
