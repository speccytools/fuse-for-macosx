/* picture.c: GTK+ routines to draw the keyboard picture
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

#ifdef UI_GTK		/* Use this file iff we're using GTK+ */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <gtk/gtk.h>

#include "display.h"
#include "fuse.h"
#include "gtkui.h"
#include "types.h"
#include "ui/ui.h"
#include "utils.h"

static gint
picture_expose( GtkWidget *widget, GdkEvent *event, gpointer data );

struct picture_data {
  GtkWidget *drawing_area;
  GdkImage *image;
};

int
gtkui_picture( const char *filename, int border )
{
  int fd, error, i, x, y;
  BYTE attr, ink, paper, data;

  BYTE *screen; size_t length;

  GtkWidget *dialog;
  GtkWidget *ok_button;
  struct picture_data *callback_data;

  fuse_emulation_pause();

  fd = utils_find_lib( filename );
  if( fd == -1 ) {
    ui_error( UI_ERROR_ERROR, "couldn't find keyboard picture ('%s')",
	      filename );
    fuse_emulation_unpause();
    return 1;
  }
  
  error = utils_read_fd( fd, filename, &screen, &length );
  if( error ) return error;

  if( length != 6912 ) {
    munmap( screen, length );
    ui_error( UI_ERROR_ERROR, "keyboard picture ('%s') is not 6912 bytes long",
	      filename );
    fuse_emulation_unpause();
    return 1;
  }

  dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( dialog ), "Fuse - Keyboard" );
  gtk_widget_set_colormap( dialog, gtk_widget_get_colormap( gtkui_window ) );

  callback_data =
    (struct picture_data*)malloc( sizeof( struct picture_data ) );
  if( !callback_data ) {
    ui_error( UI_ERROR_ERROR, "out of memory in gtkui_picture" );
    fuse_emulation_unpause();
    return 1;
  }

  callback_data->drawing_area = gtk_drawing_area_new();
  gtk_drawing_area_size( GTK_DRAWING_AREA( callback_data->drawing_area ),
			 DISPLAY_ASPECT_WIDTH, DISPLAY_SCREEN_HEIGHT );
  gtk_signal_connect( GTK_OBJECT( callback_data->drawing_area ),
		      "expose_event", GTK_SIGNAL_FUNC( picture_expose ),
		      callback_data );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->vbox ),
		     callback_data->drawing_area );

  callback_data->image =
    gdk_image_new( GDK_IMAGE_FASTEST, gdk_visual_get_system(),
		   DISPLAY_ASPECT_WIDTH, DISPLAY_SCREEN_HEIGHT );

  for( y=0; y < DISPLAY_BORDER_HEIGHT; y++ ) {
    for( x=0; x < DISPLAY_ASPECT_WIDTH; x ++ ) {
      gdk_image_put_pixel( callback_data->image, x, y, 
			   gtkdisplay_colours[ border ] );
      gdk_image_put_pixel( callback_data->image,
			   x, y + DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT,
			   gtkdisplay_colours[ border ] );
    }
  }

  for( y=0; y<DISPLAY_HEIGHT; y++ ) {

    for( x=0; x < DISPLAY_BORDER_WIDTH; x+=2 ) {
      gdk_image_put_pixel(
        callback_data->image, x >> 1, y + DISPLAY_BORDER_HEIGHT,
	gtkdisplay_colours[ border ]
      );
      gdk_image_put_pixel(
        callback_data->image,
	( x + DISPLAY_BORDER_WIDTH + DISPLAY_WIDTH ) >> 1,
	y + DISPLAY_BORDER_HEIGHT, gtkdisplay_colours[ border ]
      );
    }

    for( x=0; x < DISPLAY_WIDTH_COLS; x++ ) {

      attr = screen[ display_attr_start[y] + x ];

      ink = ( attr & 0x07 ) + ( ( attr & 0x40 ) >> 3 );
      paper = ( attr & ( 0x0f << 3 ) ) >> 3;

      data = screen[ display_line_start[y]+x ];

      for( i=0; i<8; i++ ) {
	gdk_image_put_pixel( callback_data->image,
			     ( DISPLAY_BORDER_WIDTH >> 1 ) + ( 8 * x ) + i,
			     y + DISPLAY_BORDER_HEIGHT,
			     ( data & 0x80 ) ? gtkdisplay_colours[ ink ]
                                             : gtkdisplay_colours[ paper ] );
	data <<= 1;
      }
    }

  }

  ok_button = gtk_button_new_with_label( "OK" );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->action_area ),
		     ok_button );

  gtk_signal_connect_object( GTK_OBJECT( ok_button ), "clicked",
			     GTK_SIGNAL_FUNC( gtk_widget_destroy ),
			     GTK_OBJECT( dialog ) );

  gtk_widget_show_all( dialog );

  if( munmap( screen, length ) == -1 ) {
    ui_error( UI_ERROR_ERROR, "Couldn't munmap keyboard picture ('%s'): %s",
	      filename, strerror( errno ) );
    fuse_emulation_unpause();
    return 1;
  }

  fuse_emulation_unpause();

  return 0;
}

static gint
picture_expose( GtkWidget *widget, GdkEvent *event, gpointer data )
{
  struct picture_data *ptr = (struct picture_data*)data;

  gint x = event->expose.area.x,
    y = event->expose.area.y,
    width = event->expose.area.width,
    height = event->expose.area.height;

  gdk_draw_image( ptr->drawing_area->window, gtkdisplay_gc, ptr->image, x, y,
		  x, y, width, height );

  return TRUE;
}

#endif			/* #ifdef UI_GTK */
