/* picture.c: Keyboard picture
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

#include "display.h"
#include "keyboard.h"
#include "ui/uidisplay.h"
#include "widget_internals.h"

static widget_picture_data *ptr;

int widget_picture_draw( void* data )
{
  ptr = (widget_picture_data*)data;

  uidisplay_spectrum_screen( ptr->screen, ptr->border );
  uidisplay_frame_end();

  return 0;
}

void
widget_picture_keyhandler( keyboard_key_name key, keyboard_key_name key2 )
{
  switch( key ) {

  case KEYBOARD_Resize:		/* Fake keypress used on widget resize */
    widget_picture_draw( ptr );
    break;
    
  case KEYBOARD_1: /* 1 used as `Escape' generates `Edit', which is Caps + 1 */
    if( key2 == KEYBOARD_Caps ) widget_end_widget( WIDGET_FINISHED_CANCEL );
    break;

  case KEYBOARD_Enter:
    widget_end_all( WIDGET_FINISHED_OK );
    break;

  default:	/* Keep gcc happy */
    break;

  }
}
