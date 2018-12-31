/* picture.c: GTK+ routines to draw the keyboard picture
   Copyright (c) 2002-2005 Philip Kendall

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <gtk/gtk.h>

#include "gtkinternals.h"
#include "utils.h"

static int dialog_created = 0;

static GtkWidget *dialog;

int
gtkui_picture( const char *filename, int border )
{
  GtkWidget *image, *content_area;

  if( !dialog_created ) {

    char path[PATH_MAX];

    if( utils_find_file_path( filename, path, UTILS_AUXILIARY_LIB ) ) return 1;

    image = gtk_image_new_from_file( path );

    dialog = gtkstock_dialog_new( "Fuse - Keyboard",
				  G_CALLBACK( gtk_widget_hide ) );
  
    content_area = gtk_dialog_get_content_area( GTK_DIALOG( dialog ) );
    gtk_container_add( GTK_CONTAINER( content_area ), image );

    gtkstock_create_close( dialog, NULL, G_CALLBACK( gtk_widget_hide ),
			   FALSE );

    /* Stop users resizing this window */
    gtk_window_set_resizable( GTK_WINDOW( dialog ), FALSE );

    dialog_created = 1;
  }

  gtk_widget_show_all( dialog );

  return 0;
}
