/* confirm.c: Confirmation dialog box
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

#include <config.h>

#ifdef UI_GTK

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "fuse.h"
#include "gtkui.h"

struct confirm_data {

  GtkWidget *dialog;
  int confirmed;

};

static void set_confirmed( GtkButton *button, gpointer user_data );

int
gtkui_confirm( const char *string )
{
  struct confirm_data data;

  GtkWidget *label, *button;
  GtkAccelGroup *accelerators;

  fuse_emulation_pause();

  data.confirmed = 0;

  data.dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( data.dialog ), "Fuse - Confirm" );
  gtk_signal_connect( GTK_OBJECT( data.dialog ), "delete_event",
		      GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ), NULL );

  accelerators = gtk_accel_group_new();
  gtk_window_add_accel_group( GTK_WINDOW( data.dialog ), accelerators );

  label = gtk_label_new( string );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( data.dialog )->vbox ),
		      label, TRUE, TRUE, 5 );

  button = gtk_button_new_with_label( "OK" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( data.dialog )->action_area ),
		     button );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked",
		      GTK_SIGNAL_FUNC( set_confirmed ), &data );
  gtk_widget_add_accelerator( button, "clicked", accelerators, GDK_Return, 0,
			      0);

  button = gtk_button_new_with_label( "Cancel" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( data.dialog )->action_area ),
		     button );
  gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
			     GTK_OBJECT( data.dialog ) );
  gtk_widget_add_accelerator( button, "clicked", accelerators, GDK_Escape, 0,
			      0);

  gtk_window_set_modal( GTK_WINDOW( data.dialog ), TRUE );
  gtk_widget_show_all( data.dialog );

  gtk_main();

  fuse_emulation_unpause();

  return data.confirmed;
}

static void
set_confirmed( GtkButton *button, gpointer user_data )
{
  struct confirm_data *data = user_data;

  data->confirmed = 1;

  gtkui_destroy_widget_and_quit( data->dialog, NULL );
}

#endif				/* #ifdef UI_GTK */
