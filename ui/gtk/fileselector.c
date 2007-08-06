/* fileselector.c: GTK+ fileselector routines
   Copyright (c) 2000-2007 Philip Kendall

   $Id$

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

/* TODO: reimplement "current directory" functionality */

char*
menu_get_open_filename( const char *title )
{
  GtkWidget *dialog;
  char *filename = NULL;

  dialog =
    gtk_file_chooser_dialog_new( title, GTK_WINDOW( gtkui_window ),
				 GTK_FILE_CHOOSER_ACTION_OPEN,
				 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				 NULL );

  if( gtk_dialog_run( GTK_DIALOG( dialog ) ) == GTK_RESPONSE_ACCEPT )
    filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ) );

  gtk_widget_destroy( dialog );

  return filename;
}

char*
menu_get_save_filename( const char *title )
{
  GtkWidget *dialog;
  char *filename = NULL;
  
  dialog =
    gtk_file_chooser_dialog_new( title, GTK_WINDOW( gtkui_window ),
				 GTK_FILE_CHOOSER_ACTION_SAVE,
				 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				 GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				 NULL );

  if( gtk_dialog_run( GTK_DIALOG( dialog ) ) == GTK_RESPONSE_ACCEPT )
    filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ) );

  gtk_widget_destroy( dialog );

  return filename;
}
