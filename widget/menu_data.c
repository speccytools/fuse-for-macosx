/* menu_data.c: Data for the widget menus
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

#include <stdlib.h>

#include "widget.h"

widget_menu_entry widget_menu_main[] = {
  { "Main menu", 0, 0, NULL },		/* Menu title */

  { "(F)ile",    KEYBOARD_f, WIDGET_TYPE_FILE,     NULL                 },
  { "(O)ptions", KEYBOARD_o, WIDGET_TYPE_MENU,     &widget_menu_options },
  { "(M)achine", KEYBOARD_m, WIDGET_TYPE_MACHINE,  NULL                 },
  { "(T)ape",	 KEYBOARD_t, WIDGET_TYPE_TAPE,     NULL                 },
  { "(H)elp",    KEYBOARD_h, WIDGET_TYPE_HELP,     NULL                 },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};

widget_menu_entry widget_menu_options[] = {
  { "Options", 0, 0, NULL },		/* Menu title */

  { "(G)eneral", KEYBOARD_g, WIDGET_TYPE_GENERAL, NULL                  },

  { NULL, 0, 0, NULL }			/* End marker: DO NOT REMOVE */
};
