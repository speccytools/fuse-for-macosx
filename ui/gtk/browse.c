/* browse.c: tape browser dialog box
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
#include "tape.h"

struct browse_data {

  GtkWidget *dialog;
  gint row;		/* The selected row; -1 => none */

};

static void select_row( GtkWidget *widget, gint row, gint column,
			GdkEventButton *event, gpointer data );
static void unselect_row( GtkWidget *widget, gint row, gint column,
			  GdkEventButton *event, gpointer data );
static void browse_done( GtkWidget *widget, gpointer data );

void
gtk_tape_browse( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  GtkWidget *dialog;
  GtkAccelGroup *accel_group;
  GtkWidget *scrolled_window, *clist;
  GtkWidget *ok_button, *cancel_button;

  struct browse_data callback_data;
  int current_block;

  gchar *titles[2] = { "Block type", "Data" };

  char ***text; size_t i,n;
  int error;

  error = tape_get_block_list( &text, &n );
  if( error ) return;

  /* Firstly, stop emulation */
  fuse_emulation_pause();

  /* Give me a new dialog box */
  dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( dialog ), "Fuse - Browse Tape" );

  /* And a scrolled window to pack the CList into */
  scrolled_window = gtk_scrolled_window_new( NULL, NULL );
  gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled_window ),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->vbox ),
		     scrolled_window );

  /* And the CList itself */
  clist = gtk_clist_new_with_titles( 2, titles );
  gtk_clist_column_titles_passive( GTK_CLIST( clist ) );
  for( i = 0; i < 2; i++ )
    gtk_clist_set_column_auto_resize( GTK_CLIST( clist ), i, TRUE );

  for( i=0; i<n; i++ ) {
    gtk_clist_append( GTK_CLIST( clist ), text[i] );
  }

  gtk_container_add( GTK_CONTAINER( scrolled_window ), clist );

  /* Create the OK and Cancel buttons */
  ok_button = gtk_button_new_with_label( "OK" );
  cancel_button = gtk_button_new_with_label( "Cancel" );

  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     ok_button );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     cancel_button );
  
  /* Add the necessary callbacks */
  callback_data.dialog = dialog;
  callback_data.row = -1;

  gtk_signal_connect( GTK_OBJECT( clist ), "select_row",
		      GTK_SIGNAL_FUNC( select_row ), &callback_data.row );
  gtk_signal_connect( GTK_OBJECT( clist ), "unselect_row",
		      GTK_SIGNAL_FUNC( unselect_row ), &callback_data.row );

  gtk_signal_connect( GTK_OBJECT( ok_button ), "clicked",
		      GTK_SIGNAL_FUNC( browse_done ),
		      (gpointer)(&callback_data) );
  gtk_signal_connect_object( GTK_OBJECT( cancel_button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
			     GTK_OBJECT( dialog ) );
  gtk_signal_connect( GTK_OBJECT( dialog ), "delete_event",
		      GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ), NULL );

  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(dialog), accel_group);

  /* Allow Esc to cancel */
  gtk_widget_add_accelerator( cancel_button, "clicked",
			      accel_group, GDK_Escape, 0, 0);

  /* Make the window big enough to show at least some data */
  gtk_window_set_default_size( GTK_WINDOW( dialog ), -1, 200 );

  current_block = tape_get_current_block();
  if( current_block != -1 ) {
    /* FIXME: Why doesn't this work? */
    gtk_clist_moveto( GTK_CLIST( clist ), current_block, -1,
		      0.5, 0.0 );

    gtk_clist_select_row( GTK_CLIST( clist ), current_block, 0 );
  }

  /* Set the window to be modal and display it */
  gtk_window_set_modal( GTK_WINDOW( dialog ), TRUE );
  gtk_widget_show_all( dialog );

  /* Process events until the window is done with */
  gtk_main();

  /* Free up the block list */
  tape_free_block_list( text, n );

  /* And then carry on with emulation again */
  fuse_emulation_unpause();

  return;
}

/* Called when a row is selected */
static void
select_row( GtkWidget *widget GCC_UNUSED, gint row, gint column GCC_UNUSED,
	    GdkEventButton *event GCC_UNUSED, gpointer data )
{
  *( (gint*)data ) = row;
}

/* Called when a row is unselected */
static void
unselect_row( GtkWidget *widget GCC_UNUSED, gint row GCC_UNUSED,
	      gint column GCC_UNUSED, GdkEventButton *event GCC_UNUSED,
	      gpointer data )
{
  *( (gint*)data ) = -1;
}

/* Called if the OK button is clicked */
static void
browse_done( GtkWidget *widget GCC_UNUSED, gpointer data )
{
  struct browse_data *callback_data = (struct browse_data*)data;

  /* Set the tape to the appropriate block */
  if( callback_data->row != -1 ) tape_select_block( callback_data->row );

  gtkui_destroy_widget_and_quit( callback_data->dialog, NULL );
}

#endif			/* #ifdef UI_GTK */
