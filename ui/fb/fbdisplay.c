/* fbdisplay.c: Routines for dealing with the linux fbdev display
   Copyright (c) 2000-2002 Philip Kendall, Matan Ziv-Av, Darren Salt

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

#ifdef UI_FB			/* Use this iff we're using fbdev */

#include <errno.h>
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
#include "ui/uidisplay.h"

/* rrrrrggggggbbbbb */
static const short colours[] = {
  0x0000, 0x0018, 0xC000, 0xC018, 0x0600, 0x0618, 0xC600, 0xC618,
  0x4208, 0x001F, 0xF800, 0xF81F, 0x07E0, 0x07FF, 0xFFE0, 0xFFFF
};

static int fb_fd = -1;		/* The framebuffer's file descriptor */
static WORD *image = 0, *gm = 0;

static struct fb_fix_screeninfo fixed;
static struct fb_var_screeninfo orig_display, display;
static int got_orig_display = 0;

/* The resolutions we'll try and select (in order) */
enum {
  FB_640_240,
  FB_640_480,
  FB_320_240,
  FB_FAILED
} fb_resolution;

static int fb_set_mode( void );

int uidisplay_init(int width, int height)
{
  fb_fd = open( "/dev/fb0", O_RDWR );
  if( fb_fd == -1 ) {
    fprintf( stderr, "%s: couldn't open framebuffer device\n", fuse_progname );
    return 1;
  }
  if( ioctl( fb_fd, FBIOGET_FSCREENINFO, &fixed )        ||
      ioctl( fb_fd, FBIOGET_VSCREENINFO, &orig_display )    ) {
    fprintf( stderr, "%s: couldn't read framebuffer device info\n",
	     fuse_progname );
    return 1;
  }

  if( fb_set_mode() ) return 1;

  fputs( "\x1B[H\x1B[J", stdout );	/* clear tty */
  memset( gm, 0, display.xres_virtual * display.yres_virtual * 2 );

  if( ioctl( fb_fd, FBIOPUT_VSCREENINFO, &display ) ) {
    fprintf( stderr, "%s: couldn't set framebuffer mode\n", fuse_progname );
    return 1;
  }

  return 0;
}

static int
fb_set_mode( void )
{
  fb_resolution = 0;

  while( fb_resolution != FB_FAILED )
  {
    display = orig_display;

    if( fb_resolution == FB_320_240 ) {
      display.xres_virtual = display.xres = 320;
      display.yres_virtual = display.yres = 240;
      display.pixclock = 82440;
      display.left_margin = 30;
      display.right_margin = 16;
      display.upper_margin = 20;
      display.lower_margin = 4;
      display.hsync_len = 48;
      display.vsync_len = 1;
      display.sync = 0;
      display.vmode = (display.vmode & ~FB_VMODE_MASK) | FB_VMODE_DOUBLE;
    } else {
      display.xres_virtual = display.xres = 640;
      display.pixclock = 32052;
      display.left_margin = 112;
      display.right_margin = 40;
      display.upper_margin = 28;
      display.lower_margin = 9;
      display.hsync_len = 40;
      display.vsync_len = 3;
      display.sync = 0;
      display.vmode &= ~FB_VMODE_MASK;
      if( fb_resolution == FB_640_480 ) {
	display.yres_virtual = display.yres = 480;
      } else {
	display.vmode |= FB_VMODE_DOUBLE;
	display.yres_virtual = display.yres = 240;
      }
    }

    display.xoffset = display.yoffset = 0;
    display.bits_per_pixel = 16;
    display.red.length = display.blue.length = 5;
    display.green.length = 6;

    display.red.offset = 11;
    display.green.offset = 5;
    display.blue.offset = 0;

    gm =
      mmap( 0, fixed.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0 );
    if( gm == (void *)-1 ) {
      fprintf (stderr, "%s: couldn't mmap for framebuffer: %s\n",
	       fuse_progname, strerror( errno ) );
      return 1;
    }

    image = malloc( display.xres_virtual * display.yres_virtual * 2 );
    if( !image ) {
      munmap( gm, fixed.smem_len );
      fprintf( stderr, "%s: couldn't create image\n", fuse_progname );
      return 1;
    }

    if( !ioctl( fb_fd, FBIOPUT_VSCREENINFO, &display ) ) return 0;

    fb_resolution += 1;
  }

  fprintf( stderr, "%s: couldn't set framebuffer mode\n", fuse_progname );

  return 1;
}

void uidisplay_putpixel(int x,int y,int colour)
{
  if( fb_resolution == FB_320_240 ) {
    if( ( x & 1 ) == 0 )
      *( image + 320 * y + (x >> 1) ) = colours[colour];
  } else {
    *( image + 640 * y + x ) = colours[colour];
  }
}

void
uidisplay_line( int y )
{
  switch( fb_resolution )
  {
  case FB_640_480:
    memcpy( gm +  2 * y      * display.xres_virtual, image + y * 640,
	    640 * 2                                                   );
    memcpy( gm + (2 * y + 1) * display.xres_virtual, image + y * 640,
	    640 * 2                                                   );
    break;
  case FB_640_240:
    memcpy( gm +      y      * display.xres_virtual, image + y * 640,
	    640 * 2                                                   );
    break;
  case FB_320_240:
    memcpy( gm +      y      * display.xres_virtual, image + y * 320,
	    320 * 2                                                   );
    break;
  default:;		/* Shut gcc up */
  }
}

void
uidisplay_lines( int start, int end )
{
  int y;
  switch( fb_resolution ) {
  case FB_640_480:
    for( y = start; y <= end; y++ )
    {
      memcpy( gm +   2 * y       * display.xres_virtual, image + y * 640,
	      640 * 2                                                     );
      memcpy( gm + ( 2 * y + 1 ) * display.xres_virtual, image + y * 640,
	      640 * 2                                                     );
    }
    break;
  case FB_640_240:
    for( y = start; y <= end; y++ )
      memcpy( gm +       y       * display.xres_virtual, image + y * 640,
	      640 * 2                                                     );
    break;
  case FB_320_240:
    for( y = start; y <= end; y++ )
      memcpy( gm +       y       * display.xres_virtual, image + y * 320,
	      320 * 2                                                     );
    break;
  default:;		/* Shut gcc up */
  }
}

void uidisplay_set_border(int line, int pixel_from, int pixel_to, int colour)
{
  int i;
  WORD *ptr;
  colour = colours[colour];

  switch( fb_resolution ) {
  case FB_640_480:
  case FB_640_240:
    ptr = image + line * 640 + pixel_from;
    for( i = pixel_from; i < pixel_to; i++ ) *ptr++ = colour;
    break;
  case FB_320_240:
    ptr = image + line * 320 + pixel_from / 2;
    for( i = pixel_from / 2; i < pixel_to / 2; i++ ) *ptr++ = colour;
    break;
  default:;		/* Shut gcc up */
  }
}

int uidisplay_end(void)
{
  if( fb_fd != -1 ) {
    if( got_orig_display ) ioctl( fb_fd, FBIOPUT_VSCREENINFO, &orig_display );
    close( fb_fd );
    fb_fd = -1;
  }
  return 0;
}

#endif				/* #ifdef UI_FB */
