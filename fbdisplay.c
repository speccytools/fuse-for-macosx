/* fbdisplay.c: Routines for dealing with the linux fbdev display
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

#ifdef UI_FB			/* Use this iff we're using fbdev */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

#include "fuse.h"
#include "display.h"
#include "uidisplay.h"

static unsigned short *image, *gm;

static int colours[16];
static int fd;

static int fbdisplay_allocate_colours(int numColours, int *colours);
static int fbdisplay_allocate_image(int width, int height);

/* Never used, so commented out to avoid warning */
/*  static void fbdisplay_area(int x, int y, int width, int height); */

int uidisplay_init(int width, int height)
{
  struct fb_fix_screeninfo fi;

/* fb is assumed to be in 320x240x65K colour mode */
  fd=open("/dev/fb",O_RDWR);
  ioctl(fd, FBIOGET_FSCREENINFO, &fi);
  gm=mmap(0, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  fbdisplay_allocate_image(DISPLAY_SCREEN_WIDTH, DISPLAY_SCREEN_HEIGHT);
  fbdisplay_allocate_colours(16, colours);  

  return 0;
}

static int fbdisplay_allocate_colours(int numColours, int *colours)
{
  int colour_palette[] = {
  0x0000,
  0x0018,
  0xc000,
  0xc018,
  0x0600,
  0x0618,
  0xc600,
  0xc618,
  0x0000,
  0x001f,
  0xf800,
  0xf81f,
  0x07e0,
  0x07ff,
  0xffe0,
  0xffff
};
  int i;

  for(i=0;i<numColours;i++) {
    colours[i]=colour_palette[i];
  }

  return 0;
}
  
static int fbdisplay_allocate_image(int width, int height)
{
  image=malloc(width*height*2);

  if(!image) {
    fprintf(stderr,"%s: couldn't create image\n",fuse_progname);
    return 1;
  }

  return 0;
}

void uidisplay_putpixel(int x,int y,int colour)
{
  *(image+x+y*DISPLAY_SCREEN_WIDTH)=colours[colour];
}

void uidisplay_line(int y)
{
    memcpy(gm+y*320, image+y*DISPLAY_SCREEN_WIDTH, DISPLAY_SCREEN_WIDTH*2);
}

/* Never used, so commented out to avoid warning */
/*  static void fbdisplay_area(int x, int y, int width, int height) */
/*  { */
/*      int yy; */
/*      for(yy=y; yy<y+height; yy++) */
/*          memcpy(gm+yy*320+x, image+yy*DISPLAY_SCREEN_WIDTH+x, width*2); */
/*  } */

void uidisplay_set_border(int line, int pixel_from, int pixel_to, int colour)
{
    int x;

    for(x=pixel_from;x<pixel_to;x++)
        *(image+line*DISPLAY_SCREEN_WIDTH+x)=colours[colour];
}

int uidisplay_end(void)
{
    close(fd);
    return 0;
}

#endif				/* #ifdef UI_FB */
