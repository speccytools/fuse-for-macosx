/* gtkui.c: GTK+ routines for dealing with the user interface
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

#ifdef UI_GTK		/* Use this file iff we're using GTK+ */

#include <stdio.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <glib.h>

#include "display.h"
#include "fuse.h"
#include "gtkkeyboard.h"
#include "gtkui.h"
#include "machine.h"
#include "options.h"
#include "rzx.h"
#include "screenshot.h"
#include "settings.h"
#include "snapshot.h"
#include "specplus3.h"
#include "spectrum.h"
#include "tape.h"
#include "timer.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "widget/widget.h"

/* The main Fuse window */
GtkWidget *gtkui_window;

/* The area into which the screen will be drawn */
GtkWidget *gtkui_drawing_area;

/* Popup menu widget(s), as invoked by F1 */
GtkWidget *gtkui_menu_popup;

static gboolean gtkui_make_menu(GtkAccelGroup **accel_group,
				GtkWidget **menu_bar,
				GtkItemFactoryEntry *menu_data,
				guint menu_data_size);

static gint gtkui_delete(GtkWidget *widget, GdkEvent *event,
			      gpointer data);
static void gtkui_open(GtkWidget *widget, gpointer data);
static void gtkui_save(GtkWidget *widget, gpointer data);

static void gtkui_rzx_start( GtkWidget *widget, gpointer data );
static void gtkui_rzx_start_snap( GtkWidget *widget, gpointer data );
static void gtkui_rzx_stop( GtkWidget *widget, gpointer data );
static void gtkui_rzx_play( GtkWidget *widget, gpointer data );
static int gtkui_open_snap( void );

#ifdef HAVE_PNG_H
static void gtkui_save_screen( GtkWidget *widget, gpointer data );
#endif				/* #ifdef HAVE_PNG_H */

static void gtkui_quit(GtkWidget *widget, gpointer data);
static void gtkui_reset(GtkWidget *widget, gpointer data);

static void gtkui_select(GtkWidget *widget, gpointer data);
static void gtkui_select_done( GtkWidget *widget, gpointer user_data );

static void gtkui_tape_open( GtkWidget *widget, gpointer data );
static void gtkui_tape_play( GtkWidget *widget, gpointer data );
static void gtkui_tape_rewind( GtkWidget *widget, gpointer data );
static void gtkui_tape_clear( GtkWidget *widget, gpointer data );
static void gtkui_tape_write( GtkWidget *widget, gpointer data );

#ifdef HAVE_765_H
static void gtkui_disk_open_a( GtkWidget *widget, gpointer data );
static void gtkui_disk_open_b( GtkWidget *widget, gpointer data );
static void gtkui_disk_eject_a( GtkWidget *widget, gpointer data );
static void gtkui_disk_eject_b( GtkWidget *widget, gpointer data );

static void gtkui_disk_open( specplus3_drive_number drive );
#endif				/* #ifdef HAVE_765_H */

static void gtkui_help_keyboard( GtkWidget *widget, gpointer data );

static char* gtkui_fileselector_get_filename( const char *title );
static void gtkui_fileselector_done( GtkButton *button, gpointer user_data );
static void gtkui_fileselector_cancel( GtkButton *button, gpointer user_data );

static GtkItemFactoryEntry gtkui_menu_data[] = {
  { "/File",		        NULL , NULL,                0, "<Branch>"    },
  { "/File/_Open Snapshot..." , "F3" , gtkui_open,          0, NULL          },
  { "/File/_Save Snapshot..." , "F2" , gtkui_save,          0, NULL          },
  { "/File/separator",          NULL , NULL,                0, "<Separator>" },
  { "/File/_Recording",		NULL , NULL,		    0, "<Branch>"    },
  { "/File/Recording/_Record...",NULL, gtkui_rzx_start,     0, NULL	     },
  { "/File/Recording/Record _from snapshot...",
                                NULL , gtkui_rzx_start_snap,0, NULL          },
  { "/File/Recording/_Play...", NULL , gtkui_rzx_play,	    0, NULL          },
  { "/File/Recording/_Stop",    NULL , gtkui_rzx_stop,	    0, NULL          },

#ifdef HAVE_PNG_H
  { "/File/Save S_creen...",    NULL , gtkui_save_screen,   0, NULL          },
#endif				/* #ifdef HAVE_PNG_H */

  { "/File/separator",          NULL , NULL,                0, "<Separator>" },
  { "/File/E_xit",	        "F10", gtkui_quit,          0, NULL          },
  { "/Options",		        NULL , NULL,                0, "<Branch>"    },
  { "/Options/_General...",     "F4" , gtkoptions_general,  0, NULL          },
  { "/Options/_Sound...",	NULL , gtkoptions_sound,    0, NULL          },
  { "/Options/_RZX...",		NULL , gtkoptions_rzx,      0, NULL          },
  { "/Machine",		        NULL , NULL,                0, "<Branch>"    },
  { "/Machine/_Reset",	        "F5" , gtkui_reset,         0, NULL          },
  { "/Machine/_Select...",      "F9" , gtkui_select,        0, NULL          },
  { "/Tape",                    NULL , NULL,                0, "<Branch>"    },
  { "/Tape/_Open...",	        "F7" , gtkui_tape_open,     0, NULL          },
  { "/Tape/_Play",	        "F8" , gtkui_tape_play,     0, NULL          },
  { "/Tape/_Browse...",		NULL , gtk_tape_browse,     0, NULL          },
  { "/Tape/_Rewind",		NULL , gtkui_tape_rewind,   0, NULL          },
  { "/Tape/_Clear",		NULL , gtkui_tape_clear,    0, NULL          },
  { "/Tape/_Write...",		"F6" , gtkui_tape_write,    0, NULL          },

#ifdef HAVE_765_H
  { "/Disk",			NULL , NULL,		    0, "<Branch>"    },
  { "/Disk/Drive A:",		NULL , NULL,		    0, "<Branch>"    },
  { "/Disk/Drive A:/_Insert...",NULL , gtkui_disk_open_a,   0, NULL          },
  { "/Disk/Drive A:/_Eject",    NULL , gtkui_disk_eject_a,  0, NULL          },
  { "/Disk/Drive B:",		NULL , NULL,		    0, "<Branch>"    },
  { "/Disk/Drive B:/_Insert...",NULL , gtkui_disk_open_b,   0, NULL          },
  { "/Disk/Drive B:/_Eject",    NULL , gtkui_disk_eject_b,  0, NULL          },
#endif				/* #ifdef HAVE_765_H */

  { "/Help",			NULL , NULL,		    0, "<Branch>"    },
  { "/Help/_Keyboard...",	NULL , gtkui_help_keyboard, 0, NULL	     },
};
static guint gtkui_menu_data_size =
  sizeof( gtkui_menu_data ) / sizeof( GtkItemFactoryEntry );
  
int ui_init(int *argc, char ***argv, int width, int height)
{
  GtkWidget *box,*menu_bar;
  GtkAccelGroup *accel_group;
  GdkGeometry geometry;

  gtk_init(argc,argv);

  gtkui_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title( GTK_WINDOW(gtkui_window), "Fuse" );
  gtk_window_set_wmclass( GTK_WINDOW(gtkui_window), fuse_progname, "Fuse" );
  gtk_window_set_default_size( GTK_WINDOW(gtkui_window), width, height);
  gtk_signal_connect(GTK_OBJECT(gtkui_window), "delete_event",
		     GTK_SIGNAL_FUNC(gtkui_delete), NULL);
  gtk_signal_connect(GTK_OBJECT(gtkui_window), "key-press-event",
		     GTK_SIGNAL_FUNC(gtkkeyboard_keypress), NULL);
  gtk_widget_add_events( gtkui_window, GDK_KEY_RELEASE_MASK );
  gtk_signal_connect(GTK_OBJECT(gtkui_window), "key-release-event",
		     GTK_SIGNAL_FUNC(gtkkeyboard_keyrelease), NULL);

  /* If we lose the focus, disable all keys */
  gtk_signal_connect( GTK_OBJECT( gtkui_window ), "focus-out-event",
		      GTK_SIGNAL_FUNC( gtkkeyboard_release_all ), NULL );

  box = gtk_vbox_new( FALSE, 0 );
  gtk_container_add(GTK_CONTAINER(gtkui_window), box);
  gtk_widget_show(box);

  gtkui_make_menu(&accel_group, &menu_bar, gtkui_menu_data,
		  gtkui_menu_data_size);

  gtk_window_add_accel_group( GTK_WINDOW(gtkui_window), accel_group );
  gtk_box_pack_start( GTK_BOX(box), menu_bar, FALSE, FALSE, 0 );
  gtk_widget_show(menu_bar);
  
  gtkui_drawing_area = gtk_drawing_area_new();
  if(!gtkui_drawing_area) {
    fprintf(stderr,"%s: couldn't create drawing area at %s:%d\n",
	    fuse_progname,__FILE__,__LINE__);
    return 1;
  }
  gtk_drawing_area_size( GTK_DRAWING_AREA(gtkui_drawing_area),
			 width, height );
  gtk_box_pack_start( GTK_BOX(box), gtkui_drawing_area, FALSE, FALSE, 0 );

  geometry.min_width = width;
  geometry.min_height = height;
  geometry.max_width = 2 * width;
  geometry.max_height = 2 * height;
  geometry.base_width = 0;
  geometry.base_height = 0;
  geometry.width_inc = width;
  geometry.height_inc = height;
  geometry.min_aspect = geometry.max_aspect = ((float)width)/height;

  gtk_window_set_geometry_hints( GTK_WINDOW(gtkui_window), gtkui_drawing_area,
				 &geometry,
				 GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE |
				 GDK_HINT_BASE_SIZE | GDK_HINT_RESIZE_INC |
				 GDK_HINT_ASPECT );


  gtk_widget_show(gtkui_drawing_area);

  if(uidisplay_init(width,height)) return 1;

  gtk_widget_show(gtkui_window);

  return 0;
}

static gboolean gtkui_make_menu(GtkAccelGroup **accel_group,
				GtkWidget **menu_bar,
				GtkItemFactoryEntry *menu_data,
				guint menu_data_size)
{
  GtkItemFactory *item_factory;

  *accel_group = gtk_accel_group_new();
  item_factory = gtk_item_factory_new( GTK_TYPE_MENU_BAR, "<main>",
				       *accel_group );
  gtk_item_factory_create_items(item_factory, menu_data_size, menu_data, NULL);
  *menu_bar = gtk_item_factory_get_widget( item_factory, "<main>" );

  /* We have to recreate the menus for the popup, unfortunately... */
  item_factory = gtk_item_factory_new( GTK_TYPE_MENU, "<main>", NULL );
  gtk_item_factory_create_items(item_factory, menu_data_size, menu_data, NULL);
  gtkui_menu_popup = gtk_item_factory_get_widget( item_factory, "<main>" );

  return FALSE;
}

static void
gtkui_popup_menu_pos( GtkMenu *menu GCC_UNUSED, gint *xp, gint *yp,
		      GtkWidget *data GCC_UNUSED )
{
  gdk_window_get_position( gtkui_window->window, xp, yp );
}

/* Popup main menu, as invoked by F1. */
void gtkui_popup_menu(void)
{
  gtk_menu_popup( GTK_MENU(gtkui_menu_popup), NULL, NULL,
		  (GtkMenuPositionFunc)gtkui_popup_menu_pos, NULL,
		  0, 0 );
}

int ui_event(void)
{
  while(gtk_events_pending())
    gtk_main_iteration();
  return 0;
}

int ui_end(void)
{
  int error;
  
  /* Don't display the window whilst doing all this! */
  gtk_widget_hide(gtkui_window);

  /* Tidy up the low-level stuff */
  error = uidisplay_end(); if(error) return error;

  /* Now free up the window itself */
/*    XDestroyWindow(display,mainWindow); */

  /* And disconnect from the X server */
/*    XCloseDisplay(display); */

  return 0;
}

/* The callbacks used by various routines */

/* Called by the main window on a "delete_event" */
static gint
gtkui_delete( GtkWidget *widget GCC_UNUSED, GdkEvent *event GCC_UNUSED,
	      gpointer data GCC_UNUSED )
{
  fuse_exiting=1;
  return TRUE;
}

/* Called by the menu when File/Open selected */
static void
gtkui_open( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  char *filename;

  fuse_emulation_pause();

  filename = gtkui_fileselector_get_filename( "Fuse - Load Snapshot" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  snapshot_read( filename );

  free( filename );

  display_refresh_all();

  fuse_emulation_unpause();
}

/* Called by the menu when File/Save selected */
static void
gtkui_save( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  char *filename;

  fuse_emulation_pause();

  filename = gtkui_fileselector_get_filename( "Fuse - Save Snapshot" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  snapshot_write( filename );

  free( filename );

  fuse_emulation_unpause();
}

/* Called when File/Recording/Record selected */
static void
gtkui_rzx_start( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  char *recording;

  if( rzx_playback || rzx_recording ) return;

  fuse_emulation_pause();

  recording = gtkui_fileselector_get_filename( "Fuse - Start Recording" );
  if( !recording ) { fuse_emulation_unpause(); return; }

  rzx_start_recording( recording, 1 );

  free( recording );

  fuse_emulation_unpause();
}

/* Called when `File/Recording/Record from snapshot' selected */
static void
gtkui_rzx_start_snap( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  char *snap, *recording;

  if( rzx_playback || rzx_recording ) return;

  fuse_emulation_pause();

  snap = gtkui_fileselector_get_filename( "Fuse - Load Snapshot " );
  if( !snap ) { fuse_emulation_unpause(); return; }

  recording = gtkui_fileselector_get_filename( "Fuse - Start Recording" );
  if( !recording ) { free( snap ); fuse_emulation_unpause(); return; }

  if( snapshot_read( snap ) ) {
    free( snap ); free( recording ); fuse_emulation_unpause(); return;
  }

/*    rzx_start_recording( recording, 0 ); */

  free( recording );

  display_refresh_all();

  fuse_emulation_unpause();
}

/* Called when File/Recording/Stop selected */
static void
gtkui_rzx_stop( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  if( rzx_recording ) rzx_stop_recording();
  if( rzx_playback  ) rzx_stop_playback( 1 );
}

/* Called when File/Recording/Play selected */
static void
gtkui_rzx_play( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  char *recording;

  if( rzx_playback || rzx_recording ) return;

  fuse_emulation_pause();

  recording = gtkui_fileselector_get_filename( "Fuse - Start Replay" );
  if( !recording ) { fuse_emulation_unpause(); return; }

  rzx_start_playback( recording, gtkui_open_snap );

  free( recording );

  display_refresh_all();

  fuse_emulation_unpause();
}

static int
gtkui_open_snap( void )
{
  char *filename;
  int error;

  filename = gtkui_fileselector_get_filename( "Fuse - Load Snapshot" );
  if( !filename ) return -1;

  error = snapshot_read( filename );
  free( filename );
  return error;
}

#ifdef HAVE_PNG_H
/* File/Save Screenshot */
static void
gtkui_save_screen( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  char *filename;

  fuse_emulation_pause();

  filename = gtkui_fileselector_get_filename( "Fuse - Save Screenshot" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  screenshot_save();
  screenshot_write( filename );

  free( filename );

  fuse_emulation_unpause();
}
#endif				/* #ifdef HAVE_PNG_H */

/* Called by the menu when File/Exit selected */
static void
gtkui_quit( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  fuse_exiting=1;
}

/* Called by the menu when Machine/Reset selected */
static void
gtkui_reset( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  machine_current->reset();
}

typedef struct gtkui_select_info {

  GtkWidget *dialog;
  GtkWidget **buttons;

} gtkui_select_info;

/* Called by the menu when Machine/Select selected */
static void
gtkui_select( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  gtkui_select_info dialog;
  GSList *button_group;

  GtkWidget *ok_button, *cancel_button;

  int i;
  
  /* Some space to store the radio buttons in */
  dialog.buttons = (GtkWidget**)malloc( machine_count * sizeof(GtkWidget* ) );
  if( dialog.buttons == NULL ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return;
  }

  /* Stop emulation */
  fuse_emulation_pause();

  /* Create the necessary widgets */
  dialog.dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( dialog.dialog ), "Fuse - Select Machine" );

  dialog.buttons[0] =
    gtk_radio_button_new_with_label(
      NULL, libspectrum_machine_name( machine_types[0]->machine )
    );
  button_group =
    gtk_radio_button_group( GTK_RADIO_BUTTON( dialog.buttons[0] ) );

  for( i=1; i<machine_count; i++ ) {
    dialog.buttons[i] =
      gtk_radio_button_new_with_label(
        gtk_radio_button_group (GTK_RADIO_BUTTON (dialog.buttons[i-1] ) ),
	libspectrum_machine_name( machine_types[i]->machine )
      );
  }

  for( i=0; i<machine_count; i++ ) {
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dialog.buttons[i] ),
				  machine_current == machine_types[i] );
    gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog.dialog )->vbox ),
		       dialog.buttons[i] );
  }

  /* Create and add the actions buttons to the dialog box */
  ok_button = gtk_button_new_with_label( "OK" );
  cancel_button = gtk_button_new_with_label( "Cancel" );

  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog.dialog )->action_area ),
		     ok_button );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog.dialog )->action_area ),
		     cancel_button );

  /* Add the necessary callbacks */
  gtk_signal_connect( GTK_OBJECT( ok_button ), "clicked",
		      GTK_SIGNAL_FUNC( gtkui_select_done ),
		      (gpointer) &dialog );
  gtk_signal_connect_object( GTK_OBJECT( cancel_button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
			     GTK_OBJECT( dialog.dialog ) );
  gtk_signal_connect( GTK_OBJECT( dialog.dialog ), "delete_event",
		      GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
		      (gpointer) NULL );

  /* Allow Esc to cancel */
  gtk_widget_add_accelerator( cancel_button, "clicked",
                              gtk_accel_group_get_default(),
                              GDK_Escape, 0, 0 );

  /* Set the window to be modal and display it */
  gtk_window_set_modal( GTK_WINDOW( dialog.dialog ), TRUE );
  gtk_widget_show_all( dialog.dialog );

  /* Process events until the window is done with */
  gtk_main();

  /* And then carry on with emulation again */
  fuse_emulation_unpause();
}

/* Callback used by the machine selection dialog */
static void
gtkui_select_done( GtkWidget *widget GCC_UNUSED, gpointer user_data )
{
  int i;
  gtkui_select_info *ptr = (gtkui_select_info*)user_data;

  for( i=0; i<machine_count; i++ ) {
    if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(ptr->buttons[i]) ) &&
        machine_current != machine_types[i]
      )
    {
      machine_select( machine_types[i]->machine );
    }
  }

  gtk_widget_destroy( ptr->dialog );
  gtk_main_quit();
}
    
/* Called by the menu when Tape/Open selected */
static void
gtkui_tape_open( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  char *filename;

  fuse_emulation_pause();

  filename = gtkui_fileselector_get_filename( "Fuse - Open Tape" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  tape_open( filename );

  free( filename );

  fuse_emulation_unpause();
}

/* Called by the menu when Tape/Play selected */
static void
gtkui_tape_play( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  tape_toggle_play();
}

/* Called by the menu when Tape/Rewind selected */
static void
gtkui_tape_rewind( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  tape_select_block( 0 );
}

/* Called by the menu when Tape/Clear selected */
static void
gtkui_tape_clear( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  tape_close();
}

/* Called by the menu when Tape/Write selected */
static void
gtkui_tape_write( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  char *filename;

  fuse_emulation_pause();

  filename = gtkui_fileselector_get_filename( "Fuse - Write Tape" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  tape_write( filename );

  free( filename );

  fuse_emulation_unpause();
}

#ifdef HAVE_765_H

/* Called by the mnu when Disk/Drive ?:/Open selected */
static void
gtkui_disk_open_a( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  gtkui_disk_open( SPECPLUS3_DRIVE_A );
}

static void
gtkui_disk_open_b( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  gtkui_disk_open( SPECPLUS3_DRIVE_B );
}

static void gtkui_disk_open( specplus3_drive_number drive )
{
  char *filename;

  fuse_emulation_pause();

  filename = gtkui_fileselector_get_filename(
    ( drive == SPECPLUS3_DRIVE_A ? "Fuse - Insert disk into drive A:" :
                                   "Fuse - Insert disk into drive B:" ) );
  if( !filename ) { fuse_emulation_unpause(); return; }

  specplus3_disk_insert( drive, filename );
  free( filename );

  fuse_emulation_unpause();
}

static void
gtkui_disk_eject_a( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  specplus3_disk_eject( SPECPLUS3_DRIVE_A );
}

static void
gtkui_disk_eject_b( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  specplus3_disk_eject( SPECPLUS3_DRIVE_B );
}

#endif			/* #ifdef HAVE_765_H */

static void
gtkui_help_keyboard( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  widget_picture_data picture_data = { "keyboard.scr", NULL, 0 };

  widget_menu_keyboard( &picture_data );
}

/* Generic `tidy-up' callback */
void
gtkui_destroy_widget_and_quit( GtkWidget *widget, gpointer data GCC_UNUSED )
{
  gtk_widget_destroy( widget );
  gtk_main_quit();
}

/* Bits used for the file selection dialogs */

typedef struct gktui_fileselector_info {

  GtkWidget *selector;
  gchar *filename;

} gtkui_fileselector_info;

static char* gtkui_fileselector_get_filename( const char *title )
{
  gtkui_fileselector_info selector;

  selector.selector = gtk_file_selection_new( title );
  selector.filename = NULL;

  gtk_signal_connect(
      GTK_OBJECT( GTK_FILE_SELECTION( selector.selector )->ok_button ),
      "clicked",
      GTK_SIGNAL_FUNC( gtkui_fileselector_done ),
      (gpointer) &selector );

  gtk_signal_connect(
       GTK_OBJECT( GTK_FILE_SELECTION( selector.selector )->cancel_button),
       "clicked",
       GTK_SIGNAL_FUNC(gtkui_fileselector_cancel ),
       (gpointer) &selector );

  gtk_signal_connect( GTK_OBJECT( selector.selector ), "delete_event",
		      GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
		      (gpointer) &selector );

  /* Allow Esc to cancel */
  gtk_widget_add_accelerator(
       GTK_FILE_SELECTION( selector.selector )->cancel_button,
       "clicked",
       gtk_accel_group_get_default(),
       GDK_Escape, 0, 0 );

  gtk_window_set_modal( GTK_WINDOW( selector.selector ), TRUE );

  gtk_widget_show( selector.selector );

  gtk_main();

  return selector.filename;
}

static void
gtkui_fileselector_done( GtkButton *button GCC_UNUSED, gpointer user_data )
{
  gtkui_fileselector_info *ptr = (gtkui_fileselector_info*) user_data;
  char *filename;

  filename =
    gtk_file_selection_get_filename( GTK_FILE_SELECTION( ptr->selector ) );

  /* FIXME: what to do if this runs out of memory? */
  ptr->filename = strdup( filename );

  gtk_widget_destroy( ptr->selector );

  gtk_main_quit();
}

static void
gtkui_fileselector_cancel( GtkButton *button GCC_UNUSED, gpointer user_data )
{
  gtkui_fileselector_info *ptr = (gtkui_fileselector_info*) user_data;

  gtk_widget_destroy( ptr->selector );

  gtk_main_quit();
}

#endif			/* #ifdef UI_GTK */
