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

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#ifdef UI_SVGA			/* Use this iff we're using svgalib */

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

/* A scaled copy of the image displayed on the Spectrum's screen */
static WORD scaled_image[2*DISPLAY_SCREEN_HEIGHT][2*DISPLAY_SCREEN_WIDTH];
static const ptrdiff_t scaled_pitch =
                                     2 * DISPLAY_SCREEN_WIDTH * sizeof( WORD );

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
    if( available_modes[i].fuse_id == settings_current.svga_mode ) {
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
  static const int colour_palette[] = {
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

  for(i=0;i<numColours;i++)
    vga_setpalette(i,colour_palette[i*3]>>2,colour_palette[i*3+1]>>2,
		   colour_palette[i*3+2]>>2);

  return 0;
}
  
void
uidisplay_hotswap_gfx_mode( void )
{
  return;
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
  
  /* a simplified shortcut, for normal usage */
  if ( !hires && image_scale == 1 ) {
    for( yy = y; yy < y + h; yy++ ) {
      for( xx = x, ptr = linebuf; xx < x + w; xx++ )
        *ptr++ = display_image[yy][xx];
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
  scaler_proc16( (BYTE*)&display_image[y][x], display_pitch, NULL, 
		 (BYTE*)&scaled_image[scaled_y][scaled_x], scaled_pitch,
		 w, h );

  w *= scale; h *= scale;

  for( yy = scaled_y; yy < scaled_y + h; yy++ ) {
    for( xx = scaled_x, ptr = linebuf; xx < scaled_x + w; xx++ )
      *ptr++ = scaled_image[yy][xx];
    vga_drawscansegment( linebuf, scaled_x, yy, w );
  }
}

int
uidisplay_end( void )
{
  display_ui_initialised = 0;
  return 0;
}

int svgadisplay_end( void )
{
  vga_setmode( TEXT );
  return 0;
}

#endif				/* #ifdef UI_SVGA */
