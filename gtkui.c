/* gtkui.c: GTK+ routines for dealing with the user interface
   Copyright (c) 2000-2001 Philip Kendall

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

#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "display.h"
#include "fuse.h"
#include "gtkkeyboard.h"
#include "machine.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"
#include "ui.h"
#include "uidisplay.h"

/* The main Fuse window */
GtkWidget *gtkui_window;

/* The area into which the screen will be drawn */
GtkWidget *gtkui_drawing_area;

static gboolean gtkui_make_menu(GtkAccelGroup **accel_group,
				GtkWidget **menu_bar,
				GtkItemFactoryEntry *menu_data,
				guint menu_data_size);

static gint gtkui_delete(GtkWidget *widget, GdkEvent *event,
			      gpointer data);
static void gtkui_open(GtkWidget *widget, gpointer data);
static void gtkui_save(GtkWidget *widget, gpointer data);
static void gtkui_tape(GtkWidget *widget, gpointer data);
static void gtkui_quit(GtkWidget *widget, gpointer data);
static void gtkui_reset(GtkWidget *widget, gpointer data);
static void gtkui_switch(GtkWidget *widget, gpointer data);

static GtkItemFactoryEntry gtkui_menu_data[] = {
  { "/File",		     NULL , NULL,         0, "<Branch>"    },
  { "/File/_Open Snapshot" , "F3" , gtkui_open,   0, NULL          },
  { "/File/_Save Snapshot" , "F2" , gtkui_save,   0, NULL          },
  { "/File/separator1",      NULL , NULL,         0, "<Separator>" },
  { "/File/Open _Tape File", "F7" , gtkui_tape,   0, NULL          },
  { "/File/separator2",      NULL , NULL,         0, "<Separator>" },
  { "/File/E_xit",	     "F10", gtkui_quit,   0, NULL          },
  { "/Machine",		     NULL , NULL,         0, "<Branch>"    },
  { "/Machine/_Reset",	     "F5" , gtkui_reset,  0, NULL          },
  { "/Machine/_Switch",      "F9" , gtkui_switch, 0, NULL          },
};
static guint gtkui_menu_data_size = 10;
  
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
  geometry.max_width = 3*width;
  geometry.max_height = 3*height;
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

  return FALSE;
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
static gint gtkui_delete(GtkWidget *widget, GdkEvent *event,
			 gpointer data)
{
  fuse_exiting=1;
  return TRUE;
}

/* Called by the menu when File/Open selected */
static void gtkui_open(GtkWidget *widget, gpointer data)
{
  snapshot_read( "snapshot.z80" );
  display_refresh_all();
}

/* Called by the menu when File/Save selected */
static void gtkui_save(GtkWidget *widget, gpointer data)
{
  snapshot_write( "snapshot.z80" );
}

/* Called by the menu when File/Tape selected */
static void gtkui_tape(GtkWidget *widget, gpointer data)
{
  tape_open();
}

/* Called by the menu when File/Exit selected */
static void gtkui_quit(GtkWidget *widget, gpointer data)
{
  fuse_exiting=1;
}

/* Called by the menu when Machine/Reset selected */
static void gtkui_reset(GtkWidget *widget, gpointer data)
{
  machine_current->reset();
}

/* Called by the menu when Machine/Switch selected */
static void gtkui_switch(GtkWidget *widget, gpointer data)
{
  machine_select_next();
}

#endif			/* #ifdef UI_GTK */
