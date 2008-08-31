/* wiidisplay.c: Routines for dealing with the Wii's framebuffer display
   Copyright (c) 2008 Bjoern Giesler

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

   E-mail: bjoern@giesler.de

*/

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/* Wii includes */
#include <fat.h>
#include <gccore.h>
#include <ogcsys.h>

#include "fuse.h"
#include "display.h"
#include "screenshot.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "settings.h"
#include "ui/wii/wiimouse.h"

/* A copy of every pixel on the screen */
libspectrum_word display_image[2 * DISPLAY_SCREEN_HEIGHT][DISPLAY_SCREEN_WIDTH];

/* Are we in a Timex display mode? */
static int hires;

static u32 *xfb = NULL;
static GXRModeObj *rmode = NULL;

static int needsclear = 1;

typedef struct {
  u8 r, g, b;
} rgb_t;

rgb_t spectrum_colors[16] = {
  /* no bright */
  {0, 0, 0},       /* BLACK */
  {0, 0, 192},     /* BLUE */
  {192, 0, 0},     /* RED */
  {192, 0, 192},   /* MAGENTA */
  {0, 192, 0},     /* GREEN */
  {0, 192, 192},   /* CYAN */
  {192, 192, 0},   /* YELLOW */
  {192, 192, 192}, /* WHITE */
  /* bright */
  {0, 0, 0},       /* BLACK */
  {0, 0, 255},     /* BLUE */
  {255, 0, 0},     /* RED */
  {255, 0, 255},   /* MAGENTA */
  {0, 255, 0},     /* GREEN */
  {0, 255, 255},   /* CYAN */
  {255, 255, 0},   /* YELLOW */
  {255, 255, 255}, /* WHITE */
};
  
u32 palette[16][16];

#define MOUSESIZEX 9
#define MOUSESIZEY 9
#define MOUSEHOTX 4
#define MOUSEHOTY 4
static int mousecursor[MOUSESIZEY][MOUSESIZEX] = {
  { 0xff, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff },
  { 0xff, 0xff, 0xff, 0x0f, 0x00, 0x0f, 0xff, 0xff, 0xff },
  { 0xff, 0xff, 0xff, 0x0f, 0x00, 0x0f, 0xff, 0xff, 0xff },
  { 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x0f, 0x0f, 0x0f, 0x0f },
  { 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f },
  { 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x0f, 0x0f, 0x0f, 0x0f },
  { 0xff, 0xff, 0xff, 0x0f, 0x00, 0x0f, 0xff, 0xff, 0xff },
  { 0xff, 0xff, 0xff, 0x0f, 0x00, 0x0f, 0xff, 0xff, 0xff },
  { 0xff, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff }
};
static int mousecache[MOUSESIZEY][MOUSESIZEX];
static int mousex = 0, mousey = 0;

u32 convert_rgb (rgb_t rgb1, rgb_t rgb2)
{
  int y1, cb1, cr1, y2, cb2, cr2, cb, cr;
 
  y1 = (299 * rgb1.r + 587 * rgb1.g + 114 * rgb1.b) / 1000;
  cb1 = (-16874 * rgb1.r - 33126 * rgb1.g + 50000 * rgb1.b + 12800000) / 100000;
  cr1 = (50000 * rgb1.r - 41869 * rgb1.g - 8131 * rgb1.b + 12800000) / 100000;
 
  y2 = (299 * rgb2.r + 587 * rgb2.g + 114 * rgb2.b) / 1000;
  cb2 = (-16874 * rgb2.r - 33126 * rgb2.g + 50000 * rgb2.b + 12800000) / 100000;
  cr2 = (50000 * rgb2.r - 41869 * rgb2.g - 8131 * rgb2.b + 12800000) / 100000;
 
  cb = (cb1 + cb2) >> 1;
  cr = (cr1 + cr2) >> 1;
 
  return (y1 << 24) | (cb << 16) | (y2 << 8) | cr;
}

void populate_palette(void)
{
  u8 i, j;
  for(i=0; i<16; i++)
    for(j=0; j<16; j++)
      palette[i][j] = convert_rgb(spectrum_colors[i], spectrum_colors[j]);
}

static void put_pixel(int x, int y, int colour, int mouseputpixel)
{
  if( machine_current->timex ) {
    x <<= 1; y <<= 1;
    display_image[y  ][x  ] = colour;
    display_image[y  ][x+1] = colour;
    display_image[y+1][x  ] = colour;
    display_image[y+1][x+1] = colour;
  } else {
    display_image[y][x] = colour;
    if(!mouseputpixel &&
       x >= mousex && x < mousex+MOUSESIZEX &&
       y >= mousey && y < mousey+MOUSESIZEY)
      mousecache[y-mousey][x-mousex] = colour;
  }
}

static int get_pixel(int x, int y)
{
  return display_image[y][x];
}

int uidisplay_init(int width, int height)
{
  hires = ( width == 640 ? 1 : 0 );

  scaler_register_clear();
  populate_palette();
  display_ui_initialised = 1;
  display_refresh_all();

  return 0;
}

int wiidisplay_init(void)
{
  VIDEO_Init();

  u32 videoMode = VIDEO_GetCurrentTvMode();
  switch(videoMode) {
  case VI_NTSC:
    rmode = &TVNtsc480IntDf;
    break;
  case VI_PAL:
    rmode = &TVPal528IntDf;
    break;
  case VI_MPAL:
    rmode = &TVMpal480IntDf;
    break;
  default:
    rmode = &TVNtsc480IntDf;
    break;
  }

  xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
  console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight,
	       rmode->fbWidth*VI_DISPLAY_PIX_SZ);

  VIDEO_Configure(rmode);
  VIDEO_SetNextFramebuffer(xfb);
  VIDEO_SetBlack(FALSE);
  VIDEO_Flush();

  VIDEO_WaitVSync();
  if(rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

  fatInitDefault(); /* FIXME: doesn't belong in display init */
  
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
  VIDEO_WaitVSync();
  if(rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

  return;
}

void wiidisplay_showmouse( float x, float y )
{
  int c, r;

  if( !fuse_emulation_paused ) return;

  int mousenewx = x*(DISPLAY_SCREEN_WIDTH/2-MOUSESIZEX);
  int mousenewy = y*(DISPLAY_SCREEN_HEIGHT-MOUSESIZEY);

  /* if we had no old mouse and have no new mouse, forget it */
  if( (mousex <= 0 || mousey <= 0) &&
      (mousenewx <= 0 || mousenewy <= 0) ) return;

  /* if we had a mouse pos before, put mousecache at old position into
     picture  */
  if( mousex > 0 && mousey > 0 ) {
    for( r=0; r<MOUSESIZEY; r++ )
      for( c=0; c<MOUSESIZEX; c++ )
	put_pixel( c+mousex, r+mousey, mousecache[r][c], 1 );
  }
  uidisplay_area( mousex, mousey, MOUSESIZEX, MOUSESIZEY );

  /* if we don't have a new mouse, remember that state and leave */
  if( mousenewx <= 0 || mousenewy <= 0 ) {
    mousex = mousenewx; mousey = mousenewy;
    return;
  }

  /* we had an old mouse and have a new one. read mouse cache from the
     picture and draw mouse cursor. */
  for( r=0; r<MOUSESIZEY; r++ )
    for( c=0; c<MOUSESIZEX; c++ ) {
      /* put picture at new position into mouse cache */
      mousecache[r][c] = get_pixel( c+mousenewx, r+mousenewy );
      /* put mouse cursor into picture */
      if( mousecursor[r][c] != 0xff )
	put_pixel( mousenewx+c, mousenewy+r, mousecursor[r][c], 1 );
    }
  mousex = mousenewx; mousey = mousenewy;
  uidisplay_area( mousex, mousey, MOUSESIZEX, MOUSESIZEY );
}

void
uidisplay_area( int x, int start, int width, int height )
{
  int y, i;

  int ystart = (rmode->xfbHeight - 2*DISPLAY_SCREEN_HEIGHT)/4;

  u32 *dest;

  if( needsclear ) {
    for( y=0; y<rmode->xfbHeight; y++ ) {
      for( i=0, dest = xfb + y*rmode->fbWidth/2;
           i<rmode->fbWidth;
	   i+=2, dest++ )
      *dest = palette[0][0];
    }
    needsclear = 0;
  }

  if( x%2 ) {
    x -= 1;
    width += 1;
  }
  
  for( y=start; y<start+height; y++ ) {
    /* FIXME: Black-and-white only */
    if(hires) {
      for( i=0, dest = xfb + 2*(y+ystart)*rmode->fbWidth/2 + x/2;
           i < width;
	   i+=2, dest++ )
        *dest = palette[display_image[y][x+i]][display_image[y][x+i+1]];
      for( i=0, dest = xfb + (2*(y+ystart) + 1)*rmode->fbWidth/2 + x/2;
	   i < width;
	   i+=2, dest++ )
	*dest = palette[display_image[y][x+i]][display_image[y][x+i+1]];
    } else {
      for( i=0, dest = xfb + 2*(y+ystart)*rmode->fbWidth/2 + x;
	   i < width;
	   i+=2, dest++ ) {
	*dest = palette[display_image[y][x+i]][display_image[y][x+i]];
	dest++;
	*dest = palette[display_image[y][x+i+1]][display_image[y][x+i+1]];
      }
      for( i=0, dest = xfb + (2*(y+ystart) + 1)*rmode->fbWidth/2 + x;
	   i < width;
	   i+=2, dest++ ) {
	*dest = palette[display_image[y][x+i]][display_image[y][x+i]];
	dest++;
	*dest = palette[display_image[y][x+i+1]][display_image[y][x+i+1]];
      }
    }
  }
}

int
uidisplay_end( void )
{
  display_ui_initialised = 0;
  return 0;
}

int
wiidisplay_end( void )
{
  return 0;
}

/* Set one pixel in the display */
void
uidisplay_putpixel( int x, int y, int colour )
{
  put_pixel(x, y, colour, 0);
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
      display_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
      display_image[y][x+ 1] = ( data & 0x80 ) ? ink : paper;
      display_image[y][x+ 2] = ( data & 0x40 ) ? ink : paper;
      display_image[y][x+ 3] = ( data & 0x40 ) ? ink : paper;
      display_image[y][x+ 4] = ( data & 0x20 ) ? ink : paper;
      display_image[y][x+ 5] = ( data & 0x20 ) ? ink : paper;
      display_image[y][x+ 6] = ( data & 0x10 ) ? ink : paper;
      display_image[y][x+ 7] = ( data & 0x10 ) ? ink : paper;
      display_image[y][x+ 8] = ( data & 0x08 ) ? ink : paper;
      display_image[y][x+ 9] = ( data & 0x08 ) ? ink : paper;
      display_image[y][x+10] = ( data & 0x04 ) ? ink : paper;
      display_image[y][x+11] = ( data & 0x04 ) ? ink : paper;
      display_image[y][x+12] = ( data & 0x02 ) ? ink : paper;
      display_image[y][x+13] = ( data & 0x02 ) ? ink : paper;
      display_image[y][x+14] = ( data & 0x01 ) ? ink : paper;
      display_image[y][x+15] = ( data & 0x01 ) ? ink : paper;
    }
  } else {
    display_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
    display_image[y][x+ 1] = ( data & 0x40 ) ? ink : paper;
    display_image[y][x+ 2] = ( data & 0x20 ) ? ink : paper;
    display_image[y][x+ 3] = ( data & 0x10 ) ? ink : paper;
    display_image[y][x+ 4] = ( data & 0x08 ) ? ink : paper;
    display_image[y][x+ 5] = ( data & 0x04 ) ? ink : paper;
    display_image[y][x+ 6] = ( data & 0x02 ) ? ink : paper;
    display_image[y][x+ 7] = ( data & 0x01 ) ? ink : paper;
  }
}

/* Print the 16 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (16*x) , y ) */
void
uidisplay_plot16( int x, int y, libspectrum_word data,
                  libspectrum_byte ink, libspectrum_byte paper )
{
  /* FIXME: what should this do? */
  return;
}

void
uidisplay_frame_save( void )
{
  /* FIXME: implement */
}

void
uidisplay_frame_restore( void )
{
  /* FIXME: implement */
}
