/* debugger.c: the GTK+ debugger
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

#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "debugger/debugger.h"
#include "fuse.h"
#include "gtkui.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

static int create_dialog( void );
static int activate_debugger( void );
static int deactivate_debugger( void );

static void gtkui_debugger_done_step( GtkWidget *widget, gpointer user_data );
static void gtkui_debugger_done_continue( GtkWidget *widget,
					  gpointer user_data );
static void gtkui_debugger_break( GtkWidget *widget, gpointer user_data );
static void gtkui_debugger_done_close( GtkWidget *widget, gpointer user_data );

/* The debugger dialog box and the PC printout */
static GtkWidget *dialog, *continue_button, *break_button, *registers[10];

/* Have we created the above yet? */
static int dialog_created = 0;

/* Is the debugger window active (as opposed to the debugger itself)? */
static int debugger_active;

int
ui_debugger_activate( void )
{
  fuse_emulation_pause();

  /* Create the dialog box if it doesn't already exist */
  if( !dialog_created ) if( create_dialog() ) return 1;

  gtk_widget_show_all( dialog );

  gtk_widget_set_sensitive( continue_button, 1 );
  gtk_widget_set_sensitive( break_button, 0 );
  if( !debugger_active ) activate_debugger();

  return 0;
}

static int
create_dialog( void )
{
  size_t i;
  GtkWidget *step_button, *close_button, *table, *label;

  const char *register_name[] = { "PC", "SP",
				  "AF", "AF'",
				  "BC", "BC'",
				  "DE", "DE'",
				  "HL", "HL'",
                                };

  dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( dialog ), "Fuse - Debugger" );

  table = gtk_table_new( 5, 4, FALSE );
  gtk_box_pack_start_defaults( GTK_BOX( GTK_DIALOG( dialog )->vbox ), table );

  for( i = 0; i < 10; i++ ) {

    label = gtk_label_new( register_name[i] );
    gtk_table_attach_defaults( GTK_TABLE( table ), label,
			       2*(i%2),   2*(i%2)+1, i/2, i/2+1 );

    registers[i] = gtk_label_new( "" );
    gtk_table_attach_defaults( GTK_TABLE( table ), registers[i],
			       2*(i%2)+1, 2*(i%2)+2, i/2, i/2+1 );

  }

  step_button = gtk_button_new_with_label( "Single Step" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     step_button );

  continue_button = gtk_button_new_with_label( "Continue" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     continue_button );

  break_button = gtk_button_new_with_label( "Break" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     break_button );

  close_button = gtk_button_new_with_label( "Close" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     close_button );

  gtk_signal_connect( GTK_OBJECT( step_button ), "clicked",
		      GTK_SIGNAL_FUNC( gtkui_debugger_done_step ), NULL );
  gtk_signal_connect( GTK_OBJECT( continue_button ), "clicked",
		      GTK_SIGNAL_FUNC( gtkui_debugger_done_continue ), NULL );
  gtk_signal_connect( GTK_OBJECT( break_button ), "clicked",
		      GTK_SIGNAL_FUNC( gtkui_debugger_break ), NULL );
  gtk_signal_connect_object( GTK_OBJECT( close_button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_debugger_done_close ),
			     GTK_OBJECT( dialog ) );

  gtk_signal_connect( GTK_OBJECT( dialog ), "delete_event",
		      GTK_SIGNAL_FUNC( gtkui_debugger_done_close ),
		      (gpointer) NULL );

  /* Esc `cancels' the selector by just continuing with emulation */
  gtk_widget_add_accelerator( close_button, "clicked",
                              gtk_accel_group_get_default(),
                              GDK_Escape, 0, 0 );

  dialog_created = 1;

  return 0;
}

static int
activate_debugger( void )
{
  size_t i;
  char buffer[80];

  WORD *value_ptr[] = { &PC, &SP,
			&AF, &AF_,
			&BC, &BC_,
			&DE, &DE_,
			&HL, &HL_,
                      };

  debugger_active = 1;

  for( i = 0; i < 10; i++ ) {

    snprintf( buffer, 80, "0x%04x", *value_ptr[i] );
    gtk_label_set_text( GTK_LABEL( registers[i] ), buffer );

  }

  gtk_main();
  return 0;
}

static int
deactivate_debugger( void )
{
  gtk_main_quit();
  debugger_active = 0;
  fuse_emulation_unpause();
  return 0;
}

static void
gtkui_debugger_done_step( GtkWidget *widget GCC_UNUSED,
			  gpointer user_data GCC_UNUSED )
{
  debugger_mode = DEBUGGER_MODE_STEP;
  if( debugger_active ) deactivate_debugger();
}

static void
gtkui_debugger_done_continue( GtkWidget *widget GCC_UNUSED,
			      gpointer user_data GCC_UNUSED )
{
  debugger_run();
  gtk_widget_set_sensitive( continue_button, 0 );
  gtk_widget_set_sensitive( break_button, 1 );
  if( debugger_active ) deactivate_debugger();
}

static void
gtkui_debugger_break( GtkWidget *widget GCC_UNUSED,
		      gpointer user_data GCC_UNUSED )
{
  debugger_mode = DEBUGGER_MODE_HALTED;
  gtk_widget_set_sensitive( continue_button, 1 );
  gtk_widget_set_sensitive( break_button, 0 );
}

static void
gtkui_debugger_done_close( GtkWidget *widget, gpointer user_data GCC_UNUSED )
{
  gtk_widget_hide_all( widget );
  gtkui_debugger_done_continue( NULL, NULL );
}
