/* gtkui.h: GTK+ routines for dealing with the user interface
   Copyright (c) 2000-2002 Philip Kendall, Russell Marks

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

#ifndef FUSE_GTKUI_H
#define FUSE_GTKUI_H

#ifndef __GTK_H__
#include <gtk/gtk.h>
#endif

#ifndef FUSE_GTKOPTIONS_H
#include "options.h"
#endif

extern GtkWidget* gtkui_window;
extern GtkWidget* gtkui_drawing_area;

extern GdkGC *gtkdisplay_gc;
extern unsigned long gtkdisplay_colours[];

void gtkui_destroy_widget_and_quit( GtkWidget *widget, gpointer data );
void gtk_tape_browse( GtkWidget *widget, gpointer data );

int gtkui_picture( const char *filename, int border );

extern void gtkui_popup_menu(void);

#endif			/* #ifndef FUSE_GTKUI_H */
