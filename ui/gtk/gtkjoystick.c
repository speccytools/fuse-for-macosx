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
#include "keyboard.h"
#include "settings.h"

#include "../uijoystick.c"

static void joystick_done( GtkButton *button, gpointer user_data );
static void set_key_text( GtkWidget *label, keyboard_key_name key );
static void key_callback( gpointer data, guint action, GtkWidget *widget );
static void nothing_callback( gpointer data, guint action, GtkWidget *widget );

static GtkItemFactoryEntry key_menu[] = {

  { "/Joystick Fire", NULL, key_callback, KEYBOARD_JOYSTICK_FIRE, "<Item>" },

  { "/Numbers/0", NULL, key_callback, KEYBOARD_0, "<Item>" },
  { "/Numbers/1", NULL, key_callback, KEYBOARD_1, "<Item>" },
  { "/Numbers/2", NULL, key_callback, KEYBOARD_2, "<Item>" },
  { "/Numbers/3", NULL, key_callback, KEYBOARD_3, "<Item>" },
  { "/Numbers/4", NULL, key_callback, KEYBOARD_4, "<Item>" },
  { "/Numbers/5", NULL, key_callback, KEYBOARD_5, "<Item>" },
  { "/Numbers/6", NULL, key_callback, KEYBOARD_6, "<Item>" },
  { "/Numbers/7", NULL, key_callback, KEYBOARD_7, "<Item>" },
  { "/Numbers/8", NULL, key_callback, KEYBOARD_8, "<Item>" },
  { "/Numbers/9", NULL, key_callback, KEYBOARD_9, "<Item>" },

  { "/A-M/A", NULL, key_callback, KEYBOARD_a, "<Item>" },
  { "/A-M/B", NULL, key_callback, KEYBOARD_b, "<Item>" },
  { "/A-M/C", NULL, key_callback, KEYBOARD_c, "<Item>" },
  { "/A-M/D", NULL, key_callback, KEYBOARD_d, "<Item>" },
  { "/A-M/E", NULL, key_callback, KEYBOARD_e, "<Item>" },
  { "/A-M/F", NULL, key_callback, KEYBOARD_f, "<Item>" },
  { "/A-M/G", NULL, key_callback, KEYBOARD_g, "<Item>" },
  { "/A-M/H", NULL, key_callback, KEYBOARD_h, "<Item>" },
  { "/A-M/I", NULL, key_callback, KEYBOARD_i, "<Item>" },
  { "/A-M/J", NULL, key_callback, KEYBOARD_j, "<Item>" },
  { "/A-M/K", NULL, key_callback, KEYBOARD_k, "<Item>" },
  { "/A-M/L", NULL, key_callback, KEYBOARD_l, "<Item>" },
  { "/A-M/M", NULL, key_callback, KEYBOARD_m, "<Item>" },

  { "/N-Z/N", NULL, key_callback, KEYBOARD_n, "<Item>" },
  { "/N-Z/O", NULL, key_callback, KEYBOARD_o, "<Item>" },
  { "/N-Z/P", NULL, key_callback, KEYBOARD_p, "<Item>" },
  { "/N-Z/Q", NULL, key_callback, KEYBOARD_q, "<Item>" },
  { "/N-Z/R", NULL, key_callback, KEYBOARD_r, "<Item>" },
  { "/N-Z/S", NULL, key_callback, KEYBOARD_s, "<Item>" },
  { "/N-Z/T", NULL, key_callback, KEYBOARD_t, "<Item>" },
  { "/N-Z/U", NULL, key_callback, KEYBOARD_u, "<Item>" },
  { "/N-Z/V", NULL, key_callback, KEYBOARD_v, "<Item>" },
  { "/N-Z/W", NULL, key_callback, KEYBOARD_w, "<Item>" },
  { "/N-Z/X", NULL, key_callback, KEYBOARD_x, "<Item>" },
  { "/N-Z/Y", NULL, key_callback, KEYBOARD_y, "<Item>" },
  { "/N-Z/Z", NULL, key_callback, KEYBOARD_z, "<Item>" },

  { "/Space", NULL, key_callback, KEYBOARD_space, "<Item>" },
  { "/Enter", NULL, key_callback, KEYBOARD_Enter, "<Item>" },
  { "/Caps Shift", NULL, key_callback, KEYBOARD_Caps, "<Item>" },
  { "/Symbol Shift", NULL, key_callback, KEYBOARD_Symbol, "<Item>" },

  /* Different callback needed as KEYBOARD_NONE has the value 0 */
  { "/Nothing", NULL, nothing_callback, 1, "<Item>" },

};

static const guint key_menu_count = sizeof( key_menu ) / sizeof( key_menu[0] );

struct joystick_info {

  GtkWidget *radio[ JOYSTICK_TYPE_COUNT ];
  int *type, *fire;

  GtkWidget *fire_label;
  keyboard_key_name key;

};

void
menu_options_joysticks_select( gpointer callback_data, guint callback_action,
			       GtkWidget *widget )
{
  GtkWidget *dialog, *table, *label, *menu, *button;
  GSList *button_group;
  GtkItemFactory *factory;
  struct joystick_info info;
  size_t i;

  fuse_emulation_pause();

  dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( dialog ), "Fuse - Configure joystick" );

  switch( callback_action ) {

  case 1:
    info.type = &( settings_current.joystick_1_output );
    info.fire = &( settings_current.joystick_1_fire_1 );
    break;

  case 2:
    info.type = &( settings_current.joystick_2_output );
    info.fire = &( settings_current.joystick_2_fire_1 );
    break;

  case 3:
    info.type = &( settings_current.joystick_keyboard_output );
    info.fire = NULL;
    break;
    

  }

  table = gtk_table_new( JOYSTICK_TYPE_COUNT + 2, 2, FALSE );
  gtk_box_pack_start_defaults( GTK_BOX( GTK_DIALOG( dialog )->vbox ),
			       table );

  label = gtk_label_new( "Joystick type" );
  gtk_table_attach_defaults( GTK_TABLE( table ), label, 0, 1,
			     0, JOYSTICK_TYPE_COUNT );

  button_group = NULL;

  for( i = 0; i < JOYSTICK_TYPE_COUNT; i++ ) {

    info.radio[ i ] =
      gtk_radio_button_new_with_label( button_group, joystick_name[ i ] );
    button_group =
      gtk_radio_button_group( GTK_RADIO_BUTTON( info.radio[ i ] ) );
    gtk_table_attach_defaults( GTK_TABLE( table ), info.radio[ i ], 1, 2,
			       i, i + 1 );

    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( info.radio[ i ] ),
				    i == *( info.type ) );

  }

  /* The fire button selector */

  if( info.fire ) {

    label = gtk_label_new( "Button 1" );
    gtk_table_attach_defaults( GTK_TABLE( table ), label, 0, 1,
			       JOYSTICK_TYPE_COUNT, JOYSTICK_TYPE_COUNT + 1 );

    info.key = *( info.fire );
    info.fire_label = gtk_label_new( "" );

    for( i = 0; i < key_menu_count; i++ ) {
    
      keyboard_key_name key;

      if( key_menu[i].callback == nothing_callback ) {
	key = KEYBOARD_NONE;
      } else {
	key = key_menu[i].callback_action;
      }

      if( key == *( info.fire ) ) {
	set_key_text( info.fire_label, key );
	break;
      }

    }

    gtk_table_attach_defaults( GTK_TABLE( table ), info.fire_label, 0, 1,
			       JOYSTICK_TYPE_COUNT + 1,
			       JOYSTICK_TYPE_COUNT + 2 );

    factory = gtk_item_factory_new( GTK_TYPE_OPTION_MENU, "<fire>", NULL );
    gtk_item_factory_create_items( factory, key_menu_count, key_menu, &info );

    menu = gtk_item_factory_get_widget( factory, "<fire>" );
    gtk_table_attach_defaults( GTK_TABLE( table ), menu, 1, 2,
			       JOYSTICK_TYPE_COUNT, JOYSTICK_TYPE_COUNT + 2 );
  }

  /* Create and add the action buttons to the dialog box */
  button = gtk_button_new_with_label( "OK" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     button );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked",
		      GTK_SIGNAL_FUNC( joystick_done ), &info );
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
set_key_text( GtkWidget *label, keyboard_key_name key )
{
  const char *text;
  char buffer[40];

  text = keyboard_key_text( key );

  snprintf( buffer, 40, "(currently: %s)", text );

  gtk_label_set_text( GTK_LABEL( label ), buffer );
}

static void
key_callback( gpointer data, guint action, GtkWidget *widget )
{
  struct joystick_info *info = data;

  info->key = action;
  set_key_text( info->fire_label, info->key );
}

static void
nothing_callback( gpointer data, guint action, GtkWidget *widget )
{
  key_callback( data, KEYBOARD_NONE, NULL );
}

static void
joystick_done( GtkButton *button GCC_UNUSED, gpointer user_data )
{
  struct joystick_info *info = user_data;

  int i;
  GtkToggleButton *toggle;

  if( info->fire ) *( info->fire ) = info->key;

  for( i = 0; i < JOYSTICK_TYPE_COUNT; i++ ) {

    toggle = GTK_TOGGLE_BUTTON( info->radio[ i ] );

    if( gtk_toggle_button_get_active( toggle ) ) {
      *( info->type ) = i;
      return;
    }

  }

}

#endif				/* #ifdef UI_GTK */
