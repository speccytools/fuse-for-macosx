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

#include "display.h"
#include "fuse.h"
#include "ui/ui.h"

#define MESSAGE_MAX_LENGTH 256

int
ui_verror( ui_error_level severity, const char *format, va_list ap )
{
  GtkWidget *dialog, *ok_button, *label, *vbox;
  GtkAccelGroup *accel_group;
  char message[ MESSAGE_MAX_LENGTH + 1 ];

  /* Create the message to be displayed */
  vsnprintf( message, MESSAGE_MAX_LENGTH, format, ap );

  /* If this is a 'severe' error, print it to stderr with a program
     identifier and a level indicator */
  if( severity > UI_ERROR_INFO ) {

    fprintf( stderr, "%s: ", fuse_progname );

    switch( severity ) {
    case UI_ERROR_WARNING: fprintf( stderr, "warning: " ); break;
    case UI_ERROR_ERROR: fprintf( stderr, "error: " ); break;
    default: fprintf( stderr, "(unknown error level %d): ", severity ); break;
    }

    fprintf( stderr, "%s\n", message );

  }

  /* If we don't have a UI yet, we can't output widgets */
  if( !display_ui_initialised ) return 0;

  /* Create the dialog box */
  dialog = gtk_dialog_new();

  /* Set the appropriate title */
  switch( severity ) {

  case UI_ERROR_INFO:
    gtk_window_set_title( GTK_WINDOW( dialog ), "Fuse - Info" ); break;
  case UI_ERROR_WARNING:
    gtk_window_set_title( GTK_WINDOW( dialog ), "Fuse - Warning" ); break;
  case UI_ERROR_ERROR:
    gtk_window_set_title( GTK_WINDOW( dialog ), "Fuse - Error" ); break;
  default:
    gtk_window_set_title( GTK_WINDOW( dialog ),
			  "Fuse - (Unknown error level)" );
    break;

  }
  
  /* Add the OK button into the lower half */
  ok_button = gtk_button_new_with_label( "OK" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     ok_button );

  /* Create a label with that message */
  label = gtk_label_new( message );

  /* Make a new vbox for the top part for saner spacing */
  vbox=gtk_vbox_new( FALSE, 0 );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ),
                      vbox, TRUE, TRUE, 0 );
  gtk_container_set_border_width( GTK_CONTAINER( vbox ), 5 );
  gtk_container_set_border_width( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
                                  5 );

  /* Put the label in it */
  gtk_container_add( GTK_CONTAINER( vbox ), label );

  /* Add some ways to finish the dialog box */
  gtk_signal_connect_object( GTK_OBJECT( ok_button ), "clicked",
			     GTK_SIGNAL_FUNC( gtk_widget_destroy ),
			     GTK_OBJECT( dialog ) );
  gtk_signal_connect( GTK_OBJECT( dialog ), "delete-event",
		      GTK_SIGNAL_FUNC( gtk_widget_destroy ), (gpointer) NULL );

  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(dialog), accel_group);

  gtk_widget_add_accelerator( ok_button, "clicked",
			      accel_group,
			      GDK_Return, 0, 0 );
  gtk_widget_add_accelerator( ok_button, "clicked",
			      accel_group,
			      GDK_Escape, 0, 0 );

  gtk_widget_show_all( dialog );

  return 0;
}
  
#endif			/* #ifdef UI_GTK */
