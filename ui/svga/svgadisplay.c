/* svgadisplay.c: Routines for dealing with the svgalib display
   Copyright (c) 2000-2001 Philip Kendall, Matan Ziv-Av

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

static unsigned char *image;
static int hires;

static int svgadisplay_allocate_colours( int numColours );
static int svgadisplay_allocate_image(int width, int height);

static void svgadisplay_area(int x, int y, int width, int height);

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

int uidisplay_init(int width, int height)
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

  if( hires ) {
    svgadisplay_allocate_image( DISPLAY_SCREEN_WIDTH, DISPLAY_SCREEN_HEIGHT );
  } else {
    svgadisplay_allocate_image( DISPLAY_ASPECT_WIDTH, DISPLAY_SCREEN_HEIGHT );
  }

  svgadisplay_allocate_colours( 16 );

  return 0;
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
  
static int svgadisplay_allocate_image(int width, int height)
{
  image=malloc(width*height);

  if(!image) {
    fprintf(stderr,"%s: couldn't create image\n",fuse_progname);
    return 1;
  }

  return 0;
}

void uidisplay_putpixel(int x,int y,int colour)
{
#ifdef USE_LIBPNG
  screenshot_screen[y][x] = colour;
#endif			/* #ifdef USE_LIBPNG */

  if( hires ) {
    image[ x     + y * DISPLAY_SCREEN_WIDTH ] = colour;
  } else if( ! ( x & 1 ) ) {
    image[ x / 2 + y * DISPLAY_ASPECT_WIDTH ] = colour;
  }
}

void
uidisplay_frame_end( void ) 
{
  return;
}

void uidisplay_lines( int start, int end )
{
  svgadisplay_area( 0, start, DISPLAY_ASPECT_WIDTH, ( end - start + 1 ) );
}

static void svgadisplay_area(int x, int y, int width, int height)
{
  int yy;

  if( hires ) {
    
    x *= 2; width *= 2;

    for( yy = y; yy < y + height; yy++ ) {
      vga_drawscansegment( image + yy * DISPLAY_SCREEN_WIDTH + x, x,
			   yy * 2,     width);
      vga_drawscansegment( image + yy * DISPLAY_SCREEN_WIDTH + x, x,
			   yy * 2 + 1, width);
    }

  } else {
    for( yy = y; yy < y + height; yy++ )
      vga_drawscansegment( image + yy * DISPLAY_ASPECT_WIDTH + x, x,
			   yy,         width);
  }
}

void uidisplay_set_border(int line, int pixel_from, int pixel_to, int colour)
{
  if( hires ) {
    memset( image + line * DISPLAY_SCREEN_WIDTH + pixel_from,     colour,
	    pixel_to     - pixel_from     );
  } else {
    memset( image + line * DISPLAY_ASPECT_WIDTH + pixel_from / 2, colour,
	    pixel_to / 2 - pixel_from / 2 );
  }
}

int uidisplay_end(void)
{
    vga_setmode(TEXT);

    return 0;
}

#endif				/* #ifdef UI_SVGA */
