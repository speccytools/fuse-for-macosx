/* binary.c: GTK+ routines to load/save chunks of binary data
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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <stdio.h>

#include <gtk/gtk.h>

#include "fuse.h"
#include "gtkui.h"
#include "spectrum.h"
#include "types.h"
#include "ui/ui.h"
#include "utils.h"

#ifdef UI_GTK		/* Use this file iff we're using GTK+ */

struct binary_info {

  char *filename;
  utils_file file;

  GtkWidget *dialog;
  GtkWidget *filename_widget, *start_widget, *length_widget;
};

static void change_load_filename( GtkButton *button, gpointer user_data );
static void load_data( GtkButton *button, gpointer user_data );

static void change_save_filename( GtkButton *button, gpointer user_data );
static void save_data( GtkButton *button, gpointer user_data );

void
gtkui_load_binary_data( GtkWidget *widget GCC_UNUSED, gpointer data
			GCC_UNUSED )
{
  struct binary_info info;

  GtkWidget *table, *label, *button;

  char buffer[80];
  int error;

  fuse_emulation_pause();

  info.filename = gtkui_fileselector_get_filename( "Fuse - Load Binary Data" );
  if( !info.filename ) { fuse_emulation_unpause(); return; }

  error = utils_read_file( info.filename, &info.file );
  if( error ) { free( info.filename ); fuse_emulation_unpause(); return; }

  info.dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( info.dialog ), "Fuse - Load Binary Data" );
  gtk_signal_connect( GTK_OBJECT( info.dialog ), "delete_event",
		      GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ), NULL );

  /* Information display */

  table = gtk_table_new( 3, 3, FALSE );
  gtk_box_pack_start_defaults( GTK_BOX( GTK_DIALOG( info.dialog )->vbox ),
			       table );

  label = gtk_label_new( "Filename" );
  gtk_table_attach_defaults( GTK_TABLE( table ), label, 0, 1, 0, 1 );

  info.filename_widget = gtk_label_new( info.filename );
  gtk_table_attach_defaults( GTK_TABLE( table ), info.filename_widget, 1, 2,
			     0, 1 );

  button = gtk_button_new_with_label( "Browse..." );
  gtk_table_attach_defaults( GTK_TABLE( table ), button, 2, 3, 0, 1 );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked",
		      GTK_SIGNAL_FUNC( change_load_filename ), &info );

  label = gtk_label_new( "Start" );
  gtk_table_attach_defaults( GTK_TABLE( table ), label, 0, 1, 1, 2 );

  info.start_widget = gtk_entry_new();
  gtk_table_attach_defaults( GTK_TABLE( table ), info.start_widget, 1, 3,
			     1, 2 );
  gtk_signal_connect( GTK_OBJECT( info.start_widget ), "activate",
		      GTK_SIGNAL_FUNC( load_data ), &info );

  label = gtk_label_new( "Length" );
  gtk_table_attach_defaults( GTK_TABLE( table ), label, 0, 1, 2, 3 );

  snprintf( buffer, 80, "%lu", (unsigned long)info.file.length );
  info.length_widget = gtk_entry_new();
  gtk_entry_set_text( GTK_ENTRY( info.length_widget ), buffer );
  gtk_table_attach_defaults( GTK_TABLE( table ), info.length_widget, 1, 3,
			     2, 3 );
  gtk_signal_connect( GTK_OBJECT( info.length_widget ), "activate",
		      GTK_SIGNAL_FUNC( load_data ), &info );

  /* Command buttons */

  button = gtk_button_new_with_label( "OK" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( info.dialog )->action_area ),
		     button );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked",
		      GTK_SIGNAL_FUNC( load_data ), &info );

  button = gtk_button_new_with_label( "Cancel" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( info.dialog )->action_area ),
		     button );
  gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
			     GTK_OBJECT( info.dialog ) );

  /* Process the dialog */

  gtk_window_set_modal( GTK_WINDOW( info.dialog ), TRUE );
  gtk_widget_show_all( info.dialog );

  gtk_main();

  free( info.filename );
  utils_close_file( &info.file );

  fuse_emulation_unpause();
}

static void
change_load_filename( GtkButton *button, gpointer user_data )
{
  struct binary_info *info = user_data;
  
  char *new_filename;
  utils_file new_file;

  char buffer[80];
  int error;

  new_filename = gtkui_fileselector_get_filename( "Fuse - Load Binary Data" );
  if( !new_filename ) return;

  error = utils_read_file( new_filename, &new_file );
  if( error ) { free( new_filename ); return; }

  /* Remove the data for the old file */
  error = utils_close_file( &info->file );
  if( error ) { free( new_filename ); return; }

  free( info->filename );

  /* Put the new data in */
  info->filename = new_filename; info->file = new_file;

  /* And update the displayed information */
  gtk_label_set_text( GTK_LABEL( info->filename_widget ), new_filename );
  
  snprintf( buffer, 80, "%lu", (unsigned long)info->file.length );
  gtk_entry_set_text( GTK_ENTRY( info->length_widget ), buffer );
}

static void
load_data( GtkButton *button, gpointer user_data )
{
  struct binary_info *info = user_data;

  WORD start, length; size_t i;

  length = atoi( gtk_entry_get_text( GTK_ENTRY( info->length_widget ) ) );

  if( length > info->file.length ) {
    ui_error( UI_ERROR_ERROR,
	      "'%s' contains only %lu bytes",
	      info->filename, (unsigned long)info->file.length );
    return;
  }

  start = atoi( gtk_entry_get_text( GTK_ENTRY( info->start_widget ) ) );

  for( i = 0; i < length; i++ )
    writebyte( start + i, info->file.buffer[ i ] );

  gtkui_destroy_widget_and_quit( info->dialog, NULL );
}
  
void
gtkui_save_binary_data( GtkWidget *widget GCC_UNUSED, gpointer data
			GCC_UNUSED )
{
  struct binary_info info;

  GtkWidget *table, *label, *button;

  fuse_emulation_pause();

  info.filename = gtkui_fileselector_get_filename( "Fuse - Save Binary Data" );
  if( !info.filename ) { fuse_emulation_unpause(); return; }

  info.dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( info.dialog ), "Fuse - Save Binary Data" );
  gtk_signal_connect( GTK_OBJECT( info.dialog ), "delete_event",
		      GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ), NULL );

  /* Information display */

  table = gtk_table_new( 3, 3, FALSE );
  gtk_box_pack_start_defaults( GTK_BOX( GTK_DIALOG( info.dialog )->vbox ),
			       table );

  label = gtk_label_new( "Filename" );
  gtk_table_attach_defaults( GTK_TABLE( table ), label, 0, 1, 0, 1 );

  info.filename_widget = gtk_label_new( info.filename );
  gtk_table_attach_defaults( GTK_TABLE( table ), info.filename_widget, 1, 2,
			     0, 1 );

  button = gtk_button_new_with_label( "Browse..." );
  gtk_table_attach_defaults( GTK_TABLE( table ), button, 2, 3, 0, 1 );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked",
		      GTK_SIGNAL_FUNC( change_save_filename ), &info );

  label = gtk_label_new( "Start" );
  gtk_table_attach_defaults( GTK_TABLE( table ), label, 0, 1, 1, 2 );

  info.start_widget = gtk_entry_new();
  gtk_table_attach_defaults( GTK_TABLE( table ), info.start_widget, 1, 3,
			     1, 2 );
  gtk_signal_connect( GTK_OBJECT( info.start_widget ), "activate",
		      GTK_SIGNAL_FUNC( load_data ), &info );

  label = gtk_label_new( "Length" );
  gtk_table_attach_defaults( GTK_TABLE( table ), label, 0, 1, 2, 3 );

  info.length_widget = gtk_entry_new();
  gtk_table_attach_defaults( GTK_TABLE( table ), info.length_widget, 1, 3,
			     2, 3 );
  gtk_signal_connect( GTK_OBJECT( info.length_widget ), "activate",
		      GTK_SIGNAL_FUNC( load_data ), &info );

  /* Command buttons */

  button = gtk_button_new_with_label( "OK" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( info.dialog )->action_area ),
		     button );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked",
		      GTK_SIGNAL_FUNC( save_data ), &info );

  button = gtk_button_new_with_label( "Cancel" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( info.dialog )->action_area ),
		     button );
  gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
			     GTK_OBJECT( info.dialog ) );

  /* Process the dialog */

  gtk_window_set_modal( GTK_WINDOW( info.dialog ), TRUE );
  gtk_widget_show_all( info.dialog );

  gtk_main();

  free( info.filename );

  fuse_emulation_unpause();
}

static void
change_save_filename( GtkButton *button, gpointer user_data )
{
  struct binary_info *info = user_data;
  char *new_filename;

  new_filename = gtkui_fileselector_get_filename( "Fuse - Save Binary Data" );
  if( !new_filename ) return;

  free( info->filename );

  info->filename = new_filename;

  gtk_label_set_text( GTK_LABEL( info->filename_widget ), new_filename );
}

static void
save_data( GtkButton *button, gpointer user_data )
{
  struct binary_info *info = user_data;

  WORD start, length; size_t i;
  char *buffer;

  int error;

  length = atoi( gtk_entry_get_text( GTK_ENTRY( info->length_widget ) ) );

  buffer = malloc( length * sizeof( libspectrum_byte ) );
  if( !buffer ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return;
  }

  start = atoi( gtk_entry_get_text( GTK_ENTRY( info->start_widget ) ) );

  for( i = 0; i < length; i++ )
    buffer[ i ] = readbyte( start + i );

  error = utils_write_file( info->filename, buffer, length );
  if( error ) { free( buffer ); return; }

  free( buffer );

  gtkui_destroy_widget_and_quit( info->dialog, NULL );
}
  
#endif			/* #ifdef UI_GTK */