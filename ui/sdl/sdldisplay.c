/* sdldisplay.c: Routines for dealing with the SDL display
   Copyright (c) 2000-2003 Philip Kendall, Matan Ziv-Av, Fredrick Meunier

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
#include "ui/scaler/scaler.h"
#include "machine.h"

SDL_Surface *sdldisplay_gc = NULL;   /* Hardware screen */
static SDL_Surface *tmp_screen=NULL; /* Temporary screen for scalers */

static int tmp_screen_width;

static Uint32 colour_values[16];

static SDL_Color colour_palette[] = {
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

/* This is a rule of thumb for the maximum number of rects that can be updated
   each frame. If more are generated we just update the whole screen */
#define MAX_UPDATE_RECT 300
static SDL_Rect updated_rects[MAX_UPDATE_RECT];
static int num_rects = 0;
static BYTE sdldisplay_force_full_refresh = 1;

/* The current size of the display (in units of DISPLAY_SCREEN_*) */
static float sdldisplay_current_size = 1;

static BYTE sdldisplay_is_full_screen = 0;

static int image_width;
static int image_height;

static int timex;

static void init_scalers( void );
static int sdldisplay_allocate_colours( int numColours, Uint32 *colour_values );

static int sdldisplay_load_gfx_mode( void );

static void
init_scalers( void )
{
  scaler_register_clear();

  scaler_register( SCALER_NORMAL );
  scaler_register( SCALER_DOUBLESIZE );
  scaler_register( SCALER_TRIPLESIZE );
  scaler_register( SCALER_2XSAI );
  scaler_register( SCALER_SUPER2XSAI );
  scaler_register( SCALER_SUPEREAGLE );
  scaler_register( SCALER_ADVMAME2X );
  scaler_register( SCALER_DOTMATRIX );
  if( machine_current->timex ) {
    scaler_register( SCALER_HALF ); 
    scaler_register( SCALER_HALFSKIP );
    scaler_register( SCALER_TIMEXTV );
  } else {
    scaler_register( SCALER_TV2X );
  }
  
  if( scaler_is_supported( current_scaler ) ) {
    scaler_select_scaler( current_scaler );
  } else {
    scaler_select_scaler( SCALER_NORMAL );
  }
}

int
uidisplay_init( int width, int height )
{
  image_width = width;
  image_height = height;

  timex = machine_current->timex;

  init_scalers();
  sdldisplay_load_gfx_mode();

  SDL_WM_SetCaption( "Fuse", "Fuse" );

  /* We can now output error messages to our output device */
  display_ui_initialised = 1;

  return 0;
}

static int
sdldisplay_allocate_colours( int numColours, Uint32 *colour_values )
{
  int i;

  for( i = 0; i < numColours; i++ ) {
    colour_values[i] = SDL_MapRGB( tmp_screen->format, colour_palette[i].r,
                              colour_palette[i].g, colour_palette[i].b );
  }

  return 0;
}

static int
sdldisplay_load_gfx_mode( void )
{
  Uint16 *tmp_screen_pixels;

  sdldisplay_force_full_refresh = 1;

  tmp_screen = NULL;
  tmp_screen_width = (image_width + 3);

  sdldisplay_current_size = scaler_get_scaling_factor( current_scaler );

  /* Create the surface that contains the scaled graphics in 16 bit mode */
  sdldisplay_gc = SDL_SetVideoMode(
    image_width * sdldisplay_current_size,
    image_height * sdldisplay_current_size,
    16,
    settings_current.full_screen ? (SDL_FULLSCREEN|SDL_SWSURFACE)
                                 : SDL_SWSURFACE
  );
  if( !sdldisplay_gc ) {
    fprintf( stderr, "%s: couldn't create gc\n", fuse_progname );
    return 1;
  }

  /* Distinguish 555 and 565 mode */
  if( sdldisplay_gc->format->Rmask == 0x7C00 )
    scaler_select_bitformat( 555 );
  else
    scaler_select_bitformat( 565 );

  /* Create the surface used for the graphics in 16 bit before scaling */

  /* Need some extra bytes around when using 2xSaI */
  tmp_screen_pixels = (Uint16*)calloc(tmp_screen_width*(image_height+3), sizeof(Uint16));
  tmp_screen = SDL_CreateRGBSurfaceFrom(tmp_screen_pixels,
                                        tmp_screen_width,
                                        image_height + 3,
                                        16, tmp_screen_width*2,
                                        sdldisplay_gc->format->Rmask,
                                        sdldisplay_gc->format->Gmask,
                                        sdldisplay_gc->format->Bmask,
                                        sdldisplay_gc->format->Amask );

  if( !tmp_screen ) {
    fprintf( stderr, "%s: couldn't create tmp_screen\n", fuse_progname );
    return 1;
  }

  sdldisplay_allocate_colours( 16, colour_values );

  /* Redraw the entire screen... */
  display_refresh_all();

  return 0;
}

void
uidisplay_hotswap_gfx_mode( void )
{
  fuse_emulation_pause();

  /* Free the old surface */
  if( tmp_screen ) {
    free( tmp_screen->pixels );
    SDL_FreeSurface( tmp_screen ); tmp_screen = NULL;
  }

  /* Setup the new GFX mode */
  sdldisplay_load_gfx_mode();

  /* reset palette */
  SDL_SetColors( sdldisplay_gc, colour_palette, 0, 16 );

  /* Mac OS X resets the state of the cursor after a switch to full screen mode */
  if ( settings_current.full_screen )
    SDL_ShowCursor( SDL_DISABLE );
  else
    SDL_ShowCursor( SDL_ENABLE );

  /* Redraw the entire screen... */
  display_refresh_all();

  fuse_emulation_unpause();
}

void
uidisplay_frame_end( void )
{
  SDL_Rect *r;
  Uint32 tmp_screen_pitch, dstPitch;
  SDL_Rect *last_rect;

  /* We check for a switch to fullscreen here to give systems with a
     windowed-only UI a chance to free menu etc. resources before
     the switch to fullscreen (e.g. Mac OS X) */
  if( sdldisplay_is_full_screen != settings_current.full_screen ) {
    uidisplay_hotswap_gfx_mode();
    sdldisplay_is_full_screen = !sdldisplay_is_full_screen;
  }

  /* Force a full redraw if requested */
  if ( sdldisplay_force_full_refresh ) {
    num_rects = 1;

    updated_rects[0].x = 0;
    updated_rects[0].y = 0;
    updated_rects[0].w = image_width;
    updated_rects[0].h = image_height;
  }

#ifdef USE_WIDGET
  if ( !(widget_level >= 0) && num_rects == 0 ) return;
#else                   /* #ifdef USE_WIDGET */
  if ( num_rects == 0 ) return;
#endif                  /* #ifdef USE_WIDGET */

  if( SDL_MUSTLOCK( tmp_screen ) ) SDL_LockSurface( tmp_screen );

  tmp_screen_pitch = tmp_screen->pitch;

  last_rect = updated_rects + num_rects;

  for( r = updated_rects; r != last_rect; r++ ) {

    WORD *dest_base, *dest;
    size_t xx,yy;

    dest_base =
      (WORD*)( (BYTE*)tmp_screen->pixels + (r->x*2+2) +
	       (r->y+1)*tmp_screen_pitch );

    for( yy = r->y; yy < r->y + r->h; yy++ ) {

      for( xx = r->x, dest = dest_base; xx < r->x + r->w; xx++, dest++ )
	*dest = colour_values[ display_image[yy][xx] ];

      dest_base = (WORD*)( (BYTE*)dest_base + tmp_screen_pitch );
    }
	  
  }

  if( SDL_MUSTLOCK( sdldisplay_gc ) ) SDL_LockSurface( sdldisplay_gc );

  dstPitch = sdldisplay_gc->pitch;

  for( r = updated_rects; r != last_rect; ++r ) {
    register int dst_y = r->y * sdldisplay_current_size;
    register int dst_h = r->h;

    scaler_proc16(
     (BYTE*)tmp_screen->pixels + (r->x*2+2) + (r->y+1)*tmp_screen_pitch,
     tmp_screen_pitch, NULL,
     (BYTE*)sdldisplay_gc->pixels + r->x*(BYTE)(2*sdldisplay_current_size) +
     dst_y*dstPitch, dstPitch, r->w, dst_h );

    /* Adjust rects for the destination rect size */
    r->x *= sdldisplay_current_size;
    r->y = dst_y;
    r->w *= sdldisplay_current_size;
    r->h = dst_h * sdldisplay_current_size;
  }

  if( SDL_MUSTLOCK( tmp_screen ) ) SDL_UnlockSurface( tmp_screen );
  if( SDL_MUSTLOCK( sdldisplay_gc ) ) SDL_UnlockSurface( sdldisplay_gc );

  /* Finally, blit all our changes to the screen */
  SDL_UpdateRects( sdldisplay_gc, num_rects, updated_rects );

  num_rects = 0;
  sdldisplay_force_full_refresh = 0;
}

void
uidisplay_area( int x, int y, int width, int height )
{
  if ( sdldisplay_force_full_refresh )
    return;

  if( num_rects == MAX_UPDATE_RECT ) {
    sdldisplay_force_full_refresh = 1;
    return;
  }

  /* Extend the dirty region by 1 pixel for scalers
     that "smear" the screen, e.g. 2xSAI */
  if( scaler_flags & SCALER_FLAGS_EXPAND )
    scaler_expander( &x, &y, &width, &height, image_width, image_height );

  updated_rects[num_rects].x = x;
  updated_rects[num_rects].y = y;
  updated_rects[num_rects].w = width;
  updated_rects[num_rects].h = height;

  num_rects++;
}

int
uidisplay_end( void )
{
  display_ui_initialised = 0;
  if ( tmp_screen ) {
    free( tmp_screen->pixels );
    SDL_FreeSurface( tmp_screen ); tmp_screen = NULL;
  }
  return 0;
}

#endif        /* #ifdef UI_SDL */
