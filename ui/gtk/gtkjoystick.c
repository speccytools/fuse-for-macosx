/* gtkjoystick.c: Joystick emulation
   Copyright (c) 2003-2004 Darren Salt, Philip Kendall

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

   Darren: linux@youmustbejoking.demon.co.uk

*/

#include <config.h>

#ifdef UI_GTK

#include <gtk/gtk.h>

#include "compat.h"
#include "fuse.h"
#include "gtkinternals.h"
#include "joystick.h"
#include "settings.h"

#include "../uijoystick.c"

static void create_button_group( GtkWidget **radio, GtkBox *parent,
				 const char *title, joystick_type_t current );
static void joystick_done( GtkButton *button, gpointer user_data );

void
gtkjoystick_select( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  GtkWidget *dialog, *button, *radio[ 3 * JOYSTICK_TYPE_COUNT ];
  GtkBox *box;

  fuse_emulation_pause();

  dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( dialog ), "Fuse - Select joysticks" );

  box = GTK_BOX( GTK_DIALOG( dialog )->vbox );

  create_button_group( &radio[ 0 * JOYSTICK_TYPE_COUNT ], box,
		       "Joystick 1", settings_current.joystick_1_output );
  create_button_group( &radio[ 1 * JOYSTICK_TYPE_COUNT ], box,
		       "Joystick 2", settings_current.joystick_2_output );
  create_button_group( &radio[ 2 * JOYSTICK_TYPE_COUNT ], box,
		       "Keyboard", settings_current.joystick_keyboard_output );

  /* Create and add the action buttons to the dialog box */
  button = gtk_button_new_with_label( "OK" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     button );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked",
		      GTK_SIGNAL_FUNC( joystick_done ), radio );
  gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
			     GTK_OBJECT( dialog ) );

  button = gtk_button_new_with_label( "Cancel" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     button );
  gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
			     GTK_OBJECT( dialog ) );

  gtk_window_set_modal( GTK_WINDOW( dialog ), TRUE );
  gtk_widget_show_all( dialog );

  gtk_main();

  fuse_emulation_unpause();
}

static void
create_button_group( GtkWidget **radio, GtkBox *parent, const char *title,
		     joystick_type_t current )
{
  GtkWidget *frame, *box;
  GSList *button_group;
  size_t i;

  button_group = NULL;

  frame = gtk_frame_new( title );
  gtk_box_pack_start_defaults( parent, frame );

  box = gtk_vbox_new( FALSE, FALSE );
  gtk_container_add( GTK_CONTAINER( frame ), box );

  for( i = 0; i < JOYSTICK_TYPE_COUNT; i++ ) {

    radio[ i ] =
      gtk_radio_button_new_with_label( button_group, joystick_name[ i ] );
    button_group = gtk_radio_button_group( GTK_RADIO_BUTTON( radio[ i ] ) );
    gtk_box_pack_start_defaults( GTK_BOX( box ), radio[ i ] );

    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( radio[ i ] ),
				  i == current );

  }
}

static void
joystick_done( GtkButton *button GCC_UNUSED, gpointer user_data )
{
  GtkWidget **radio = user_data;

  int joystick, i;
  int *setting = NULL;
  GtkWidget **group;

  for( joystick = 0; joystick < 3; joystick++ ) {

    group = &radio[ joystick * JOYSTICK_TYPE_COUNT ];

    switch( joystick ) {
    case 0: setting = &settings_current.joystick_1_output; break;
    case 1: setting = &settings_current.joystick_2_output; break;
    case 2: setting = &settings_current.joystick_keyboard_output; break;
    }

    for( i = 0; i < JOYSTICK_TYPE_COUNT; i++ ) {
      if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( group[ i ] ) ) ) {
	*setting = i;
	break;
      }
    }

  }

}

#endif				/* #ifdef UI_GTK */
