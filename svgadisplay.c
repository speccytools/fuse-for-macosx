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

   E-mail: pak@ast.cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#ifdef UI_SVGA			/* Use this iff we're using svgalib */

#include <stdio.h>
#include <stdlib.h>
#include <vga.h>
#include <vgakeyboard.h>

#include "display.h"
#include "svgadisplay.h"

static unsigned char *image;

static int colours[16];

static int svgadisplay_allocate_colours(int numColours, int *colours);
static int svgadisplay_allocate_image(int width, int height);

int svgadisplay_init(int width, int height)
{

  vga_init();
  vga_setmode(G320x240x256V);
  
  svgadisplay_allocate_image(DISPLAY_SCREEN_WIDTH, DISPLAY_SCREEN_HEIGHT);
  svgadisplay_allocate_colours(16, colours);  

  return 0;
}

static int svgadisplay_allocate_colours(int numColours, int *colours)
{
  int colour_palette[] = {
  0,0,0,
  0,0,128,
  128,0,0,
  128,0,128,
  0,128,0,
  0,128,128,
  128,128,0,
  128,128,128,
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
    colours[i]=i;
    vga_setpalette(i,colour_palette[i*3]>>2,colour_palette[i*3+1]>>2,
		   colour_palette[i*3+2]>>2);
  }

  return 0;
}
  
static int svgadisplay_allocate_image(int width, int height)
{
  image=malloc(width*height);

  if(!image) {
    fprintf(stderr,"%s: couldn't create image\n",progname);
    return 1;
  }

  return 0;
}

void svgadisplay_putpixel(int x,int y,int colour)
{
  *(image+x+y*DISPLAY_SCREEN_WIDTH)=colour;
}

void svgadisplay_line(int y)
{
    vga_drawscansegment(image+y*DISPLAY_SCREEN_WIDTH,0,y,DISPLAY_SCREEN_WIDTH);
}

void svgadisplay_area(int x, int y, int width, int height)
{
    int yy;
    for(yy=y; yy<y+height; yy++)
        vga_drawscansegment(image+yy*DISPLAY_SCREEN_WIDTH+x,x,yy,width);
}

void svgadisplay_set_border(int line, int pixel_from, int pixel_to, int colour)
{
    int x;
  
    for(x=pixel_from;x<pixel_to;x++)
        *(image+line*DISPLAY_SCREEN_WIDTH+x)=colours[colour];
}

int svgadisplay_end(void)
{
    vga_setmode(TEXT);

    return 0;
}

#endif				/* #ifdef UI_SVGA */
