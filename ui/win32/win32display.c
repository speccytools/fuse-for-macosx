/* win32display.c: Routines for dealing with the Win32 DirectDraw display
   Copyright (c) 2003 Philip Kendall, Marek Januszewski

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
#include "ui/ui.h"
#include "ui/scaler/scaler.h"
#include "win32keyboard.h"
#include "win32display.h"
#include "win32internals.h"

/* The size of a 1x1 image in units of
   DISPLAY_ASPECT WIDTH x DISPLAY_SCREEN_HEIGHT */
int image_scale;

/* The height and width of a 1x1 image in pixels */
int image_width, image_height;

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

/* The scaled image */
char scaled_image[ 4 * 3 * DISPLAY_SCREEN_HEIGHT *
			    (size_t)(1.5 * DISPLAY_SCREEN_WIDTH) ];
const ptrdiff_t scaled_pitch = 4 * 1.5 * DISPLAY_SCREEN_WIDTH;

const char rgb_colours[16][3] = {

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
int win32display_init( void );
int uidisplay_init( int width, int height );
int win32display_configure_notify( int width );
int register_scalers( void );
void uidisplay_frame_end( void );
void uidisplay_area( int x, int y, int width, int height );
void win32display_area(int x, int y, int width, int height);
int uidisplay_end( void );
int win32display_end( void );

int
win32display_init( void )
{
  int x, y, error;
  libspectrum_dword black;

  error = init_colours(); if( error ) return error;

/* TODO: implement settings
  black = settings_current.bw_tv ? bw_colours[0] : win32display_colours[0];
*/
  black = win32display_colours[0];

  for( y = 0; y < DISPLAY_SCREEN_HEIGHT + 4; y++ )
    for( x = 0; x < DISPLAY_SCREEN_WIDTH + 3; x++ )
      *(libspectrum_dword*)( rgb_image + y * rgb_pitch + 4 * x ) = black;

  display_ui_initialised = 1;

  return 0;
}

int
init_colours( void )
{
  size_t i;

  for( i = 0; i < 16; i++ ) {

    char red, green, blue, grey;

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

int
uidisplay_init( int width, int height )
{
  int error;

  image_width = width; image_height = height;
  image_scale = width / DISPLAY_ASPECT_WIDTH;

//
  HRESULT ddres;
  RECT rect, rect2;

  /* resize */
  GetClientRect( fuse_hWnd, &rect );
  rect.right = rect.left + width;
/* TODO: get statusbar height somehow */
  rect.bottom = rect.top + height + 40;
  GetWindowRect( fuse_hWnd, &rect2 );
  AdjustWindowRect( &rect,
    WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
    FALSE );
  MoveWindow( fuse_hWnd, rect2.left, rect2.top,
    rect.right - rect.left, rect.bottom - rect.top, TRUE );

  /* dd init */
  ddres = DirectDrawCreate(NULL, &lpdd, NULL);
  DD_ERROR( "error initializing" )

  /* set cooperative level to normal */
  ddres = IDirectDraw_SetCooperativeLevel( lpdd, fuse_hWnd, DDSCL_NORMAL );
  DD_ERROR( "error setting cooperative level" )

  /* fill surface description */
  memset( &ddsd, 0, sizeof( ddsd ) );
  ddsd.dwSize = sizeof( ddsd );
  ddsd.dwFlags = DDSD_CAPS;
  ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

  /* attempt to create primary surface */
  ddres = IDirectDraw_CreateSurface( lpdd, &ddsd, &lpdds, NULL );
  DD_ERROR( "error creating primary surface" )

  /* fill off screen surface description based on window size*/
  GetClientRect( fuse_hWnd, &rect);
  memset( &ddsd, 0, sizeof( ddsd ) );
  ddsd.dwSize = sizeof( ddsd );
  ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
  ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
  ddsd.dwWidth = rect.right - rect.left;
  ddsd.dwHeight = rect.bottom - rect.top;

  /* attempt to create off screen surface */
  ddres = IDirectDraw_CreateSurface( lpdd, &ddsd, &lpdds2, NULL );
  DD_ERROR( "error creating off-screen surface" )

  /* create the clipper */
  ddres = IDirectDraw_CreateClipper( lpdd, 0, &lpddc, NULL );
  DD_ERROR( "error creating clipper" )

  /* set the window */
  ddres = IDirectDrawClipper_SetHWnd( lpddc, 0, fuse_hWnd );
  DD_ERROR( "error setting window for the clipper" )

  /* set clipper for the primary surface */
/*
  moved to WM_PAINT
  ddres = IDirectDrawSurface_SetClipper( lpdds, lpddc );
  DD_ERROR( "error setting clipper for primary surface" )
*/

//
  error = register_scalers(); if( error ) return error;

  display_refresh_all();
  return 0;
}

int gtkdisplay_configure_notify( int width )
{
  int size, error;

  size = width / DISPLAY_ASPECT_WIDTH;

  /* If we're the same size as before, nothing special needed */
  if( size == win32display_current_size ) return 0;

  /* Else set ourselves to the new height */
  win32display_current_size=size;
/* TODO: change the area size
  gtk_drawing_area_size( GTK_DRAWING_AREA(gtkui_drawing_area),
			 size * DISPLAY_ASPECT_WIDTH,
			 size * DISPLAY_SCREEN_HEIGHT );
*/
  error = register_scalers(); if( error ) return error;

  /* Redraw the entire screen... */
  display_refresh_all();

  return 0;
}

int
register_scalers( void )
{
  scaler_register_clear();

  switch( win32display_current_size ) {

  case 1:

    switch( image_scale ) {
    case 1:
      scaler_register( SCALER_NORMAL );
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
      if( !scaler_is_supported( current_scaler ) )
	scaler_select_scaler( SCALER_DOUBLESIZE );
      return 0;
    case 2:
      scaler_register( SCALER_NORMAL );
      scaler_register( SCALER_TIMEXTV );
      if( !scaler_is_supported( current_scaler ) )
	scaler_select_scaler( SCALER_NORMAL );
      return 0;
    }

  case 3:

    switch( image_scale ) {
    case 1:
      scaler_register( SCALER_TRIPLESIZE );
      scaler_register( SCALER_ADVMAME3X );
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

/* TODO: implement settings
  palette = settings_current.bw_tv ? bw_colours : win32display_colours;
*/
  palette = win32display_colours;

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

unsigned int colour_pal[] = {
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

void win32display_area(int x, int y, int width, int height)
{
  int disp_x,disp_y;
  DWORD pix_colour;

  HDC dc;

  IDirectDrawSurface_GetDC( lpdds2, &dc );

  for(disp_x=x;disp_x < x+width;disp_x++)
  {
    for(disp_y=y;disp_y < y+height;disp_y++)
    {
      pix_colour =
	colour_pal[3*win32display_image[disp_y][disp_x]] +
	colour_pal[(1+(3*win32display_image[disp_y][disp_x]))] * 256 +
	colour_pal[(2+(3*win32display_image[disp_y][disp_x]))] * 256 * 256;
      SetPixel( dc, disp_x, disp_y, pix_colour);
    }
  }

  IDirectDrawSurface_ReleaseDC( lpdds2, dc );
 return;
/*
  int disp_x,disp_y;
  libspectrum_dword pix_colour;
  char *buf = &scaled_image[ y * scaled_pitch + 4 * x ];

  HDC dc;

  /* get DC of off-screen buffer */
/*
 IDirectDrawSurface_GetDC( lpdds2, &dc );

  for(disp_x=x;disp_x < x+width;disp_x++)
  {
    for(disp_y=y;disp_y < y+height;disp_y++)
    {
      pix_colour =
	buf[ disp_y * scaled_pitch + disp_x * 4 + 0] +
	buf[ disp_y * scaled_pitch + disp_x * 4 + 1] * 255 +
	buf[ disp_y * scaled_pitch + disp_x * 4 + 1] * 255 * 255;
      SetPixel( dc, disp_x, disp_y, pix_colour);
    }
  }

  IDirectDrawSurface_ReleaseDC( lpdds2, dc );
  return;
*/

}

int
uidisplay_hotswap_gfx_mode( void )
{
/* TODO: pause hangs emulator
  fuse_emulation_pause();
*/
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
