/* sdldisplay.c: Routines for dealing with the SDL display
   Copyright (c) 2000-2002 Philip Kendall, Matan Ziv-Av, Fredrick Meunier

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

#ifdef UI_SDL			/* Use this iff we're using SDL */

#include <stdio.h>
#include <string.h>
#include <SDL.h>

#include "fuse.h"
#include "display.h"
#include "ui/uidisplay.h"
#ifdef USE_WIDGET
#include "widget/widget.h"
#endif				/* #ifdef USE_WIDGET */
#include "scld.h"
#include "screenshot.h"
#include "settings.h"

static SDL_Surface *gc=NULL;
static SDL_Surface *image=NULL;

static const SDL_VideoInfo *vidinfo = NULL;

static Uint32 colour_values[16];

static SDL_Rect updated_rects[DISPLAY_SCREEN_HEIGHT];

static BYTE sdldisplay_dirty_table[DISPLAY_SCREEN_HEIGHT][DISPLAY_SCREEN_WIDTH];

static int num_rects = 0;

/* The current size of the display (in units of DISPLAY_SCREEN_*) */
static int sdldisplay_current_size = 1;

/* The last size of the window in windowed mode */
static int sdldisplay_last_window_size = 1;

static int sdldisplay_allocate_colours( int numColours, Uint32 *colour_values );
static int sdldisplay_allocate_image( int width, int height );
static void sdldisplay_area( int x, int y, int width, int height );

int
uidisplay_init( int width, int height )
{
  vidinfo = SDL_GetVideoInfo();
  gc = SDL_SetVideoMode( width, height, vidinfo->vfmt->BitsPerPixel, SDL_SWSURFACE|SDL_ANYFORMAT|SDL_HWPALETTE|SDL_RESIZABLE );
  if ( gc == NULL ) return 1;

  SDL_WM_SetCaption( "Fuse", "Fuse" );

  sdldisplay_allocate_image( DISPLAY_ASPECT_WIDTH, DISPLAY_SCREEN_HEIGHT );
  sdldisplay_allocate_colours( 16, colour_values );
  
  memset( sdldisplay_dirty_table, 0, DISPLAY_SCREEN_HEIGHT * DISPLAY_SCREEN_WIDTH );

  return 0;
}

static int
sdldisplay_allocate_colours( int numColours, Uint32 *colour_values )
{
  SDL_Color colour_palette[] = {
  { 000, 000, 000, 000 }, 
  { 000, 000, 192, 000 }, 
  { 192, 000, 000, 000 }, 
  { 192, 000, 192, 000 }, 
  { 000, 192, 000, 000 }, 
  { 000, 192, 192, 000 }, 
  { 192, 192, 000, 000 }, 
  { 192, 192, 192, 000 }, 
  { 000, 000, 000, 000 }, 
  { 000, 000, 255, 000 }, 
  { 255, 000, 000, 000 }, 
  { 255, 000, 255, 000 }, 
  { 000, 255, 000, 000 }, 
  { 000, 255, 255, 000 }, 
  { 255, 255, 000, 000 }, 
  { 255, 255, 255, 000 }
  };

  int i;

  SDL_SetColors(image, colour_palette, 0, 16);

  for( i = 0; i < numColours; i++ ) {
    colour_values[i] = SDL_MapRGB( image->format,colour_palette[i].r,
                              colour_palette[i].g, colour_palette[i].b );
  }

  return 0;
}
  
static
int sdldisplay_allocate_image(int width, int height)
{
  image = SDL_CreateRGBSurface( SDL_SWSURFACE, 2*width, 2*height, 8, 0, 0, 0, 0 );

  if( !image ) {
    fprintf( stderr, "%s: couldn't create image\n", fuse_progname );
    return 1;
  }

  return 0;
}

int
uidisplay_toggle_fullscreen( void )
{
  int colour, vid_flags;

  fuse_emulation_pause();

  colour = scld_hires ? display_hires_border : display_lores_border;

  settings_current.full_screen = !settings_current.full_screen;

  vid_flags = SDL_SWSURFACE|SDL_ANYFORMAT|SDL_HWPALETTE;

  if ( settings_current.full_screen ) {
    sdldisplay_current_size = 2;
    vid_flags |= SDL_FULLSCREEN;
  } else {
    sdldisplay_current_size = sdldisplay_last_window_size;
    vid_flags |= SDL_RESIZABLE;
  }

  gc = SDL_SetVideoMode(
      DISPLAY_ASPECT_WIDTH * sdldisplay_current_size,
      DISPLAY_SCREEN_HEIGHT * sdldisplay_current_size,
      vidinfo->vfmt->BitsPerPixel,
      vid_flags
    );

/* Mac OS X resets the state of the cursor after a switch to full screen mode */
  if ( settings_current.full_screen ) {
    SDL_ShowCursor( SDL_DISABLE );
  } else {
    SDL_ShowCursor( SDL_ENABLE );
  }

  /* Redraw the entire screen... */
  display_refresh_all();

  /* And the entire border */
  display_refresh_border();

  /* If widgets are active, redraw the widget */
  if( widget_level >= 0 ) widget_keyhandler( KEYBOARD_Resize, KEYBOARD_NONE );

  fuse_emulation_unpause();

  return 0;
}

int
sdldisplay_resize_event( SDL_ResizeEvent *resize )
{
  int size, colour;

  fuse_emulation_pause();

  colour = scld_hires ? display_hires_border : display_lores_border;
  size = resize->w / DISPLAY_ASPECT_WIDTH;

  if( size < 1 ) {
  /* If we are being resized to smaller than a speccy screen go back to a
   * sensible size */
    size = 1;
  } else if( size > 2 ) {
  /* If we are being resized larger than we can support go back to our
   * maximum size */
    size = 2;
  }

  sdldisplay_current_size = sdldisplay_last_window_size = size;

  gc = SDL_SetVideoMode(
      DISPLAY_ASPECT_WIDTH * sdldisplay_current_size,
      DISPLAY_SCREEN_HEIGHT * sdldisplay_current_size,
      vidinfo->vfmt->BitsPerPixel,
      SDL_SWSURFACE|SDL_ANYFORMAT|SDL_HWPALETTE|SDL_RESIZABLE
    );


  /* Redraw the entire screen... */
  display_refresh_all();

  /* And the entire border */
  display_refresh_border();

  /* If widgets are active, redraw the widget */
  if( widget_level >= 0 ) widget_keyhandler( KEYBOARD_Resize, KEYBOARD_NONE );

  fuse_emulation_unpause();

  return 0;

}

static void
sdldisplay_putpixel( int x, int y, int colour )
{
  int bpp = image->format->BytesPerPixel;
  Uint8 *p = (Uint8 *)image->pixels + y*image->pitch + x*bpp;

  switch( bpp ) {
  case 1:
    *p = colour;
    break;

  case 2:
    *(Uint16 *)p = colour;
    break;

  case 3:
    if( SDL_BYTEORDER == SDL_BIG_ENDIAN ) {
	  p[0] = ( colour >> 16 ) & 0xff;
	  p[1] = ( colour >> 8 ) & 0xff;
	  p[2] = colour & 0xff;
    } else {
	  p[0] = colour & 0xff;
	  p[1] = ( colour >> 8 ) & 0xff;
	  p[2] = ( colour >> 16 ) & 0xff;
    }
    break;

  case 4:
    *(Uint32 *)p = colour;
    break;
  }
}

void
uidisplay_putpixel( int x, int y, int colour )
{
#ifdef HAVE_PNG_H
  screenshot_screen[y][x] = colour;
#endif                 /* #ifdef HAVE_PNG_H */

  sdldisplay_dirty_table[y][x] = 1;

  switch( sdldisplay_current_size ) {
  case 1:
    if( x%2!=0 ) return;
    if( SDL_MUSTLOCK( image ) ) SDL_LockSurface( image );
    sdldisplay_putpixel( x>>1, y, colour_values[colour] );
    if( SDL_MUSTLOCK( image ) ) SDL_UnlockSurface( image );
    break;
  case 2:
    if( SDL_MUSTLOCK( image ) ) SDL_LockSurface( image );
    sdldisplay_putpixel( x, y<<1 , colour_values[colour] );
    sdldisplay_putpixel( x, ( y<<1 ) + 1, colour_values[colour] );
    if( SDL_MUSTLOCK( image ) ) SDL_UnlockSurface( image );
    break;
  }
}

void
uidisplay_lines( int start, int end )
{
  int row, col;
  int x1 = DISPLAY_SCREEN_WIDTH - 1;
  int x2 = 0;
  
/* Process lines from start to end in reference to the sdldisplay_dirty_table */
  for( row = start; row <= end; row++) {
  
/* Find and record first change in this row */
    for( col = 0; ( col < x1 ) && ( col < DISPLAY_SCREEN_WIDTH ); col++ ) {
      if( sdldisplay_dirty_table[ row ][ col ] == 1 ) {
        x1 = col;
        break;
      }
    }
    
/* Find and record last change in this row */
    for( col = DISPLAY_SCREEN_WIDTH - 1; ( col > x2 ) && ( col >= 0 ); col-- ) {
      if( sdldisplay_dirty_table[ row ][ col ] == 1 ) {
        x2 = col;
        break;
      }
    }
    
    if ( ( x1 == 0 ) && ( x2 == DISPLAY_SCREEN_WIDTH - 1 ) ) break;
  }

  switch( sdldisplay_current_size ) {
  case 1:
    x1 >>= 1;
    x2 >>= 1;
    break;
  case 2:
    break;
  }

  memset( sdldisplay_dirty_table[start], 0, (end - start + 1) * DISPLAY_SCREEN_WIDTH );

  sdldisplay_area( x1, sdldisplay_current_size * start,
                   x2 - x1 + 1,
                   sdldisplay_current_size * ( end - start + 1 ) );

}

void
uidisplay_frame_end( void )
{
  int i;

  if (num_rects > 0) {
    for( i = 0; i < num_rects; i++ )
      SDL_BlitSurface( image, &updated_rects[i], gc, &updated_rects[i] );
    SDL_UpdateRects( gc, num_rects, updated_rects );
    num_rects = 0; 
  }
}

void
sdldisplay_area( int x, int y, int width, int height )
{
  updated_rects[num_rects].x = x;
  updated_rects[num_rects].y = y;
  updated_rects[num_rects].w = width;
  updated_rects[num_rects].h = height;

  num_rects++;
}

void
uidisplay_set_border( int line, int pixel_from, int pixel_to, int colour )
{
  int x;

  for( x = pixel_from; x < pixel_to; x++ )
    uidisplay_putpixel( x, line, colour );
}

int
uidisplay_end( void )
{
    SDL_FreeSurface( image );
    return 0;
}

#endif				/* #ifdef UI_SDL */
