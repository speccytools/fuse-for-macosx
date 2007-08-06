/* fileselector.c: GTK+ fileselector routines
   Copyright (c) 2000-2003 Philip Kendall

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

#include <errno.h>
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif			/* #ifdef HAVE_LIBGEN_H */
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "gtkinternals.h"
#include "menu.h"
#include "ui/ui.h"

/* The last directory we visited */
static char *cwd = NULL;

static void fileselector_done( GtkButton *button, gpointer user_data );
static char *fileselector_get_filename( const char *title );

typedef struct fileselector_info {

  GtkWidget *selector;
  gchar *filename;

} fileselector_info;

static char*
fileselector_get_filename( const char *title )
{
  fileselector_info selector;
  GtkAccelGroup *accel_group;

  selector.selector = gtk_file_selection_new( title );
  selector.filename = NULL;

  gtk_signal_connect(
    GTK_OBJECT( GTK_FILE_SELECTION( selector.selector )->ok_button ),
    "clicked", GTK_SIGNAL_FUNC( fileselector_done ), &selector
  );

  gtk_signal_connect_object(
    GTK_OBJECT( GTK_FILE_SELECTION( selector.selector )->cancel_button ),
    "clicked", DEFAULT_DESTROY, GTK_OBJECT( selector.selector )
  );

  gtk_signal_connect( GTK_OBJECT( selector.selector ), "delete-event",
		      DEFAULT_DESTROY, NULL );

  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group( GTK_WINDOW( selector.selector ), accel_group );

  /* Allow Esc to cancel */
  gtk_widget_add_accelerator(
    GTK_FILE_SELECTION( selector.selector )->cancel_button, "clicked",
    accel_group, GDK_Escape, 0, 0
  );

  gtk_window_set_modal( GTK_WINDOW( selector.selector ), TRUE );

  if( cwd )
    gtk_file_selection_set_filename( GTK_FILE_SELECTION( selector.selector ),
				     cwd );

  gtk_widget_show( selector.selector );

  gtk_main();

  return selector.filename;
}

char*
menu_get_open_filename( const char *title )
{
  return fileselector_get_filename( title );
}

char*
menu_get_save_filename( const char *title )
{
  return fileselector_get_filename( title );
}

static void
fileselector_done( GtkButton *button GCC_UNUSED, gpointer user_data )
{
  fileselector_info *ptr = user_data;
  const char *filename; char *buffer, *buffer2;

  filename =
    gtk_file_selection_get_filename( GTK_FILE_SELECTION( ptr->selector ) );

  ptr->filename = strdup( filename );
  if( !ptr->filename ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    goto end;
  }

  /* And store a copy of the path so we can return here */
  buffer = strdup( filename );
  if( !buffer ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    goto end;
  }

  buffer2 = dirname( buffer );
  
  free( cwd );
  cwd = malloc( strlen( buffer2 ) + 2 );
  if( !cwd ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    goto end;
  }
   
  strcpy( cwd, buffer2 ); strcat( cwd, "/" );
  free( buffer );

  end:
  gtk_widget_destroy( ptr->selector );
  gtk_main_quit();
}
