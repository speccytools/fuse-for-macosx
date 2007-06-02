/* win32display.c: Routines for dealing with the Win32 DirectDraw display
   Copyright (c) 2003 Philip Kendall, Marek Januszewski, Stuart Brady

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

#include "config.h"

#ifdef UI_WIN32			/* Use this iff we're using UI_WIN32 */

#include "display.h"
#include "machine.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "ui/scaler/scaler.h"
#include "win32keyboard.h"
#include "win32display.h"
#include "win32internals.h"

/* The size of a 1x1 image in units of
   DISPLAY_ASPECT WIDTH x DISPLAY_SCREEN_HEIGHT */
int image_scale;

/* The height and width of a 1x1 image in pixels */
int image_width, image_height;

int fuse_nCmdShow;

/* A copy of every pixel on the screen, replaceable by plotting directly into
   rgb_image below */
libspectrum_word
  win32display_image[ 2 * DISPLAY_SCREEN_HEIGHT ][ DISPLAY_SCREEN_WIDTH ];
ptrdiff_t win32display_pitch = DISPLAY_SCREEN_WIDTH *
			       sizeof( libspectrum_word );

/* An RGB image of the Spectrum screen; slightly bigger than the real
   screen to handle the smoothing filters which read around each pixel */
char rgb_image[ 4 * 2 * ( DISPLAY_SCREEN_HEIGHT + 4 ) *
	                    ( DISPLAY_SCREEN_WIDTH  + 3 )   ];

const int rgb_pitch = ( DISPLAY_SCREEN_WIDTH + 3 ) * 4;

BITMAPINFO fuse_BMI;
HBITMAP fuse_BMP;

/* The scaled image */
char scaled_image[ 4 * 3 * DISPLAY_SCREEN_HEIGHT *
			    (size_t)(1.5 * DISPLAY_SCREEN_WIDTH) ];
const ptrdiff_t scaled_pitch = 4 * 1.5 * DISPLAY_SCREEN_WIDTH;

void *win32_pixdata;

const unsigned char rgb_colours[16][3] = {

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

libspectrum_dword win32display_colours[16];
libspectrum_dword bw_colours[16];

/* The current size of the window (in units of DISPLAY_SCREEN_*) */
int win32display_current_size=1;

int init_colours( void );
int register_scalers( void );
int register_scalers_noresize( void );
void win32display_area(int x, int y, int width, int height);

void
blit( void )
{
  if( display_ui_initialised ) {
    HDC dest_dc, src_dc;
    RECT dest_rec;
    HBITMAP src_bmp;

    dest_dc = GetDC( fuse_hWnd );
    GetClientRect( fuse_hWnd, &dest_rec );

    src_dc = CreateCompatibleDC(0);
    src_bmp = SelectObject( src_dc, fuse_BMP );
    BitBlt( dest_dc, 0, 0, image_width * win32display_current_size,
	    image_height * win32display_current_size, src_dc, 0, 0, SRCCOPY );
    SelectObject( src_dc, src_bmp );
    DeleteObject( src_bmp );
    DeleteDC( src_dc );
    ReleaseDC( fuse_hWnd, dest_dc );

/* If BitBlt isn't available:
  SetDIBitsToDevice( dc, 0, 0, dest_rec.right, dest_rec.bottom, 0, 0, 0,
		       image_height + 100, win32_pixdata, &fuse_BMI,
		       DIB_RGB_COLORS );
*/
  }
}

int
win32display_init( void )
{
  int x, y, error;
  libspectrum_dword black;

  error = init_colours(); if( error ) return error;

  black = settings_current.bw_tv ? bw_colours[0] : win32display_colours[0];

  for( y = 0; y < DISPLAY_SCREEN_HEIGHT + 4; y++ )
    for( x = 0; x < DISPLAY_SCREEN_WIDTH + 3; x++ )
      *(libspectrum_dword*)( rgb_image + y * rgb_pitch + 4 * x ) = black;

  /* create the back buffer */

  memset( &fuse_BMI, 0, sizeof( fuse_BMI ) );
  fuse_BMI.bmiHeader.biSize = sizeof( fuse_BMI.bmiHeader );
  fuse_BMI.bmiHeader.biWidth = (size_t)( 1.5 * DISPLAY_SCREEN_WIDTH );
  /* negative to avoid "shep-mode": */
  fuse_BMI.bmiHeader.biHeight = -( 3 * DISPLAY_SCREEN_HEIGHT );
  fuse_BMI.bmiHeader.biPlanes = 1;
  fuse_BMI.bmiHeader.biBitCount = 32;
  fuse_BMI.bmiHeader.biCompression = BI_RGB;
  fuse_BMI.bmiHeader.biSizeImage = 0;
  fuse_BMI.bmiHeader.biXPelsPerMeter = 0;
  fuse_BMI.bmiHeader.biYPelsPerMeter = 0;
  fuse_BMI.bmiHeader.biClrUsed = 0;
  fuse_BMI.bmiHeader.biClrImportant = 0;
  fuse_BMI.bmiColors[0].rgbRed = 0;
  fuse_BMI.bmiColors[0].rgbGreen = 0;
  fuse_BMI.bmiColors[0].rgbBlue = 0;
  fuse_BMI.bmiColors[0].rgbReserved = 0;

  HDC dc = GetDC( fuse_hWnd );

  fuse_BMP = CreateDIBSection( dc, &fuse_BMI, DIB_RGB_COLORS, &win32_pixdata,
			       NULL, 0 );

  ReleaseDC( fuse_hWnd, dc );

  display_ui_initialised = 1;

  return 0;
}

int
init_colours( void )
{
  size_t i;

  for( i = 0; i < 16; i++ ) {

    unsigned char red, green, blue, grey;

    red   = rgb_colours[i][0];
    green = rgb_colours[i][1];
    blue  = rgb_colours[i][2];

    /* Addition of 0.5 is to avoid rounding errors */
    grey = ( 0.299 * red + 0.587 * green + 0.114 * blue ) + 0.5;

#ifdef WORDS_BIGENDIAN

    win32display_colours[i] =  red << 24 | green << 16 | blue << 8;
	      bw_colours[i] = grey << 24 |  grey << 16 | grey << 8;

#else				/* #ifdef WORDS_BIGENDIAN */

    win32display_colours[i] =  red | green << 8 | blue << 16;
	      bw_colours[i] = grey |  grey << 8 | grey << 16;

#endif				/* #ifdef WORDS_BIGENDIAN */

  }

  return 0;
}

void
win32display_setsize()
{
  RECT rect, wrect, srect;
  int statusbar_height;
  int width = image_width;
  int height = image_height;
  float scale = (float)win32display_current_size / image_scale;

  width *= scale;
  height *= scale;

/*
  GetClientRect( fuse_hStatusWindow, &srect );
  statusbar_height = srect.bottom - srect.top;
*/
  statusbar_height = 0;

  GetWindowRect( fuse_hWnd, &wrect );
  GetClientRect( fuse_hWnd, &rect );
  rect.right = rect.left + width;
  rect.bottom = rect.top + height + statusbar_height;

  AdjustWindowRect( &rect,
    WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX,
    TRUE );
  /* 'rect' now holds the size of the window */
  MoveWindow( fuse_hWnd, wrect.left, wrect.top,
    rect.right - rect.left, rect.bottom - rect.top, TRUE );
}

int
uidisplay_init( int width, int height )
{
  image_width = width; image_height = height;
  image_scale = width / DISPLAY_ASPECT_WIDTH;

  /* resize */

  register_scalers();

  win32display_current_size = image_scale;
  win32display_setsize();

  ShowWindow( fuse_hWnd, fuse_nCmdShow );

  display_refresh_all();
  return 0;
}

int
register_scalers_noresize( void )
{
  scaler_register_clear();

  switch( win32display_current_size ) {

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
	    win32display_current_size, image_scale );
  return 1;
}

static int
register_scalers( void )
{
  scaler_register_clear();

  scaler_register( SCALER_NORMAL );
  scaler_register( SCALER_DOUBLESIZE );
  scaler_register( SCALER_TRIPLESIZE );
  scaler_register( SCALER_2XSAI );
  scaler_register( SCALER_SUPER2XSAI );
  scaler_register( SCALER_SUPEREAGLE );
  scaler_register( SCALER_ADVMAME2X );
  scaler_register( SCALER_ADVMAME3X );
  scaler_register( SCALER_DOTMATRIX );
  scaler_register( SCALER_PALTV );
  if( machine_current->timex ) {
    scaler_register( SCALER_HALF );
    scaler_register( SCALER_HALFSKIP );
    scaler_register( SCALER_TIMEXTV );
    scaler_register( SCALER_TIMEX1_5X );
  } else {
    scaler_register( SCALER_TV2X );
    scaler_register( SCALER_TV3X );
    scaler_register( SCALER_PALTV2X );
    scaler_register( SCALER_PALTV3X );
  }

  if( scaler_is_supported( current_scaler ) ) {
    scaler_select_scaler( current_scaler );
  } else {
    scaler_select_scaler( SCALER_NORMAL );
  }
  return 0;
}

void
uidisplay_frame_end( void )
{
  InvalidateRect( fuse_hWnd, NULL, FALSE );
}

void
uidisplay_area( int x, int y, int w, int h )
{
  float scale = (float)win32display_current_size / image_scale;
  int scaled_x, scaled_y, i, yy;
  libspectrum_dword *palette;

  /* Extend the dirty region by 1 pixel for scalers
     that "smear" the screen, e.g. 2xSAI */
  if( scaler_flags & SCALER_FLAGS_EXPAND )
    scaler_expander( &x, &y, &w, &h, image_width, image_height );

  scaled_x = scale * x; scaled_y = scale * y;

  palette = settings_current.bw_tv ? bw_colours : win32display_colours;

  /* Create the RGB image */
  for( yy = y; yy < y + h; yy++ ) {

    libspectrum_dword *rgb; libspectrum_word *display;

    rgb = (libspectrum_dword*)( rgb_image + ( yy + 2 ) * rgb_pitch );
    rgb += x + 1;

    display = &win32display_image[yy][x];

    for( i = 0; i < w; i++, rgb++, display++ ) *rgb = palette[ *display ];
  }

  /* Create scaled image */
  scaler_proc32( &rgb_image[ ( y + 2 ) * rgb_pitch + 4 * ( x + 1 ) ],
		 rgb_pitch,
		 &scaled_image[ scaled_y * scaled_pitch + 4 * scaled_x ],
		 scaled_pitch, w, h );

  w *= scale; h *= scale;

  /* Blit to the real screen */
  win32display_area( scaled_x, scaled_y, w, h );
}

void
win32display_area(int x, int y, int width, int height)
{
  int disp_x,disp_y;
  long ofs;
  char *pixdata = win32_pixdata;

  for( disp_y = y; disp_y < y + height; disp_y++)
  {
    for( disp_x = x; disp_x < x + width; disp_x++)
    {
      ofs = ( 4 * disp_x ) + ( disp_y * scaled_pitch );

      pixdata[ ofs + 0 ] = scaled_image[ ofs + 2 ]; /* blue */
      pixdata[ ofs + 1 ] = scaled_image[ ofs + 1 ]; /* green */
      pixdata[ ofs + 2 ] = scaled_image[ ofs + 0 ]; /* red */
      pixdata[ ofs + 3 ] = 0; /* unused */
    }
  }
}

int
uidisplay_hotswap_gfx_mode( void )
{
/* TODO: pause hangs emulator
  fuse_emulation_pause();
*/
  win32display_current_size = scaler_get_scaling_factor( current_scaler ) * image_scale;
  win32display_setsize();

  /* Redraw the entire screen... */
  display_refresh_all();
/* TODO: pause hangs emulator
  fuse_emulation_unpause();
*/

  return 0;
}

int
uidisplay_end( void )
{
  return 0;
}

/* Set one pixel in the display */
void
uidisplay_putpixel( int x, int y, int colour )
{
  if( machine_current->timex ) {
    x <<= 1; y <<= 1;
    win32display_image[y  ][x  ] = colour;
    win32display_image[y  ][x+1] = colour;
    win32display_image[y+1][x  ] = colour;
    win32display_image[y+1][x+1] = colour;
  } else {
    win32display_image[y][x] = colour;
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
      win32display_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
      win32display_image[y][x+ 1] = ( data & 0x80 ) ? ink : paper;
      win32display_image[y][x+ 2] = ( data & 0x40 ) ? ink : paper;
      win32display_image[y][x+ 3] = ( data & 0x40 ) ? ink : paper;
      win32display_image[y][x+ 4] = ( data & 0x20 ) ? ink : paper;
      win32display_image[y][x+ 5] = ( data & 0x20 ) ? ink : paper;
      win32display_image[y][x+ 6] = ( data & 0x10 ) ? ink : paper;
      win32display_image[y][x+ 7] = ( data & 0x10 ) ? ink : paper;
      win32display_image[y][x+ 8] = ( data & 0x08 ) ? ink : paper;
      win32display_image[y][x+ 9] = ( data & 0x08 ) ? ink : paper;
      win32display_image[y][x+10] = ( data & 0x04 ) ? ink : paper;
      win32display_image[y][x+11] = ( data & 0x04 ) ? ink : paper;
      win32display_image[y][x+12] = ( data & 0x02 ) ? ink : paper;
      win32display_image[y][x+13] = ( data & 0x02 ) ? ink : paper;
      win32display_image[y][x+14] = ( data & 0x01 ) ? ink : paper;
      win32display_image[y][x+15] = ( data & 0x01 ) ? ink : paper;
    }
  } else {
    win32display_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
    win32display_image[y][x+ 1] = ( data & 0x40 ) ? ink : paper;
    win32display_image[y][x+ 2] = ( data & 0x20 ) ? ink : paper;
    win32display_image[y][x+ 3] = ( data & 0x10 ) ? ink : paper;
    win32display_image[y][x+ 4] = ( data & 0x08 ) ? ink : paper;
    win32display_image[y][x+ 5] = ( data & 0x04 ) ? ink : paper;
    win32display_image[y][x+ 6] = ( data & 0x02 ) ? ink : paper;
    win32display_image[y][x+ 7] = ( data & 0x01 ) ? ink : paper;
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
    win32display_image[y][x+ 0] = ( data & 0x8000 ) ? ink : paper;
    win32display_image[y][x+ 1] = ( data & 0x4000 ) ? ink : paper;
    win32display_image[y][x+ 2] = ( data & 0x2000 ) ? ink : paper;
    win32display_image[y][x+ 3] = ( data & 0x1000 ) ? ink : paper;
    win32display_image[y][x+ 4] = ( data & 0x0800 ) ? ink : paper;
    win32display_image[y][x+ 5] = ( data & 0x0400 ) ? ink : paper;
    win32display_image[y][x+ 6] = ( data & 0x0200 ) ? ink : paper;
    win32display_image[y][x+ 7] = ( data & 0x0100 ) ? ink : paper;
    win32display_image[y][x+ 8] = ( data & 0x0080 ) ? ink : paper;
    win32display_image[y][x+ 9] = ( data & 0x0040 ) ? ink : paper;
    win32display_image[y][x+10] = ( data & 0x0020 ) ? ink : paper;
    win32display_image[y][x+11] = ( data & 0x0010 ) ? ink : paper;
    win32display_image[y][x+12] = ( data & 0x0008 ) ? ink : paper;
    win32display_image[y][x+13] = ( data & 0x0004 ) ? ink : paper;
    win32display_image[y][x+14] = ( data & 0x0002 ) ? ink : paper;
    win32display_image[y][x+15] = ( data & 0x0001 ) ? ink : paper;
  }
}

int
win32display_end( void )
{
  return 0;
}

#endif			/* #ifdef UI_WIN32 */
