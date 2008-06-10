/* svgadisplay.c: Routines for dealing with the svgalib display
   Copyright (c) 2000-2003 Philip Kendall, Matan Ziv-Av, Witold Filipczyk,
			   Russell Marks

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vga.h>
#include <vgakeyboard.h>

#include "fuse.h"
#include "display.h"
#include "screenshot.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"

/* The size of a 1x1 image in units of
   DISPLAY_ASPECT WIDTH x DISPLAY_SCREEN_HEIGHT */
int image_scale;

/* The height and width of a 1x1 image in pixels */
int image_width, image_height;

/* A copy of every pixel on the screen */
libspectrum_word
  svgadisplay_image[ 2 * DISPLAY_SCREEN_HEIGHT ][ DISPLAY_SCREEN_WIDTH ];
ptrdiff_t svgadisplay_pitch = DISPLAY_SCREEN_WIDTH * sizeof( libspectrum_word );

/* A scaled copy of the image displayed on the Spectrum's screen */
static libspectrum_word
  scaled_image[2*DISPLAY_SCREEN_HEIGHT][2*DISPLAY_SCREEN_WIDTH];
static const ptrdiff_t scaled_pitch =
  2 * DISPLAY_SCREEN_WIDTH * sizeof( libspectrum_word );

static int hires;

static int register_scalers( void );
static int svgadisplay_allocate_colours( int numColours );

typedef struct svga_mode_t {
  int fuse_id;
  int svgalib_id;
  int hires;
} svga_mode_t;

svga_mode_t available_modes[] = {
#ifdef G640x480x256
  { 640, G640x480x256,  1 },
#endif				/* #ifdef G640x480x256 */
#ifdef G320x240x256V
  { 320, G320x240x256V, 0 },
#endif				/* #ifdef G320x240x256V */
  {   0, G320x240x256,  0 },
};

size_t mode_count = sizeof( available_modes ) / sizeof( svga_mode_t );

int svgadisplay_init( void )
{
  size_t i;
  int found_mode = 0;

  vga_init();

  /* First, see if our preferred mode exists */
  for( i=0; i<mode_count && !found_mode; i++ ) {
    if( available_modes[i].fuse_id == settings_current.svga_mode ||
	settings_current.doublescan_mode == 0 ) {
      if( vga_hasmode( available_modes[i].svgalib_id ) ) {
	vga_setmode( available_modes[i].svgalib_id );
	hires = available_modes[i].hires;
	found_mode = 1;
      }
    }
  }

  /* If we haven't found a mode yet, try each in order */
  for( i=0; i<mode_count && !found_mode; i++ ) {
    if( vga_hasmode( available_modes[i].svgalib_id ) ) {
      vga_setmode( available_modes[i].svgalib_id );
      hires = available_modes[i].hires;
      found_mode = 1;
    }
  }

  /* Error out if we couldn't find a VGA mode */
  if( !found_mode ) {
    ui_error( UI_ERROR_ERROR, "couldn't find a mode to start in" );
    return 1;
  }

  svgadisplay_allocate_colours( 16 );

  return 0;
}

int uidisplay_init( int width, int height )
{
  int error;

  scaler_register_clear();
  
  image_width = width; image_height = height;
  image_scale = width / DISPLAY_ASPECT_WIDTH;

  error = register_scalers(); if( error ) return error;

  display_ui_initialised = 1;

  display_refresh_all();

  return 0;
}

static int
register_scalers( void )
{
  scaler_register_clear();

  switch( hires + 1 ) {

  case 1:

    switch( image_scale ) {
    case 1:
      scaler_register( SCALER_NORMAL );
      scaler_select_scaler( SCALER_NORMAL );
      return 0;
    case 2:
      scaler_register( SCALER_HALFSKIP );
      scaler_select_scaler( SCALER_HALFSKIP );
      return 0;
    }

  case 2:

    switch( image_scale ) {
    case 1:
      scaler_register( SCALER_DOUBLESIZE );
      scaler_register( SCALER_ADVMAME2X );
      scaler_select_scaler( SCALER_DOUBLESIZE );
      return 0;
    case 2:
      scaler_register( SCALER_NORMAL );
      scaler_select_scaler( SCALER_NORMAL );
      return 0;
    }

  }

  ui_error( UI_ERROR_ERROR, "Unknown display size/image size %d/%d",
	    hires + 1, image_scale );
  return 1;
}

static int
svgadisplay_allocate_colours( int numColours )
{
  static const libspectrum_byte colour_palette[] = {
  0,0,0,
  0,0,192,
  192,0,0,
  192,0,192,
  0,192,0,
  0,192,192,
  192,192,0,
  192,192,192,
  0,0,0,
  0,0,255,
  255,0,0,
  255,0,255,
  0,255,0,
  0,255,255,
  255,255,0,
  255,255,255
  };

  int i;

  for(i=0;i<numColours;i++) {
    float grey = 0.299 * colour_palette[i*3]
               + 0.587 * colour_palette[i*3+1]
               + 0.114 * colour_palette[i*3+2] + 0.5;
    vga_setpalette(i+16, grey / 4, grey / 4, grey / 4);
    vga_setpalette(i,colour_palette[i*3]>>2,colour_palette[i*3+1]>>2,
		   colour_palette[i*3+2]>>2);
  }

  return 0;
}
  
int
uidisplay_hotswap_gfx_mode( void )
{
  return 0;
}

void
uidisplay_frame_end( void ) 
{
  return;
}

void
uidisplay_area( int x, int y, int w, int h )
{
  static unsigned char linebuf[2*DISPLAY_SCREEN_WIDTH];
  unsigned char *ptr;
  float scale;
  int scaled_x, scaled_y, xx, yy;
  unsigned char grey = settings_current.bw_tv ? 16 : 0;

  /* a simplified shortcut, for normal usage */
  if ( !hires && image_scale == 1 ) {
    for( yy = y; yy < y + h; yy++ ) {
      for( xx = x, ptr = linebuf; xx < x + w; xx++ )
        *ptr++ = svgadisplay_image[yy][xx] | grey;
      vga_drawscansegment( linebuf, x, yy, w );
    }

    return;
  }

  scale = (float) ( hires + 1 ) / image_scale;
  
  /* Extend the dirty region by 1 pixel for scalers
     that "smear" the screen, e.g. AdvMAME2x */
  if( scaler_flags & SCALER_FLAGS_EXPAND )
    scaler_expander( &x, &y, &w, &h, image_width, image_height );

  scaled_x = scale * x; scaled_y = scale * y;

  /* Create scaled image */
  scaler_proc16( (libspectrum_byte*)&svgadisplay_image[y][x], svgadisplay_pitch,
		 (libspectrum_byte*)&scaled_image[scaled_y][scaled_x],
		 scaled_pitch, w, h );

  w *= scale; h *= scale;

  for( yy = scaled_y; yy < scaled_y + h; yy++ ) {
    for( xx = scaled_x, ptr = linebuf; xx < scaled_x + w; xx++ )
      *ptr++ = scaled_image[yy][xx] | grey;
    vga_drawscansegment( linebuf, scaled_x, yy, w );
  }
}

int
uidisplay_end( void )
{
  display_ui_initialised = 0;
  return 0;
}

/* Set one pixel in the display */
void
uidisplay_putpixel( int x, int y, int colour )
{
  if( machine_current->timex ) {
    x <<= 1; y <<= 1;
    svgadisplay_image[y  ][x  ] = colour;
    svgadisplay_image[y  ][x+1] = colour;
    svgadisplay_image[y+1][x  ] = colour;
    svgadisplay_image[y+1][x+1] = colour;
  } else {
    svgadisplay_image[y][x] = colour;
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
      svgadisplay_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
      svgadisplay_image[y][x+ 1] = ( data & 0x80 ) ? ink : paper;
      svgadisplay_image[y][x+ 2] = ( data & 0x40 ) ? ink : paper;
      svgadisplay_image[y][x+ 3] = ( data & 0x40 ) ? ink : paper;
      svgadisplay_image[y][x+ 4] = ( data & 0x20 ) ? ink : paper;
      svgadisplay_image[y][x+ 5] = ( data & 0x20 ) ? ink : paper;
      svgadisplay_image[y][x+ 6] = ( data & 0x10 ) ? ink : paper;
      svgadisplay_image[y][x+ 7] = ( data & 0x10 ) ? ink : paper;
      svgadisplay_image[y][x+ 8] = ( data & 0x08 ) ? ink : paper;
      svgadisplay_image[y][x+ 9] = ( data & 0x08 ) ? ink : paper;
      svgadisplay_image[y][x+10] = ( data & 0x04 ) ? ink : paper;
      svgadisplay_image[y][x+11] = ( data & 0x04 ) ? ink : paper;
      svgadisplay_image[y][x+12] = ( data & 0x02 ) ? ink : paper;
      svgadisplay_image[y][x+13] = ( data & 0x02 ) ? ink : paper;
      svgadisplay_image[y][x+14] = ( data & 0x01 ) ? ink : paper;
      svgadisplay_image[y][x+15] = ( data & 0x01 ) ? ink : paper;
    }
  } else {
    svgadisplay_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
    svgadisplay_image[y][x+ 1] = ( data & 0x40 ) ? ink : paper;
    svgadisplay_image[y][x+ 2] = ( data & 0x20 ) ? ink : paper;
    svgadisplay_image[y][x+ 3] = ( data & 0x10 ) ? ink : paper;
    svgadisplay_image[y][x+ 4] = ( data & 0x08 ) ? ink : paper;
    svgadisplay_image[y][x+ 5] = ( data & 0x04 ) ? ink : paper;
    svgadisplay_image[y][x+ 6] = ( data & 0x02 ) ? ink : paper;
    svgadisplay_image[y][x+ 7] = ( data & 0x01 ) ? ink : paper;
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
    svgadisplay_image[y][x+ 0] = ( data & 0x8000 ) ? ink : paper;
    svgadisplay_image[y][x+ 1] = ( data & 0x4000 ) ? ink : paper;
    svgadisplay_image[y][x+ 2] = ( data & 0x2000 ) ? ink : paper;
    svgadisplay_image[y][x+ 3] = ( data & 0x1000 ) ? ink : paper;
    svgadisplay_image[y][x+ 4] = ( data & 0x0800 ) ? ink : paper;
    svgadisplay_image[y][x+ 5] = ( data & 0x0400 ) ? ink : paper;
    svgadisplay_image[y][x+ 6] = ( data & 0x0200 ) ? ink : paper;
    svgadisplay_image[y][x+ 7] = ( data & 0x0100 ) ? ink : paper;
    svgadisplay_image[y][x+ 8] = ( data & 0x0080 ) ? ink : paper;
    svgadisplay_image[y][x+ 9] = ( data & 0x0040 ) ? ink : paper;
    svgadisplay_image[y][x+10] = ( data & 0x0020 ) ? ink : paper;
    svgadisplay_image[y][x+11] = ( data & 0x0010 ) ? ink : paper;
    svgadisplay_image[y][x+12] = ( data & 0x0008 ) ? ink : paper;
    svgadisplay_image[y][x+13] = ( data & 0x0004 ) ? ink : paper;
    svgadisplay_image[y][x+14] = ( data & 0x0002 ) ? ink : paper;
    svgadisplay_image[y][x+15] = ( data & 0x0001 ) ? ink : paper;
  }
}

int svgadisplay_end( void )
{
  vga_setmode( TEXT );
  return 0;
}

void
uidisplay_frame_save( void )
{
  /* FIXME: Save current framebuffer state as the widget UI wants to scribble
     in here */
}

void
uidisplay_frame_restore( void )
{
  /* FIXME: Restore saved framebuffer state as the widget UI wants to draw a
     new menu */
}
