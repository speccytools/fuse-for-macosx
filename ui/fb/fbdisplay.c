/* fbdisplay.c: Routines for dealing with the linux fbdev display
   Copyright (c) 2000-2003 Philip Kendall, Matan Ziv-Av, Darren Salt,
			   Witold Filipczy

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
#include "screenshot.h"
#include "ui/uidisplay.h"
#include "settings.h"

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

unsigned long fb_resolution; /* == xres << 16 | yres */
#define FB_RES(X,Y) ((X) << 16 | (Y))
#define FB_WIDTH (fb_resolution >> 16)
#define IF_FB_WIDTH(X) ((fb_resolution >> 16) == (X))
#define IF_FB_HEIGHT(Y) ((fb_resolution & 0xFFFF) == (Y))
#define IF_FB_RES(X, Y) (fb_resolution == FB_RES ((X), (Y)))

static const struct {
  int xres, yres, pixclock;
  int left_margin, right_margin, upper_margin, lower_margin;
  int hsync_len, vsync_len;
  int sync, doublescan;
} fb_modes[] = {
  { 640, 240, 32052,  128, 24, 28, 9,  40, 3,  0, 1 }, /* 640x240, 72Hz */
  { 640, 480, 32052,  128, 24, 28, 9,  40, 3,  0, 0 }, /* 640x480, 72Hz */
  { 320, 240, 64104,   64, 12, 14, 4,  20, 3,  0, 1 }, /* 320x240, 72Hz */
  { 0 }
};

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
  got_orig_display = 1;

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
fb_select_mode( size_t index )
{
  display = orig_display;
  display.xres_virtual = display.xres = fb_modes[index].xres;
  display.yres_virtual = display.yres = fb_modes[index].yres;
  display.pixclock = fb_modes[index].pixclock;
  display.left_margin = fb_modes[index].left_margin;
  display.right_margin = fb_modes[index].right_margin;
  display.upper_margin = fb_modes[index].upper_margin; 
  display.lower_margin = fb_modes[index].lower_margin;
  display.hsync_len = fb_modes[index].hsync_len;
  display.vsync_len = fb_modes[index].vsync_len;     
  display.sync = fb_modes[index].sync;
  display.vmode &= ~FB_VMODE_MASK;
  if( fb_modes[index].doublescan ) display.vmode |= FB_VMODE_DOUBLE;
  
  display.xoffset = display.yoffset = 0;
  display.bits_per_pixel = 16;
  display.red.length = display.blue.length = 5;
  display.green.length = 6;
  
#if 0				/* VIDC20 */
  display.red.offset = 0;
  display.green.offset = 5;
  display.blue.offset = 10;
#else				/* VIDC20 */
  display.red.offset = 11;     
  display.green.offset = 5;
  display.blue.offset = 0;
#endif				/* VIDC20 */

  gm = mmap( 0, fixed.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0 );
  if( gm == (void*)-1 ) {
    fprintf (stderr, "%s: couldn't mmap for framebuffer: %s\n",
	     fuse_progname, strerror( errno ) );
    return 1;
  }

  image = malloc( display.xres_virtual * 240 * 2 );
  if( !image ) {
    munmap( gm, fixed.smem_len );
    fprintf( stderr, "%s: couldn't create image\n", fuse_progname );
    return 1;
  }

  fputs( "\x1B[H\x1B[J", stdout );	/* clear tty */
  memset( gm, 0, display.xres_virtual * display.yres_virtual * 2 );

  if( ioctl( fb_fd, FBIOPUT_VSCREENINFO, &display ) ) {
    free( image ); munmap( gm, fixed.smem_len );
    return 1;
  }

#ifdef HAVE_GPM_H
  {
    Gpm_Connect conn = { 0, 0, 0, ~0 };		/* mouse event sink */
    Gpm_Open( &conn, 0 );
    gpm_connected = 1;
  }
#endif				/* #ifdef HAVE_GPM_H */

  fb_resolution = FB_RES( display.xres, display.yres );
  return 0;			/* success */
}

static int
fb_set_mode( void )
{
  size_t i;

  /* First, try to use out preferred mode */
  for( i=0; fb_modes[i].xres; i++ )
    if( fb_modes[i].xres == settings_current.svga_mode )
      if( !fb_select_mode( i ) )
	return 0;

  /* If that failed, try to use the first available mode */
  for( i=0; fb_modes[i].xres; i++ )
    if( !fb_select_mode( i ) )
      return 0;

  /* If that filed, we can't continue :-( */
  fprintf( stderr, "%s: couldn't find a framebuffer mode to start in\n",
	   fuse_progname );
  return 1;
}

void
uidisplay_frame_end( void ) 
{
  return;
}

void
uidisplay_area( int x, int start, int width, int height)
{
  int y;
  switch( fb_resolution ) {
  case FB_RES( 640, 480 ):
    for( y = start; y < start + height; y++ )
    {
      memcpy( gm +   2 * y       * display.xres_virtual + x * 2, image + y * 640 + x * 2,
	      width * 4                                                     );
      memcpy( gm + ( 2 * y + 1 ) * display.xres_virtual + x * 2, image + y * 640 + x * 2,
	      width * 4                                                     );
    }
    break;
  case FB_RES( 640, 240 ):
    for( y = start; y < start + height; y++ )
      memcpy( gm +       y       * display.xres_virtual + x * 2, image + y * 640 + x * 2,
	      width * 4                                                     );
    break;
  case FB_RES( 320, 240 ):
    for( y = start; y < start + height; y++ )
      memcpy( gm +       y       * display.xres_virtual + x, image + y * 320 + x,
	     width * 2                                                    );
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

#ifdef HAVE_GPM_H
    gpm_connected = 0;
    Gpm_Close();
#endif				/* #ifdef HAVE_GPM_H */

  }
  return 0;
}

#endif				/* #ifdef UI_FB */
