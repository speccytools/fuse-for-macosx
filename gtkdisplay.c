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

   E-mail: pak@ast.cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#ifdef UI_GTK		/* Use this file iff we're using GTK+ */

#include <stdio.h>

#include <gtk/gtk.h>

#include "display.h"
#include "fuse.h"
#include "gtkdisplay.h"
#include "gtkui.h"

static GdkGC *gc;
static GdkImage *image;

static unsigned long colours[16];

/* The current size of the window (in units of DISPLAY_SCREEN_*) */
static int gtkdisplay_current_size=1;

static int gtkdisplay_allocate_colours(int numColours, unsigned long *colours);

/* Callbacks */

static gint gtkdisplay_expose(GtkWidget *widget, GdkEvent *event,
			      gpointer data);
static gint gtkdisplay_configure(GtkWidget *widget, GdkEvent *event,
				 gpointer data);

int gtkdisplay_init(int width, int height)
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
  gc = gtk_gc_get(depth, gtk_widget_get_colormap(gtkui_drawing_area),
		  &gc_values, (GdkGCValuesMask) 0 );

  if(gtkdisplay_allocate_colours(16,colours)) return 1;

  return 0;
}

static int gtkdisplay_allocate_colours(int numColours, unsigned long *colours)
{
  GdkColor gdk_colours[16];
  GdkColormap *current_map;
  gboolean success[16];

  char *colour_names[] = {
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

  int i;

  current_map = gtk_widget_get_colormap( gtkui_drawing_area );

  for(i=0;i<16;i++) {
    if( ! gdk_color_parse(colour_names[i],&gdk_colours[i]) ) {
      fprintf(stderr,"%s: couldn't parse colour `%s'\n",
	      fuse_progname, colour_names[i]);
      return 1;
    }
  }

  if( gdk_colormap_alloc_colors( current_map, gdk_colours, 16, FALSE, FALSE,
				 success ) != 16 ) {
    fprintf(stderr,"%s: Couldn't allocate all colours in default colourmap\n"
	    "%s: switching to private colourmap\n",
	    fuse_progname,fuse_progname);
    /* FIXME: should free colours in default map here */
    current_map = gdk_colormap_new( gdk_visual_get_system(), FALSE );
    if( gdk_colormap_alloc_colors( current_map, gdk_colours, 16, FALSE, FALSE,
				   success ) != 16 ) {
      fprintf(stderr,"%s: Still couldn't allocate all colours\n",
	      fuse_progname);
      return 1;
    }
    gtk_widget_set_colormap( gtkui_window, current_map );
  }

  for(i=0;i<16;i++) colours[i]=gdk_colours[i].pixel;

  return 0;

}
  
int gtkdisplay_configure_notify(int width, int height)
{
  int y,size;

  size = width  / DISPLAY_SCREEN_WIDTH;

  /* If we're the same size as before, nothing special needed */
  if( size == gtkdisplay_current_size ) return 0;

  /* Else set ourselves to the new height */
  gtkdisplay_current_size=size;
  gtk_drawing_area_size( GTK_DRAWING_AREA(gtkui_drawing_area),
			 size * DISPLAY_SCREEN_WIDTH,
			 size * DISPLAY_SCREEN_HEIGHT );

  /* Redraw the entire screen... */
  display_refresh_all();

  /* And the entire border */
  for(y=0;y<DISPLAY_BORDER_HEIGHT;y++) {
    gtkdisplay_set_border(y,0,DISPLAY_SCREEN_WIDTH,display_border);
    gtkdisplay_set_border(DISPLAY_BORDER_HEIGHT+DISPLAY_HEIGHT+y,0,
			  DISPLAY_SCREEN_WIDTH,display_border);
  }

  for(y=DISPLAY_BORDER_HEIGHT;y<DISPLAY_BORDER_HEIGHT+DISPLAY_HEIGHT;y++) {
    gtkdisplay_set_border(y,0,DISPLAY_BORDER_WIDTH,display_border);
    gtkdisplay_set_border(y,DISPLAY_BORDER_WIDTH+DISPLAY_WIDTH,
			  DISPLAY_SCREEN_WIDTH,display_border);
  }

  return 0;
}

void gtkdisplay_putpixel(int x,int y,int colour)
{
  switch(gtkdisplay_current_size) {
  case 1:
    gdk_image_put_pixel(image,  x  ,  y  ,colours[colour]);
    break;
  case 2:
    gdk_image_put_pixel(image,2*x,  2*y  ,colours[colour]);
    gdk_image_put_pixel(image,2*x+1,2*y  ,colours[colour]);
    gdk_image_put_pixel(image,2*x  ,2*y+1,colours[colour]);
    gdk_image_put_pixel(image,2*x+1,2*y+1,colours[colour]);
    break;
  case 3:
    gdk_image_put_pixel(image,3*x,  3*y  ,colours[colour]);
    gdk_image_put_pixel(image,3*x+1,3*y  ,colours[colour]);
    gdk_image_put_pixel(image,3*x+2,3*y  ,colours[colour]);
    gdk_image_put_pixel(image,3*x  ,3*y+1,colours[colour]);
    gdk_image_put_pixel(image,3*x+1,3*y+1,colours[colour]);
    gdk_image_put_pixel(image,3*x+2,3*y+1,colours[colour]);
    gdk_image_put_pixel(image,3*x  ,3*y+2,colours[colour]);
    gdk_image_put_pixel(image,3*x+1,3*y+2,colours[colour]);
    gdk_image_put_pixel(image,3*x+2,3*y+2,colours[colour]);
    break;
  }
}

void gtkdisplay_line(int y)
{
  gdk_draw_image(gtkui_drawing_area->window, gc, image,
		 0,gtkdisplay_current_size*y,
		 0,gtkdisplay_current_size*y,
		 gtkdisplay_current_size*DISPLAY_SCREEN_WIDTH,
		 gtkdisplay_current_size);
}

void gtkdisplay_area(int x, int y, int width, int height)
{
  gdk_draw_image(gtkui_drawing_area->window,gc,image,x,y,x,y,width,height);
}

void gtkdisplay_set_border(int line, int pixel_from, int pixel_to, int colour)
{
  int x;
  
  switch(gtkdisplay_current_size) {
  case 1:
    for(x=pixel_from;x<pixel_to;x++) {
      gdk_image_put_pixel(image,  x  ,  line  ,colours[colour]);
    }
    break;
  case 2:
    for(x=pixel_from;x<pixel_to;x++) {
      gdk_image_put_pixel(image,2*x  ,2*line  ,colours[colour]);
      gdk_image_put_pixel(image,2*x+1,2*line  ,colours[colour]);
      gdk_image_put_pixel(image,2*x  ,2*line+1,colours[colour]);
      gdk_image_put_pixel(image,2*x+1,2*line+1,colours[colour]);
    }
    break;
  case 3:
    for(x=pixel_from;x<pixel_to;x++) {
      gdk_image_put_pixel(image,3*x  ,3*line  ,colours[colour]);
      gdk_image_put_pixel(image,3*x+1,3*line  ,colours[colour]);
      gdk_image_put_pixel(image,3*x+2,3*line  ,colours[colour]);
      gdk_image_put_pixel(image,3*x  ,3*line+1,colours[colour]);
      gdk_image_put_pixel(image,3*x+1,3*line+1,colours[colour]);
      gdk_image_put_pixel(image,3*x+2,3*line+1,colours[colour]);
      gdk_image_put_pixel(image,3*x  ,3*line+2,colours[colour]);
      gdk_image_put_pixel(image,3*x+1,3*line+2,colours[colour]);
      gdk_image_put_pixel(image,3*x+2,3*line+2,colours[colour]);
    }
    break;
  }
}

int gtkdisplay_end(void)
{
  /* Free the XImage used to store screen data; also frees the malloc'd
     data */
/*    XDestroyImage(image); */

  /* Free the allocated GC */
  gtk_gc_release(gc);

  return 0;
}

/* Callbacks */

/* Called by gtkui_drawing_area on "expose_event" */
static gint gtkdisplay_expose(GtkWidget *widget, GdkEvent *event,
			      gpointer data)
{
  gtkdisplay_area(event->expose.area.x, event->expose.area.y,
		  event->expose.area.width, event->expose.area.height);
  return TRUE;
}

/* Called by gtkui_drawing_area on "configure_event" */
static gint gtkdisplay_configure(GtkWidget *widget, GdkEvent *event,
				 gpointer data)
{
  gtkdisplay_configure_notify(event->configure.width, event->configure.height);
  return FALSE;
}

#endif			/* #ifdef UI_GTK */
