/* error.c: GTK+ routines for producing the error dialog box
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

#ifdef UI_GTK		/* Use this file iff we're using GTK+ */

#include <stdarg.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "fuse.h"

#define MESSAGE_MAX_LENGTH 256

int ui_error( const char *format, ... )
{
  va_list ap;
  GtkWidget *dialog, *ok_button, *label;
  char message[ MESSAGE_MAX_LENGTH + 1 ];

  /* Create the message from the given arguments */
  va_start( ap, format );
  vsnprintf( message, MESSAGE_MAX_LENGTH, format, ap );
  va_end( ap );

  /* Print the message to stderr, along with a program identifier */
  fprintf( stderr, "%s: %s", fuse_progname, message );

  /* Create the dialog box */
  dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( dialog ), "Fuse - Error" );
  
  /* Add the OK button into the lower half */
  ok_button = gtk_button_new_with_label( "OK" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     ok_button );

  /* Create a label with that message */
  label = gtk_label_new( message );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->vbox ), label );

  /* Add some ways to finish the dialog box */
  gtk_signal_connect_object( GTK_OBJECT( ok_button ), "clicked",
			     GTK_SIGNAL_FUNC( gtk_widget_destroy ),
			     GTK_OBJECT( dialog ) );
  gtk_signal_connect( GTK_OBJECT( dialog ), "delete_event",
		      GTK_SIGNAL_FUNC( gtk_widget_destroy ), (gpointer) NULL );
  gtk_widget_add_accelerator( ok_button, "clicked",
			      gtk_accel_group_get_default(),
			      GDK_Return, 0, 0 );
  gtk_widget_add_accelerator( ok_button, "clicked",
			      gtk_accel_group_get_default(),
			      GDK_Escape, 0, 0 );

  gtk_widget_show_all( dialog );

  return 0;
}
  
#endif			/* #ifdef UI_GTK */
