/* menu.c: general menu widget
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
#include "widget.h"
#include "ui/uidisplay.h"

widget_menu_entry *menu;

int widget_menu_draw( void *data )
{
  widget_menu_entry *ptr;
  size_t menu_entries; int i;

  menu = (widget_menu_entry*)data;

  /* How many menu items do we have? */
  for( ptr = &menu[1]; ptr->text; ptr++ ) ;
  menu_entries = ptr - &menu[1];

  widget_dialog_with_border( 1, 2, 30, menu_entries + 2 );

  widget_printstring( 15 - strlen( menu->text ) / 2, 2,
		      WIDGET_COLOUR_FOREGROUND, menu->text );

  for( i=0; i<menu_entries; i++ )
    widget_printstring( 2, i+4, WIDGET_COLOUR_FOREGROUND,
			menu[i+1].text );

  uidisplay_lines( DISPLAY_BORDER_HEIGHT + 16,
		   DISPLAY_BORDER_HEIGHT + 16 + (menu_entries+2)*8 );

  return 0;
}

void widget_menu_keyhandler( int key )
{
  widget_menu_entry *ptr;

  switch( key ) {
    
  case KEYBOARD_1: /* 1 used as `Escape' generates `Edit', which is Caps + 1 */
    widget_return[ widget_level ].finished = WIDGET_FINISHED_CANCEL;
    break;

  case KEYBOARD_Enter:
    widget_return[ widget_level ].finished = WIDGET_FINISHED_OK;
    break;

  }

  for( ptr=&menu[1]; ptr->text; ptr++ ) {
    if( key == ptr->key ) {
      widget_do( ptr->widget, ptr->widget_args );
      break;
    }
  }
}

