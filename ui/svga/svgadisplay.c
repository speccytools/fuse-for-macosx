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
#include "ui/uidisplay.h"

static unsigned char *image;
static int hires;

static int svgadisplay_allocate_colours( int numColours );
static int svgadisplay_allocate_image(int width, int height);

static void svgadisplay_area(int x, int y, int width, int height);

int uidisplay_init(int width, int height)
{

  vga_init();

#ifdef G640x480x256
  if( vga_hasmode( G640x480x256 ) ) {
    vga_setmode( G640x480x256 );
    hires = 1;
  } else 
#endif				/* #ifdef G640x480x256 */
#ifdef G320x240x256V
  if( vga_hasmode( G320x240x256V ) ) {
    vga_setmode( G320x240x256V );
    hires = 0;
  } else
#endif				/* #ifdef G320x240x256V */
  {
    vga_setmode( G320x240x256 );
    hires = 0;
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
  if( hires ) {
    image[ x     + y * DISPLAY_SCREEN_WIDTH ] = colour;
  } else if( ! ( x & 1 ) ) {
    image[ x / 2 + y * DISPLAY_ASPECT_WIDTH ] = colour;
  }
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
