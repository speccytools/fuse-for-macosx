/* gtkdisplay.c: GTK+ routines for dealing with the Speccy screen
   Copyright (c) 2000-2005 Philip Kendall

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

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "display.h"
#include "fuse.h"
#include "gtkinternals.h"
#include "screenshot.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "ui/scaler/scaler.h"
#include "scld.h"
#include "settings.h"

/* The size of a 1x1 image in units of
   DISPLAY_ASPECT WIDTH x DISPLAY_SCREEN_HEIGHT */
int image_scale;

/* The height and width of a 1x1 image in pixels */
int image_width, image_height;

/* A copy of every pixel on the screen, replaceable by plotting directly into
   rgb_image below */
libspectrum_word
  gtkdisplay_image[ 2 * DISPLAY_SCREEN_HEIGHT ][ DISPLAY_SCREEN_WIDTH ];
ptrdiff_t gtkdisplay_pitch = DISPLAY_SCREEN_WIDTH * sizeof( libspectrum_word );

/* An RGB image of the Spectrum screen; slightly bigger than the real
   screen to handle the smoothing filters which read around each pixel */
static guchar rgb_image[ 4 * 2 * ( DISPLAY_SCREEN_HEIGHT + 4 ) *
		                 ( DISPLAY_SCREEN_WIDTH  + 3 )   ];
static const gint rgb_pitch = ( DISPLAY_SCREEN_WIDTH + 3 ) * 4;

/* The scaled image */
static guchar scaled_image[ 4 * 3 * DISPLAY_SCREEN_HEIGHT *
			    (size_t)(1.5 * DISPLAY_SCREEN_WIDTH) ];
static const ptrdiff_t scaled_pitch = 4 * 1.5 * DISPLAY_SCREEN_WIDTH;

/* The colour palette */
static const guchar rgb_colours[16][3] = {

  {   0,   0,   0 },
  {   0,   0, 192 },
  { 192,   0,   0 },
  { 192,   0, 192 },
  {   0, 192,   0 },
  {   0, 192, 192 },
  { 192, 192,   0 },
  { 192, 192, 192 },
  {   0,   0,   0 },
  {   0,   0, 255 },
  { 255,   0,   0 },
  { 255,   0, 255 },
  {   0, 255,   0 },
  {   0, 255, 255 },
  { 255, 255,   0 },
  { 255, 255, 255 },

};

/* And the colours (and black and white 'colours') in 32-bit format */
libspectrum_dword gtkdisplay_colours[16];
static libspectrum_dword bw_colours[16];

/* The current size of the window (in units of DISPLAY_SCREEN_*) */
static int gtkdisplay_current_size=1;

static int init_colours( void );
static void gtkdisplay_area(int x, int y, int width, int height);
static int register_scalers( void );

/* Callbacks */

static gint gtkdisplay_expose(GtkWidget *widget, GdkEvent *event,
			      gpointer data);
static gint drawing_area_resize_callback( GtkWidget *widget, GdkEvent *event,
					  gpointer data );

int
gtkdisplay_init( void )
{
  int x, y, error;
  libspectrum_dword black;

  gtk_signal_connect( GTK_OBJECT(gtkui_drawing_area), "expose_event", 
		      GTK_SIGNAL_FUNC(gtkdisplay_expose), NULL);
  gtk_signal_connect( GTK_OBJECT(gtkui_drawing_area), "configure_event", 
		      GTK_SIGNAL_FUNC( drawing_area_resize_callback ), NULL);

  error = init_colours(); if( error ) return error;

  black = settings_current.bw_tv ? bw_colours[0] : gtkdisplay_colours[0];

  for( y = 0; y < DISPLAY_SCREEN_HEIGHT + 4; y++ )
    for( x = 0; x < DISPLAY_SCREEN_WIDTH + 3; x++ )
      *(libspectrum_dword*)( rgb_image + y * rgb_pitch + 4 * x ) = black;

  display_ui_initialised = 1;

  return 0;
}

static int
init_colours( void )
{
  size_t i;

  for( i = 0; i < 16; i++ ) {


    guchar red, green, blue, grey;

    red   = rgb_colours[i][0];
    green = rgb_colours[i][1];
    blue  = rgb_colours[i][2];

    /* Addition of 0.5 is to avoid rounding errors */
    grey = ( 0.299 * red + 0.587 * green + 0.114 * blue ) + 0.5;

#ifdef WORDS_BIGENDIAN

    gtkdisplay_colours[i] =  red << 24 | green << 16 | blue << 8;
            bw_colours[i] = grey << 24 |  grey << 16 | grey << 8;

#else				/* #ifdef WORDS_BIGENDIAN */

    gtkdisplay_colours[i] =  red | green << 8 | blue << 16;
            bw_colours[i] = grey |  grey << 8 | grey << 16;

#endif				/* #ifdef WORDS_BIGENDIAN */

  }

  return 0;
}

int

uidisplay_init( int width, int height )
{
  int error;

  image_width = width; image_height = height;
  image_scale = width / DISPLAY_ASPECT_WIDTH;

  error = register_scalers(); if( error ) return error;

  display_refresh_all();

  return 0;
}

static int
drawing_area_resize( int width, int height )
{
  int size, error;

  size = width / DISPLAY_ASPECT_WIDTH;
  if( size > height / DISPLAY_SCREEN_HEIGHT )
    size = height / DISPLAY_SCREEN_HEIGHT;

  /* If we're the same size as before, no need to do anything else */
  if( size == gtkdisplay_current_size ) return 0;

  gtkdisplay_current_size = size;

  error = register_scalers(); if( error ) return error;

  memset( scaled_image, 0, sizeof( scaled_image ) );
  display_refresh_all();

  return 0;
}

static int
register_scalers( void )
{
  scaler_register_clear();

  switch( gtkdisplay_current_size ) {

  case 1:

    switch( image_scale ) {
    case 1:
      scaler_register( SCALER_NORMAL );
      scaler_register( SCALER_PALTV );
      if( !scaler_is_supported( current_scaler ) )
	scaler_select_scaler( SCALER_NORMAL );
      return 0;
    case 2:
      scaler_register( SCALER_HALF );
      scaler_register( SCALER_HALFSKIP );
      if( !scaler_is_supported( current_scaler ) )
	scaler_select_scaler( SCALER_HALF );
      return 0;
    }

  case 2:

    switch( image_scale ) {
    case 1:
      scaler_register( SCALER_DOUBLESIZE );
      scaler_register( SCALER_TV2X );
      scaler_register( SCALER_ADVMAME2X );
      scaler_register( SCALER_2XSAI );
      scaler_register( SCALER_SUPER2XSAI );
      scaler_register( SCALER_SUPEREAGLE );
      scaler_register( SCALER_DOTMATRIX );
      scaler_register( SCALER_PALTV2X );
      if( !scaler_is_supported( current_scaler ) )
	scaler_select_scaler( SCALER_DOUBLESIZE );
      return 0;
    case 2:
      scaler_register( SCALER_NORMAL );
      scaler_register( SCALER_TIMEXTV );
      scaler_register( SCALER_PALTV );
      if( !scaler_is_supported( current_scaler ) )
	scaler_select_scaler( SCALER_NORMAL );
      return 0;
    }

  case 3:

    switch( image_scale ) {
    case 1:
      scaler_register( SCALER_TRIPLESIZE );
      scaler_register( SCALER_TV3X );
      scaler_register( SCALER_ADVMAME3X );
      scaler_register( SCALER_PALTV3X );
      if( !scaler_is_supported( current_scaler ) )
	scaler_select_scaler( SCALER_TRIPLESIZE );
      return 0;
    case 2:
      scaler_register( SCALER_TIMEX1_5X );
      if( !scaler_is_supported( current_scaler ) )
	scaler_select_scaler( SCALER_TIMEX1_5X );
      return 0;
    }

  }

  ui_error( UI_ERROR_ERROR, "Unknown display size/image size %d/%d",
	    gtkdisplay_current_size, image_scale );
  return 1;
}

void
uidisplay_frame_end( void ) 
{
  return;
}

void
uidisplay_area( int x, int y, int w, int h )
{
  float scale = (float)gtkdisplay_current_size / image_scale;
  int scaled_x, scaled_y, i, yy;
  libspectrum_dword *palette;

  /* Extend the dirty region by 1 pixel for scalers
     that "smear" the screen, e.g. 2xSAI */
  if( scaler_flags & SCALER_FLAGS_EXPAND )
    scaler_expander( &x, &y, &w, &h, image_width, image_height );

  scaled_x = scale * x; scaled_y = scale * y;

  palette = settings_current.bw_tv ? bw_colours : gtkdisplay_colours;

  /* Create the RGB image */
  for( yy = y; yy < y + h; yy++ ) {

    libspectrum_dword *rgb; libspectrum_word *display;

    rgb = (libspectrum_dword*)( rgb_image + ( yy + 2 ) * rgb_pitch );
    rgb += x + 1;

    display = &gtkdisplay_image[yy][x];

    for( i = 0; i < w; i++, rgb++, display++ ) *rgb = palette[ *display ];
  }

  /* Create scaled image */
  scaler_proc32( &rgb_image[ ( y + 2 ) * rgb_pitch + 4 * ( x + 1 ) ],
		 rgb_pitch,
		 &scaled_image[ scaled_y * scaled_pitch + 4 * scaled_x ],
		 scaled_pitch, w, h );

  w *= scale; h *= scale;

  /* Blit to the real screen */
  gtkdisplay_area( scaled_x, scaled_y, w, h );
}

static void gtkdisplay_area(int x, int y, int width, int height)
{
  gdk_draw_rgb_32_image( gtkui_drawing_area->window,
			 gtkui_drawing_area->style->fg_gc[GTK_STATE_NORMAL],
			 x, y, width, height, GDK_RGB_DITHER_NONE,
			 &scaled_image[ y * scaled_pitch + 4 * x ],
			 scaled_pitch );
}

void
uidisplay_hotswap_gfx_mode( void )
{
  fuse_emulation_pause();

  /* Redraw the entire screen... */
  display_refresh_all();

  fuse_emulation_unpause();
}

int
uidisplay_end( void )
{
  return 0;
}

int
gtkdisplay_end( void )
{
  return 0;
}

/* Set one pixel in the display */
void
uidisplay_putpixel( int x, int y, int colour )
{
  if( machine_current->timex ) {
    x <<= 1; y <<= 1;
    gtkdisplay_image[y  ][x  ] = colour;
    gtkdisplay_image[y  ][x+1] = colour;
    gtkdisplay_image[y+1][x  ] = colour;
    gtkdisplay_image[y+1][x+1] = colour;
  } else {
    gtkdisplay_image[y][x] = colour;
  }
}

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y ) */
void
uidisplay_plot8( int x, int y, libspectrum_byte data,
                libspectrum_byte ink, libspectrum_byte paper )
{
  x <<= 3;

  if( machine_current->timex ) {
    int i;

    x <<= 1; y <<= 1;
    for( i=0; i<2; i++,y++ ) {
      gtkdisplay_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
      gtkdisplay_image[y][x+ 1] = ( data & 0x80 ) ? ink : paper;
      gtkdisplay_image[y][x+ 2] = ( data & 0x40 ) ? ink : paper;
      gtkdisplay_image[y][x+ 3] = ( data & 0x40 ) ? ink : paper;
      gtkdisplay_image[y][x+ 4] = ( data & 0x20 ) ? ink : paper;
      gtkdisplay_image[y][x+ 5] = ( data & 0x20 ) ? ink : paper;
      gtkdisplay_image[y][x+ 6] = ( data & 0x10 ) ? ink : paper;
      gtkdisplay_image[y][x+ 7] = ( data & 0x10 ) ? ink : paper;
      gtkdisplay_image[y][x+ 8] = ( data & 0x08 ) ? ink : paper;
      gtkdisplay_image[y][x+ 9] = ( data & 0x08 ) ? ink : paper;
      gtkdisplay_image[y][x+10] = ( data & 0x04 ) ? ink : paper;
      gtkdisplay_image[y][x+11] = ( data & 0x04 ) ? ink : paper;
      gtkdisplay_image[y][x+12] = ( data & 0x02 ) ? ink : paper;
      gtkdisplay_image[y][x+13] = ( data & 0x02 ) ? ink : paper;
      gtkdisplay_image[y][x+14] = ( data & 0x01 ) ? ink : paper;
      gtkdisplay_image[y][x+15] = ( data & 0x01 ) ? ink : paper;
    }
  } else {
    gtkdisplay_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
    gtkdisplay_image[y][x+ 1] = ( data & 0x40 ) ? ink : paper;
    gtkdisplay_image[y][x+ 2] = ( data & 0x20 ) ? ink : paper;
    gtkdisplay_image[y][x+ 3] = ( data & 0x10 ) ? ink : paper;
    gtkdisplay_image[y][x+ 4] = ( data & 0x08 ) ? ink : paper;
    gtkdisplay_image[y][x+ 5] = ( data & 0x04 ) ? ink : paper;
    gtkdisplay_image[y][x+ 6] = ( data & 0x02 ) ? ink : paper;
    gtkdisplay_image[y][x+ 7] = ( data & 0x01 ) ? ink : paper;
  }
}

/* Print the 16 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (16*x) , y ) */
void
uidisplay_plot16( int x, int y, libspectrum_word data,
                 libspectrum_byte ink, libspectrum_byte paper )
{
  int i;
  x <<= 4; y <<= 1;

  for( i=0; i<2; i++,y++ ) {
    gtkdisplay_image[y][x+ 0] = ( data & 0x8000 ) ? ink : paper;
    gtkdisplay_image[y][x+ 1] = ( data & 0x4000 ) ? ink : paper;
    gtkdisplay_image[y][x+ 2] = ( data & 0x2000 ) ? ink : paper;
    gtkdisplay_image[y][x+ 3] = ( data & 0x1000 ) ? ink : paper;
    gtkdisplay_image[y][x+ 4] = ( data & 0x0800 ) ? ink : paper;
    gtkdisplay_image[y][x+ 5] = ( data & 0x0400 ) ? ink : paper;
    gtkdisplay_image[y][x+ 6] = ( data & 0x0200 ) ? ink : paper;
    gtkdisplay_image[y][x+ 7] = ( data & 0x0100 ) ? ink : paper;
    gtkdisplay_image[y][x+ 8] = ( data & 0x0080 ) ? ink : paper;
    gtkdisplay_image[y][x+ 9] = ( data & 0x0040 ) ? ink : paper;
    gtkdisplay_image[y][x+10] = ( data & 0x0020 ) ? ink : paper;
    gtkdisplay_image[y][x+11] = ( data & 0x0010 ) ? ink : paper;
    gtkdisplay_image[y][x+12] = ( data & 0x0008 ) ? ink : paper;
    gtkdisplay_image[y][x+13] = ( data & 0x0004 ) ? ink : paper;
    gtkdisplay_image[y][x+14] = ( data & 0x0002 ) ? ink : paper;
    gtkdisplay_image[y][x+15] = ( data & 0x0001 ) ? ink : paper;
  }
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
drawing_area_resize_callback( GtkWidget *widget GCC_UNUSED, GdkEvent *event,
			      gpointer data GCC_UNUSED )
{
  drawing_area_resize( event->configure.width, event->configure.height );
  return TRUE;
}

#endif			/* #ifdef UI_GTK */
