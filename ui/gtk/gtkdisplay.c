/* gtkdisplay.c: GTK+ routines for dealing with the Speccy screen
   Copyright (c) 2000 Philip Kendall

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

#include <gtk/gtk.h>

#include "display.h"
#include "fuse.h"
#include "gtkui.h"
#include "screenshot.h"
#include "ui/uidisplay.h"
#include "scld.h"

GdkGC *gtkdisplay_gc;
static GdkImage *image;

unsigned long gtkdisplay_colours[16];

/* The current size of the window (in units of DISPLAY_SCREEN_*) */
static int gtkdisplay_current_size=1;

static int gtkdisplay_allocate_colours( unsigned long *pixel_values );
static void gtkdisplay_area(int x, int y, int width, int height);
static int gtkdisplay_configure_notify( int width );

/* Callbacks */

static gint gtkdisplay_expose(GtkWidget *widget, GdkEvent *event,
			      gpointer data);
static gint gtkdisplay_configure(GtkWidget *widget, GdkEvent *event,
				 gpointer data);

int uidisplay_init(int width, int height)
{
  int x,y,get_width,get_height, depth;
  GdkGCValues gc_values;

  image = gdk_image_new(GDK_IMAGE_FASTEST, gdk_visual_get_system(),
			3*width, 3*height );

  gtk_signal_connect( GTK_OBJECT(gtkui_drawing_area), "expose_event", 
		      GTK_SIGNAL_FUNC(gtkdisplay_expose), NULL);
  gtk_signal_connect( GTK_OBJECT(gtkui_drawing_area), "configure_event", 
		      GTK_SIGNAL_FUNC(gtkdisplay_configure), NULL);

  gdk_window_get_geometry( gtkui_drawing_area->window, &x, &y,
			   &get_width, &get_height, &depth );
  gtkdisplay_gc =
    gtk_gc_get( depth, gtk_widget_get_colormap( gtkui_drawing_area ),
		&gc_values, (GdkGCValuesMask) 0 );

  if( gtkdisplay_allocate_colours( gtkdisplay_colours ) ) return 1;

  return 0;
}

static int gtkdisplay_allocate_colours( unsigned long *pixel_values )
{
  GdkColor gdk_colours[16];
  GdkColormap *current_map;
  gboolean success[16];

  const char *colour_names[] = {
    "black",
    "blue3",
    "red3",
    "magenta3",
    "green3",
    "cyan3",
    "yellow3",
    "gray80",
    "black",
    "blue",
    "red",
    "magenta",
    "green",
    "cyan",
    "yellow",
    "white",
  };

  int i,ok;

  current_map = gtk_widget_get_colormap( gtkui_drawing_area );

  for(i=0;i<16;i++) {
    if( ! gdk_color_parse(colour_names[i],&gdk_colours[i]) ) {
      fprintf(stderr,"%s: couldn't parse colour `%s'\n",
	      fuse_progname, colour_names[i]);
      return 1;
    }
  }

  gdk_colormap_alloc_colors( current_map, gdk_colours, 16, FALSE, FALSE,
			     success );
  for(i=0,ok=1;i<16;i++) if(!success[i]) ok=0;

  if(!ok) {
    fprintf(stderr,"%s: couldn't allocate all colours in default colourmap\n"
	    "%s: switching to private colourmap\n",
	    fuse_progname,fuse_progname);
    /* FIXME: should free colours in default map here */
    current_map = gdk_colormap_new( gdk_visual_get_system(), FALSE );
    gdk_colormap_alloc_colors( current_map, gdk_colours, 16, FALSE, FALSE,
			       success );
    for(i=0,ok=1;i<16;i++) if(!success[i]) ok=0;
    if( !ok ) {
      fprintf(stderr,"%s: still couldn't allocate all colours\n",
	      fuse_progname);
      return 1;
    }
    gtk_widget_set_colormap( gtkui_window, current_map );
  }

  for(i=0;i<16;i++) pixel_values[i] = gdk_colours[i].pixel;

  return 0;

}
  
static int gtkdisplay_configure_notify( int width )
{
  int size, colour;

  colour = scld_last_dec.name.hires ? display_hires_border :
                                      display_lores_border;
  size = width / DISPLAY_ASPECT_WIDTH;

  /* If we're the same size as before, nothing special needed */
  if( size == gtkdisplay_current_size ) return 0;

  /* Else set ourselves to the new height */
  gtkdisplay_current_size=size;
  gtk_drawing_area_size( GTK_DRAWING_AREA(gtkui_drawing_area),
			 size * DISPLAY_ASPECT_WIDTH,
			 size * DISPLAY_SCREEN_HEIGHT );

  /* Redraw the entire screen... */
  display_refresh_all();

  return 0;
}

void uidisplay_putpixel(int x,int y,int colour)
{
#ifdef USE_LIBPNG
  screenshot_screen[y][x] = colour;
#endif			/* #ifdef USE_LIBPNG */

  switch(gtkdisplay_current_size) {
  case 1:
    if(x%2!=0) return;
    gdk_image_put_pixel( image, x>>1, y, gtkdisplay_colours[colour] );
    break;
  case 2:
    gdk_image_put_pixel( image,x,  y<<1   , gtkdisplay_colours[colour] );
    gdk_image_put_pixel( image,x, (y<<1)+1, gtkdisplay_colours[colour] );
    break;
  }
}

void
uidisplay_frame_end( void ) 
{
  return;
}

void
uidisplay_area( int x, int y, int w, int h )
{
  gtkdisplay_area( gtkdisplay_current_size * x,
		   gtkdisplay_current_size * y,
		   gtkdisplay_current_size * w,
		   gtkdisplay_current_size * h );
}

static void gtkdisplay_area(int x, int y, int width, int height)
{
  gdk_draw_image( gtkui_drawing_area->window, gtkdisplay_gc, image, x, y, x, y,
		  width, height );
}

int uidisplay_end(void)
{
  /* Free the XImage used to store screen data; also frees the malloc'd
     data */
/*    XDestroyImage(image); */

  /* Free the allocated GC */
  gtk_gc_release( gtkdisplay_gc );

  return 0;
}

/* Callbacks */

/* Called by gtkui_drawing_area on "expose_event" */
static gint
gtkdisplay_expose( GtkWidget *widget GCC_UNUSED, GdkEvent *event,
		   gpointer data GCC_UNUSED )
{
  gtkdisplay_area(event->expose.area.x, event->expose.area.y,
		  event->expose.area.width, event->expose.area.height);
  return TRUE;
}

/* Called by gtkui_drawing_area on "configure_event" */
static gint
gtkdisplay_configure( GtkWidget *widget GCC_UNUSED, GdkEvent *event,
		      gpointer data GCC_UNUSED )
{
  gtkdisplay_configure_notify( event->configure.width );
  return FALSE;
}

#endif			/* #ifdef UI_GTK */
