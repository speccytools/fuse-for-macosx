/* confirm.c: Confirmation dialog box
   Copyright (c) 2000-2003 Philip Kendall, Russell Marks

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

#include "compat.h"
#include "fuse.h"
#include "gtkinternals.h"
#include "settings.h"
#include "ui/ui.h"

static void set_confirmed( GtkButton *button, gpointer user_data );
static void set_save( GtkButton *button, gpointer user_data );
static void set_dont_save( GtkButton *button, gpointer user_data );

int
gtkui_confirm( const char *string )
{
  GtkWidget *dialog, *label, *button;
  GtkAccelGroup *accelerators;
  int confirm;

  /* Return value isn't an error code, but signifies whether to undertake
     the action */
  if( !settings_current.confirm_actions ) return 1;

  fuse_emulation_pause();

  confirm = 0;

  dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( dialog ), "Fuse - Confirm" );
  gtk_signal_connect( GTK_OBJECT( dialog ), "delete-event",
		      GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ), NULL );

  accelerators = gtk_accel_group_new();
  gtk_window_add_accel_group( GTK_WINDOW( dialog ), accelerators );

  label = gtk_label_new( string );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ), label,
		      TRUE, TRUE, 5 );

  button = gtk_button_new_with_label( "OK" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     button );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked",
		      GTK_SIGNAL_FUNC( set_confirmed ), &confirm );
  gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
			     GTK_OBJECT( dialog ) );
  gtk_widget_add_accelerator( button, "clicked", accelerators, GDK_Return, 0,
			      0);

  button = gtk_button_new_with_label( "Cancel" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     button );
  gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
			     GTK_OBJECT( dialog ) );
  gtk_widget_add_accelerator( button, "clicked", accelerators, GDK_Escape, 0,
			      0);

  gtk_window_set_modal( GTK_WINDOW( dialog ), TRUE );
  gtk_widget_show_all( dialog );

  gtk_main();

  fuse_emulation_unpause();

  return confirm;
}

static void
set_confirmed( GtkButton *button GCC_UNUSED, gpointer user_data )
{
  int *ptr = user_data;

  *ptr = 1;
}

ui_confirm_save_t
ui_confirm_save( const char *message )
{
  GtkWidget *dialog, *label, *button;
  GtkAccelGroup *accelerators;
  ui_confirm_save_t confirm;

  if( !settings_current.confirm_actions ) return UI_CONFIRM_SAVE_DONTSAVE;

  fuse_emulation_pause();

  confirm = UI_CONFIRM_SAVE_CANCEL;

  dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( dialog ), "Fuse - Confirm" );
  gtk_signal_connect( GTK_OBJECT( dialog ), "delete-event",
		      GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ), NULL );

  accelerators = gtk_accel_group_new();
  gtk_window_add_accel_group( GTK_WINDOW( dialog ), accelerators );

  label = gtk_label_new( message );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ), label,
		      TRUE, TRUE, 5 );

  button = gtk_button_new_with_label( "Save" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     button );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked",
		      GTK_SIGNAL_FUNC( set_save ), &confirm );
  gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
			     GTK_OBJECT( dialog ) );

  button = gtk_button_new_with_label( "Don't Save" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     button );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked",
		      GTK_SIGNAL_FUNC( set_dont_save ), &confirm );
  gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
			     GTK_OBJECT( dialog ) );

  button = gtk_button_new_with_label( "Cancel" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     button );
  gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
			     GTK_OBJECT( dialog ) );
  gtk_widget_add_accelerator( button, "clicked", accelerators, GDK_Escape, 0,
			      0);

  gtk_window_set_modal( GTK_WINDOW( dialog ), TRUE );
  gtk_widget_show_all( dialog );

  gtk_main();

  fuse_emulation_unpause();

  return confirm;
}

static void
set_save( GtkButton *button GCC_UNUSED, gpointer user_data )
{
  ui_confirm_save_t *ptr = user_data;

  *ptr = UI_CONFIRM_SAVE_SAVE;
}

static void
set_dont_save( GtkButton *button GCC_UNUSED, gpointer user_data )
{
  ui_confirm_save_t *ptr = user_data;

  *ptr = UI_CONFIRM_SAVE_DONTSAVE;
}

#endif				/* #ifdef UI_GTK */
