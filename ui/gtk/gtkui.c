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

#include <libspectrum.h>

#include "dck.h"
#include "debugger/debugger.h"
#include "display.h"
#include "event.h"
#include "fuse.h"
#include "gtkdisplay.h"
#include "gtkkeyboard.h"
#include "gtkui.h"
#include "machine.h"
#include "options.h"
#include "psg.h"
#include "rzx.h"
#include "screenshot.h"
#include "settings.h"
#include "snapshot.h"
#include "specplus3.h"
#include "spectrum.h"
#include "tape.h"
#include "timer.h"
#include "trdos.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "ui/scaler/scaler.h"
#include "utils.h"

/* The main Fuse window */
GtkWidget *gtkui_window;

/* The area into which the screen will be drawn */
GtkWidget *gtkui_drawing_area;

/* Popup menu widget(s), as invoked by F1 */
GtkWidget *gtkui_menu_popup;

/* The item factory used to create the menu bar */
GtkItemFactory *menu_factory;

/* And that used to create the popup menus */
GtkItemFactory *popup_factory;

/* Structure used by the radio button selection widgets (the graphics
   filter selectors and Machine/Select) */
typedef struct gtkui_select_info {

  GtkWidget *dialog;
  GtkWidget **buttons;

  /* Used by the graphics filter selectors */
  ui_scaler_available available;
  scaler_type selected;

} gtkui_select_info;

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

static void gtkui_psg_start( GtkWidget *widget, gpointer data );
static void gtkui_psg_stop( GtkWidget *widget, gpointer data );

static void gtkui_open_scr( GtkWidget *widget, gpointer data );
static void gtkui_save_scr( GtkWidget *widget, gpointer data );
#ifdef USE_LIBPNG
static void gtkui_save_screen( GtkWidget *widget, gpointer data );
#endif				/* #ifdef USE_LIBPNG */

static void gtkui_quit(GtkWidget *widget, gpointer data);

static void select_filter(GtkWidget *widget, gpointer data);
static void select_filter_done( GtkWidget *widget, gpointer user_data );

#ifdef HAVE_LIB_XML2
static void save_options( GtkWidget *widget, gpointer data );
#endif				/* #ifdef HAVE_LIB_XML2 */

static void gtkui_reset(GtkWidget *widget, gpointer data);

static void gtkui_select(GtkWidget *widget, gpointer data);
static void gtkui_select_done( GtkWidget *widget, gpointer user_data );

static void gtkui_break( GtkWidget *widget, gpointer data );
static void gtkui_nmi( GtkWidget *widget, gpointer data );

static void gtkui_tape_open( GtkWidget *widget, gpointer data );
static void gtkui_tape_play( GtkWidget *widget, gpointer data );
static void gtkui_tape_rewind( GtkWidget *widget, gpointer data );
static void gtkui_tape_clear( GtkWidget *widget, gpointer data );
static void gtkui_tape_write( GtkWidget *widget, gpointer data );

static void gtkui_disk_open_a( GtkWidget *widget, gpointer data );
static void gtkui_disk_open_b( GtkWidget *widget, gpointer data );
static void gtkui_disk_eject_a( GtkWidget *widget, gpointer data );
static void gtkui_disk_eject_b( GtkWidget *widget, gpointer data );

static void gtkui_disk_open( specplus3_drive_number drive );

static void cartridge_insert( GtkWidget *widget, gpointer data );
static void cartridge_eject( GtkWidget *widget, gpointer data );

static void gtkui_help_keyboard( GtkWidget *widget, gpointer data );

static void gtkui_fileselector_done( GtkButton *button, gpointer user_data );
static void gtkui_fileselector_cancel( GtkButton *button, gpointer user_data );

/* Set a menu item (in)active in both the menu bar and the popup menus */
static int set_menu_item_active( const char *path, int active );

static GtkItemFactoryEntry gtkui_menu_data[] = {
  { "/File",		        NULL , NULL,                0, "<Branch>"    },
  { "/File/_Open...",		"F3" , gtkui_open,          0, NULL          },
  { "/File/_Save Snapshot..." , "F2" , gtkui_save,          0, NULL          },
  { "/File/_Recording",		NULL , NULL,		    0, "<Branch>"    },
  { "/File/Recording/_Record...",NULL, gtkui_rzx_start,     0, NULL	     },
  { "/File/Recording/Record _from snapshot...",
                                NULL , gtkui_rzx_start_snap,0, NULL          },
  { "/File/Recording/_Play...", NULL , gtkui_rzx_play,	    0, NULL          },
  { "/File/Recording/_Stop",    NULL , gtkui_rzx_stop,	    0, NULL          },
  { "/File/A_Y Logging",        NULL , NULL,                0, "<Branch>"    },
  { "/File/AY Logging/_Record...",NULL, gtkui_psg_start,    0, NULL          },
  { "/File/AY Logging/_Stop",   NULL , gtkui_psg_stop,      0, NULL          },

  { "/File/separator",          NULL , NULL,                0, "<Separator>" },
  { "/File/O_pen SCR Screenshot...", NULL, gtkui_open_scr,  0, NULL          },
  { "/File/S_ave Screen as SCR...", NULL, gtkui_save_scr,   0, NULL          },
#ifdef USE_LIBPNG
  { "/File/Save S_creen as PNG...", NULL,gtkui_save_screen, 0, NULL          },
#endif				/* #ifdef USE_LIBPNG */

  { "/File/separator",          NULL , NULL,                0, "<Separator>" },
  { "/File/E_xit",	        "F10", gtkui_quit,          0, NULL          },
  { "/Options",			NULL , NULL,                0, "<Branch>"    },
  { "/Options/_General...",     "F4" , gtkoptions_general,  0, NULL          },
  { "/Options/_Sound...",	NULL , gtkoptions_sound,    0, NULL          },
  { "/Options/_RZX...",		NULL , gtkoptions_rzx,      0, NULL          },
  { "/Options/S_elect ROMs...", NULL , gtkui_roms,          0, NULL          },
  { "/Options/_Filter...",	NULL , select_filter,	    0, NULL          },

#ifdef HAVE_LIB_XML2
  { "/Options/separator",       NULL , NULL,                0, "<Separator>" },
  { "/Options/S_ave",		NULL , save_options,	    0, NULL          },
#endif				/* #ifdef HAVE_LIB_XML2 */

  { "/Machine",		        NULL , NULL,                0, "<Branch>"    },
  { "/Machine/_Reset",	        "F5" , gtkui_reset,         0, NULL          },
  { "/Machine/_Select...",      "F9" , gtkui_select,        0, NULL          },
  { "/Machine/_Debugger...",	NULL , gtkui_break,	    0, NULL          },
  { "/Machine/_NMI",		NULL , gtkui_nmi,	    0, NULL          },

  { "/Media",			NULL , NULL,                0, "<Branch>"    },

  { "/Media/_Tape",		NULL , NULL,                0, "<Branch>"    },
  { "/Media/Tape/_Open...",	"F7" , gtkui_tape_open,     0, NULL          },
  { "/Media/Tape/_Play",	"F8" , gtkui_tape_play,     0, NULL          },
  { "/Media/Tape/_Browse...",	NULL , gtk_tape_browse,     0, NULL          },
  { "/Media/Tape/_Rewind",	NULL , gtkui_tape_rewind,   0, NULL          },
  { "/Media/Tape/_Clear",	NULL , gtkui_tape_clear,    0, NULL          },
  { "/Media/Tape/_Write...",	"F6" , gtkui_tape_write,    0, NULL          },

  { "/Media/_Disk",		NULL , NULL,		    0, "<Branch>"    },
  { "/Media/Disk/Drive A:",	NULL , NULL,		    0, "<Branch>"    },
  { "/Media/Disk/Drive A:/_Insert...",
				NULL , gtkui_disk_open_a,   0, NULL          },
  { "/Media/Disk/Drive A:/_Eject",
				NULL , gtkui_disk_eject_a,  0, NULL          },
  { "/Media/Disk/Drive B:",	NULL , NULL,		    0, "<Branch>"    },
  { "/Media/Disk/Drive B:/_Insert...",
				NULL , gtkui_disk_open_b,   0, NULL          },
  { "/Media/Disk/Drive B:/_Eject",
				NULL , gtkui_disk_eject_b,  0, NULL          },

  { "/Media/_Cartridge",	NULL , NULL,		    0, "<Branch>"    },
  { "/Media/Cartridge/_Insert...",
				NULL , cartridge_insert,    0, NULL          },
  { "/Media/Cartridge/_Eject",	NULL , cartridge_eject,     0, NULL          },

  { "/Help",			NULL , NULL,		    0, "<Branch>"    },
  { "/Help/_Keyboard...",	NULL , gtkui_help_keyboard, 0, NULL	     },
};
static guint gtkui_menu_data_size =
  sizeof( gtkui_menu_data ) / sizeof( GtkItemFactoryEntry );
  
int
ui_init( int *argc, char ***argv )
{
  GtkWidget *box,*menu_bar;
  GtkAccelGroup *accel_group;
  GdkGeometry geometry;
  GdkWindowHints hints;

  gtk_init(argc,argv);

  gdk_rgb_init();
  gdk_rgb_set_install( TRUE );
  gtk_widget_set_default_colormap( gdk_rgb_get_cmap() );
  gtk_widget_set_default_visual( gdk_rgb_get_visual() );

  gtkui_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title( GTK_WINDOW(gtkui_window), "Fuse" );
  gtk_window_set_wmclass( GTK_WINDOW(gtkui_window), fuse_progname, "Fuse" );

  gtk_window_set_default_size( GTK_WINDOW(gtkui_window),
			       DISPLAY_ASPECT_WIDTH, DISPLAY_SCREEN_HEIGHT );

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
			 DISPLAY_ASPECT_WIDTH, DISPLAY_SCREEN_HEIGHT );

  gtk_box_pack_start( GTK_BOX(box), gtkui_drawing_area, FALSE, FALSE, 0 );

  hints = GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE |
          GDK_HINT_BASE_SIZE | GDK_HINT_RESIZE_INC;

  geometry.min_width = DISPLAY_ASPECT_WIDTH;
  geometry.min_height = DISPLAY_SCREEN_HEIGHT;
  geometry.max_width = 2 * DISPLAY_ASPECT_WIDTH;
  geometry.max_height = 2 * DISPLAY_SCREEN_HEIGHT;
  geometry.base_width = 0;
  geometry.base_height = 0;
  geometry.width_inc = DISPLAY_ASPECT_WIDTH;
  geometry.height_inc = DISPLAY_SCREEN_HEIGHT;

  if( settings_current.aspect_hint ) {
    hints |= GDK_HINT_ASPECT;
    geometry.min_aspect = geometry.max_aspect =
      ((float)DISPLAY_ASPECT_WIDTH)/DISPLAY_SCREEN_HEIGHT;
  }

  gtk_window_set_geometry_hints( GTK_WINDOW(gtkui_window), gtkui_drawing_area,
				 &geometry, hints );

  if( gtkdisplay_init() ) return 1;

  gtk_widget_show_all( gtkui_window );

  return 0;
}

static gboolean gtkui_make_menu(GtkAccelGroup **accel_group,
				GtkWidget **menu_bar,
				GtkItemFactoryEntry *menu_data,
				guint menu_data_size)
{
  *accel_group = gtk_accel_group_new();
  menu_factory = gtk_item_factory_new( GTK_TYPE_MENU_BAR, "<main>",
				       *accel_group );
  gtk_item_factory_create_items( menu_factory, menu_data_size, menu_data,
				 NULL);
  *menu_bar = gtk_item_factory_get_widget( menu_factory, "<main>" );

  /* We have to recreate the menus for the popup, unfortunately... */
  popup_factory = gtk_item_factory_new( GTK_TYPE_MENU, "<main>", NULL );
  gtk_item_factory_create_items( popup_factory, menu_data_size, menu_data,
				 NULL);
  gtkui_menu_popup = gtk_item_factory_get_widget( popup_factory, "<main>" );

  /* Start the recording menu off in the 'not playing' state */
  ui_menu_activate_recording( 0 );

  /* Start the AY logging menu off in the 'not playing' state */
  ui_menu_activate_ay_logging( 0 );

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
  error = gtkdisplay_end(); if( error ) return error;

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

  filename = gtkui_fileselector_get_filename( "Fuse - Open Spectrum File" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  utils_open_file( filename, settings_current.auto_load, NULL );

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

  rzx_start_recording( recording, 0 );

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

  ui_menu_activate_recording( 1 );

  fuse_emulation_unpause();
}

/* Called when File/AY Logging/Record selected */
static void
gtkui_psg_start( GtkWidget *widget, gpointer data )
{
  char *psgfile;

  if( psg_recording ) return;

  fuse_emulation_pause();

  psgfile = gtkui_fileselector_get_filename( "Fuse - Start AY log" );
  if ( !psgfile ) { fuse_emulation_unpause(); return; }

  psg_start_recording( psgfile );

  free( psgfile );

  display_refresh_all();

  ui_menu_activate_ay_logging( 1 );

  fuse_emulation_unpause();
}

/* Called when File/AY Logging/Stop selected */
static void
gtkui_psg_stop( GtkWidget *widget, gpointer data )
{
  if ( !psg_recording ) return;
  psg_stop_recording();

  ui_menu_activate_ay_logging( 0 );

  return;
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

/* File/Open SCR Screenshot */
static void
gtkui_open_scr( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  char *filename;

  fuse_emulation_pause();

  filename =
    gtkui_fileselector_get_filename( "Fuse - Open SCR Screenshot" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  screenshot_scr_read( filename );

  free( filename );

  fuse_emulation_unpause();
}

/* File/Save Screenshot as SCR */
static void
gtkui_save_scr( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  char *filename;

  fuse_emulation_pause();

  filename =
    gtkui_fileselector_get_filename( "Fuse - Save Screenshot as SCR" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  screenshot_scr_write( filename );

  free( filename );

  fuse_emulation_unpause();
}

#ifdef USE_LIBPNG
/* File/Save Screenshot as PNG */
static void
gtkui_save_screen( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  scaler_type scaler;
  char *filename;

  fuse_emulation_pause();

  scaler = ui_get_scaler( screenshot_available_scalers );
  if( scaler == SCALER_NUM ) {
    fuse_emulation_unpause();
    return;
  }

  filename =
    gtkui_fileselector_get_filename( "Fuse - Save Screenshot as PNG" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  screenshot_save();
  screenshot_write( filename, scaler );

  free( filename );

  fuse_emulation_unpause();
}
#endif				/* #ifdef USE_LIBPNG */

/* Called by the menu when File/Exit selected */
static void
gtkui_quit( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  fuse_exiting=1;
}

/* Called by the menu when Options/Filter selected */
static void
select_filter( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  scaler_type scaler;

  /* Stop emulation */
  fuse_emulation_pause();

  scaler = ui_get_scaler( scaler_is_supported );
  if( scaler != SCALER_NUM && scaler != current_scaler )
    scaler_select_scaler( scaler );

  /* Carry on with emulation again */
  fuse_emulation_unpause();
}

/* Select a graphics filter from those for which `available' returns
   true */
scaler_type
ui_get_scaler( ui_scaler_available available )
{
  gtkui_select_info dialog;
  GtkAccelGroup *accel_group;
  GSList *button_group = NULL;

  GtkWidget *ok_button, *cancel_button;

  int count;
  scaler_type scaler;

  /* Store the function which tells us which scalers are currently
     available */
  dialog.available = available;

  /* No scaler currently selected */
  dialog.selected = SCALER_NUM;
  
  /* Some space to store the radio buttons in */
  dialog.buttons = (GtkWidget**)malloc( SCALER_NUM * sizeof(GtkWidget* ) );
  if( dialog.buttons == NULL ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return SCALER_NUM;
  }

  count = 0;

  /* Create the necessary widgets */
  dialog.dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( dialog.dialog ), "Fuse - Select Scaler" );

  for( scaler = 0; scaler < SCALER_NUM; scaler++ ) {

    if( !available( scaler ) ) continue;

    dialog.buttons[ count ] =
      gtk_radio_button_new_with_label( button_group, scaler_name( scaler ) );
    button_group =
      gtk_radio_button_group( GTK_RADIO_BUTTON( dialog.buttons[ count ] ) );

    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( dialog.buttons[ count ] ),
				  current_scaler == scaler );

    gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog.dialog )->vbox ),
		       dialog.buttons[ count ] );

    count++;
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
		      GTK_SIGNAL_FUNC( select_filter_done ),
		      (gpointer) &dialog );
  gtk_signal_connect_object( GTK_OBJECT( cancel_button ), "clicked",
			     GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
			     GTK_OBJECT( dialog.dialog ) );
  gtk_signal_connect( GTK_OBJECT( dialog.dialog ), "delete_event",
		      GTK_SIGNAL_FUNC( gtkui_destroy_widget_and_quit ),
		      (gpointer) NULL );

  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group( GTK_WINDOW( dialog.dialog ), accel_group);

  /* Allow Esc to cancel */
  gtk_widget_add_accelerator( cancel_button, "clicked",
                              accel_group,
                              GDK_Escape, 0, 0 );

  /* Set the window to be modal and display it */
  gtk_window_set_modal( GTK_WINDOW( dialog.dialog ), TRUE );
  gtk_widget_show_all( dialog.dialog );

  /* Process events until the window is done with */
  gtk_main();

  return dialog.selected;
}

/* Callback used by the filter selection dialog */
static void
select_filter_done( GtkWidget *widget GCC_UNUSED, gpointer user_data )
{
  int i, count;
  gtkui_select_info *ptr = (gtkui_select_info*)user_data;

  count = 0;

  for( i = 0; i < SCALER_NUM; i++ ) {

    if( !ptr->available( i ) ) continue;

    if( gtk_toggle_button_get_active(
	  GTK_TOGGLE_BUTTON( ptr->buttons[ count ] )
	)
      ) {
      ptr->selected = i;
    }

    count++;
  }

  gtk_widget_destroy( ptr->dialog );
  gtk_main_quit();
}

#ifdef HAVE_LIB_XML2
/* Options/Save */
static void
save_options( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  settings_write_config( &settings_current );
}
#endif				/* #ifdef HAVE_LIB_XML2 */

/* Called by the menu when Machine/Reset selected */
static void
gtkui_reset( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  if( machine_reset() ) {
    ui_error( UI_ERROR_ERROR, "couldn't reset machine: giving up!" );

    /* FIXME: abort() seems a bit extreme here, but it'll do for now */
    fuse_abort();
  }
}

/* Called by the menu when Machine/Select selected */
static void
gtkui_select( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  gtkui_select_info dialog;
  GtkAccelGroup *accel_group;

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

  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(dialog.dialog), accel_group);

  /* Allow Esc to cancel */
  gtk_widget_add_accelerator( cancel_button, "clicked",
                              accel_group,
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

/* Machine/Break */
static void
gtkui_break( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  debugger_mode = DEBUGGER_MODE_HALTED;
}

/* Machine/NMI */
static void
gtkui_nmi( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  /* Add an NMI to go off as soon as possible */
  if( event_add( 0, EVENT_TYPE_NMI ) ) return;
}
    
/* Called by the menu when Tape/Open selected */
static void
gtkui_tape_open( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  char *filename;

  fuse_emulation_pause();

  filename = gtkui_fileselector_get_filename( "Fuse - Open Tape" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  tape_open_default_autoload( filename );

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

#ifdef HAVE_765_H
  if( machine_current->machine == LIBSPECTRUM_MACHINE_PLUS3 ) {
    specplus3_disk_insert( drive, filename );
  } else
#endif				/* #ifdef HAVE_765_H */
    trdos_disk_insert( drive, filename );

  free( filename );

  fuse_emulation_unpause();
}

static void
gtkui_disk_eject_a( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
#ifdef HAVE_765_H
  if( machine_current->machine == LIBSPECTRUM_MACHINE_PLUS3 ) {
    specplus3_disk_eject( SPECPLUS3_DRIVE_A );
    return;
  }
#endif				/* #ifdef HAVE_765_H */

  trdos_disk_eject( TRDOS_DRIVE_A );
}

static void
gtkui_disk_eject_b( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
#ifdef HAVE_765_H
  if( machine_current->machine == LIBSPECTRUM_MACHINE_PLUS3 ) {
    specplus3_disk_eject( SPECPLUS3_DRIVE_B );
    return;
  }
#endif				/* #ifdef HAVE_765_H */

  trdos_disk_eject( TRDOS_DRIVE_B );
}

/* Cartridge/Insert */
static void
cartridge_insert( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  char *filename;

  fuse_emulation_pause();

  filename = gtkui_fileselector_get_filename( "Fuse - Insert Cartridge" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  dck_insert( filename );

  free( filename );

  fuse_emulation_unpause();
}

/* Cartridge/Eject */
static void
cartridge_eject( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  dck_eject();
}

static void
gtkui_help_keyboard( GtkWidget *widget GCC_UNUSED, gpointer data GCC_UNUSED )
{
  gtkui_picture( "keyboard.scr", 0 );
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

char*
gtkui_fileselector_get_filename( const char *title )
{
  gtkui_fileselector_info selector;
  GtkAccelGroup *accel_group;

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

  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(selector.selector), accel_group);

  /* Allow Esc to cancel */
  gtk_widget_add_accelerator(
       GTK_FILE_SELECTION( selector.selector )->cancel_button,
       "clicked",
       accel_group,
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
  const char *filename;

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

/* Functions to activate and deactivate certain menu items */

static int
set_menu_item_active( const char *path, int active )
{
  GtkWidget *menu_item;

  menu_item = gtk_item_factory_get_widget( menu_factory, path );
  if( !menu_item ) {
    ui_error( UI_ERROR_ERROR, "couldn't get menu item '%s' from menu_factory",
	      path );
    return 1;
  }
  gtk_widget_set_sensitive( menu_item, active );

  menu_item = gtk_item_factory_get_widget( popup_factory, path );
  if( !menu_item ) {
    ui_error( UI_ERROR_ERROR, "couldn't get menu item '%s' from popup_factory",
	      path );
    return 1;
  }
  gtk_widget_set_sensitive( menu_item, active );

  return 0;
}

int
ui_menu_activate_media_cartridge( int active )
{
  return set_menu_item_active( "/Media/Cartridge", active );
}

int
ui_menu_activate_media_cartridge_eject( int active )
{
  return set_menu_item_active( "/Media/Cartridge/Eject", active );
}

int
ui_menu_activate_media_disk( int active )
{
  return set_menu_item_active( "/Media/Disk", active );
}

int
ui_menu_activate_media_disk_eject( int which, int active )
{
  if( which == 0 ) {
    return set_menu_item_active( "/Media/Disk/Drive A:/Eject", active );
  } else {
    return set_menu_item_active( "/Media/Disk/Drive B:/Eject", active );
  }
}

int
ui_menu_activate_recording( int active )
{
  int error;

  error = set_menu_item_active( "/File/Recording/Record...", !active );
  if( error ) return error;

  error = set_menu_item_active( "/File/Recording/Record from snapshot...",
				!active );
  if( error ) return error;

  error = set_menu_item_active( "/File/Recording/Play...", !active );
  if( error ) return error;

  return set_menu_item_active( "/File/Recording/Stop", active );
}

int
ui_menu_activate_ay_logging( int active )
{
  int error;

  error = set_menu_item_active( "/File/AY Logging/Record...", !active );
  if( error ) return error;

  return set_menu_item_active( "/File/AY Logging/Stop", active );
}

#endif			/* #ifdef UI_GTK */
