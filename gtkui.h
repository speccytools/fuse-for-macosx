/* gtkui.h: GTK+ routines for dealing with the user interface
   Copyright (c) 2000-2001 Philip Kendall

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

   E-mail: pak@ast.cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_GTKUI_H
#define FUSE_GTKUI_H

#ifndef __GTK_H__
#include <gtk/gtk.h>
#endif

extern GtkWidget* gtkui_window;
extern GtkWidget* gtkui_drawing_area;

int gtkui_init(int *argc, char ***argv, int width, int height);
int gtkui_end(void);

#endif			/* #ifndef FUSE_GTKUI_H */
