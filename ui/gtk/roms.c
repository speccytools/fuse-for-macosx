/* roms.c: ROM selector dialog box
   Copyright (c) 2003 Philip Kendall

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
   Foundation, Inc., 49 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#ifdef UI_GTK		/* Use this file iff we're using GTK+ */

#include <stdlib.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "fuse.h"
#include "gtkui.h"
#include "settings.h"

static void add_rom( GtkWidget *table, gint row, const char *name );
static void select_new_rom( GtkWidget *widget, gpointer data );
static void roms_done( GtkWidget *widget, gpointer data );

/* The labels used to display the current ROMs */
GtkWidget *rom[ SETTINGS_ROM_COUNT ];

void
gtkui_roms( GtkWidget *widget, gpointer data )
{
  GtkWidget *dialog;
  GtkAccelGroup *accel_group;
  GtkWidget *table;
  GtkWidget *ok_button, *cancel_button;

  size_t i;

  /* Firstly, stop emulation */
  fuse_emulation_pause();

  /* Give me a new dialog box */
  dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( dialog ), "Fuse - Select ROMs" );

  /* A table to put all the labels in */
  table = gtk_table_new( SETTINGS_ROM_COUNT, 3, FALSE );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->vbox ),
		     table );

  /* And the current values of each of the ROMs */
  for( i = 0; i < SETTINGS_ROM_COUNT; i++ )
    add_rom( table, i, settings_rom_name[i] );

  /* Create the OK and Cancel buttons */
  ok_button = gtk_button_new_with_label( "OK" );
  cancel_button = gtk_button_new_with_label( "Cancel" );

  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     ok_button );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     cancel_button );
  
  gtk_signal_connect_object( GTK_OBJECT( ok_button ), "clicked",
			     GTK_SIGNAL_FUNC( roms_done ),
			     GTK_OBJECT( dialog ) );
  gtk_signal_connect_object( GTK_OBJECT( cancel_button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
			     GTK_OBJECT( dialog ) );
  gtk_signal_connect( GTK_OBJECT( dialog ), "delete_event",
		      GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ), NULL );

  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group( GTK_WINDOW( dialog ), accel_group );

  /* Allow Esc to cancel */
  gtk_widget_add_accelerator( cancel_button, "clicked",
			      accel_group, GDK_Escape, 0, 0);

  /* Set the window to be modal and display it */
  gtk_window_set_modal( GTK_WINDOW( dialog ), TRUE );
  gtk_widget_show_all( dialog );

  /* Process events until the window is done with */
  gtk_main();

  /* And then carry on with emulation again */
  fuse_emulation_unpause();

  return;
}

static void
add_rom( GtkWidget *table, gint row, const char *name )
{
  GtkWidget *label, *change_button;

  label = gtk_label_new( name );
  gtk_table_attach_defaults( GTK_TABLE( table ), label, 0, 1, row, row + 1 );

  rom[ row ] =
    gtk_label_new( *( settings_get_rom_setting( &settings_current, row ) ) );
  gtk_table_attach_defaults( GTK_TABLE( table ), rom[ row ], 1, 2,
			     row, row + 1 );

  change_button = gtk_button_new_with_label( "Change" );
  gtk_signal_connect( GTK_OBJECT( change_button ), "clicked",
		      GTK_SIGNAL_FUNC( select_new_rom ),
		      rom[ row ] );
  gtk_table_attach_defaults( GTK_TABLE( table ), change_button, 2, 3,
			     row, row + 1 );
}

static void
select_new_rom( GtkWidget *widget, gpointer data )
{
  char *filename;

  GtkWidget *label = data;

  filename = gtkui_fileselector_get_filename( "Fuse - Select ROM" );
  if( !filename ) return;

  gtk_label_set( GTK_LABEL( label ), filename );
}

static void
roms_done( GtkWidget *widget, gpointer data )
{
  size_t i;
  int error;
  
  char *string;

  for( i = 0; i < SETTINGS_ROM_COUNT; i++ ) {
    gtk_label_get( GTK_LABEL( rom[i] ), &string );
    error =
      settings_set_string( settings_get_rom_setting( &settings_current, i ),
			   string );
    if( error ) return;
  }

  gtkui_destroy_widget_and_quit( widget, NULL );
}

#endif			/* #ifdef UI_GTK */
