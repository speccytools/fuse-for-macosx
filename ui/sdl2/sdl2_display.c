/* sdl2_display.c: SDL 2 display handling
   Copyright (c) 2026 Fredrick Meunier

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "config.h"

#include <limits.h>
#include <stdlib.h>
#include <SDL.h>
#include <string.h>

#include "display.h"
#include "fuse.h"
#include "machine.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/scaler/scaler.h"
#include "ui/uidisplay.h"
#include "utils.h"
#include "sdl2_display_internal.h"
#include "sdl2_display.h"

static SDL_Window *sdl2_window;
static SDL_Surface *sdl2_window_surface;
static SDL_Surface *tmp_screen;
static SDL_Surface *scaled_screen;
static SDL_Surface *saved;
static libspectrum_byte sdl2display_is_full_screen;
static int fullscreen_x_off;
static int fullscreen_y_off;
static int fullscreen_width;
static int fullscreen_height;

static SDL_Surface *red_cassette[ 2 ], *green_cassette[ 2 ];
static SDL_Surface *red_mdr[ 2 ], *green_mdr[ 2 ];
static SDL_Surface *red_disk[ 2 ], *green_disk[ 2 ];

static ui_statusbar_state sdl2_disk_state, sdl2_mdr_state, sdl2_tape_state;
static int sdl2_status_updated;

static Uint32 colour_values[ 16 ];
static Uint32 bw_values[ 16 ];

static SDL_Rect updated_rects[ 300 ];
static int num_rects;
static libspectrum_byte sdl2display_force_full_refresh = 1;

static int image_width;
static int image_height;
static int tmp_screen_width;
static float sdl2display_current_size = 1.0f;

static void sdl2display_init_fullscreen_mode( void );
static int sdl2display_get_fullscreen_modes(
  sdl2_fullscreen_mode_info **modes_out,
  int *count_out );

static SDL_Color colour_palette[] = {
  {   0,   0,   0,   0 },
  {   0,   0, 192,   0 },
  { 192,   0,   0,   0 },
  { 192,   0, 192,   0 },
  {   0, 192,   0,   0 },
  {   0, 192, 192,   0 },
  { 192, 192,   0,   0 },
  { 192, 192, 192,   0 },
  {   0,   0,   0,   0 },
  {   0,   0, 255,   0 },
  { 255,   0,   0,   0 },
  { 255,   0, 255,   0 },
  {   0, 255,   0,   0 },
  {   0, 255, 255,   0 },
  { 255, 255,   0,   0 },
  { 255, 255, 255,   0 }
};

static void
init_scalers( void )
{
  scaler_register_clear();

  scaler_register( SCALER_NORMAL );
  scaler_register( SCALER_2XSAI );
  scaler_register( SCALER_SUPER2XSAI );
  scaler_register( SCALER_SUPEREAGLE );
  scaler_register( SCALER_ADVMAME2X );
  scaler_register( SCALER_ADVMAME3X );
  scaler_register( SCALER_DOTMATRIX );
  scaler_register( SCALER_PALTV );
  scaler_register( SCALER_HQ2X );
  if( machine_current->timex ) {
    scaler_register( SCALER_HALF );
    scaler_register( SCALER_HALFSKIP );
    scaler_register( SCALER_TIMEXTV );
    scaler_register( SCALER_TIMEX1_5X );
    scaler_register( SCALER_TIMEX2X );
  } else {
    scaler_register( SCALER_DOUBLESIZE );
    scaler_register( SCALER_TRIPLESIZE );
    scaler_register( SCALER_QUADSIZE );
    scaler_register( SCALER_TV2X );
    scaler_register( SCALER_TV3X );
    scaler_register( SCALER_TV4X );
    scaler_register( SCALER_PALTV2X );
    scaler_register( SCALER_PALTV3X );
    scaler_register( SCALER_PALTV4X );
    scaler_register( SCALER_HQ3X );
    scaler_register( SCALER_HQ4X );
  }

  if( scaler_is_supported( current_scaler ) ) {
    scaler_select_scaler( current_scaler );
  } else {
    scaler_select_scaler( SCALER_NORMAL );
  }
}

static int
sdl2display_compare_fullscreen_modes( const void *left_raw,
                                      const void *right_raw )
{
  const sdl2_fullscreen_mode_info *left = left_raw;
  const sdl2_fullscreen_mode_info *right = right_raw;

  return sdl2display_compare_mode_info( left, right );
}

static int
sdl2display_get_fullscreen_modes( sdl2_fullscreen_mode_info **modes_out,
                                  int *count_out )
{
  SDL_DisplayMode mode;
  sdl2_fullscreen_mode_info *modes;
  unsigned char supported[ SCALER_NUM ];
  float scales[ SCALER_NUM ];
  int num_modes;
  int count;
  int i;

  *modes_out = NULL;
  *count_out = 0;

  num_modes = SDL_GetNumDisplayModes( 0 );
  if( num_modes < 1 ) return 1;

  modes = libspectrum_new0( sdl2_fullscreen_mode_info, num_modes );
  if( !modes ) {
    fprintf( stderr, "%s: out of memory enumerating SDL2 fullscreen modes\n",
             fuse_progname );
    fuse_abort();
  }

  count = 0;

  for( i = 0; i < SCALER_NUM; i++ ) {
    supported[i] = scaler_is_supported( i );
    scales[i] = scaler_get_scaling_factor( i );
  }

  for( i = 0; i < num_modes; i++ ) {
    int j;

    if( SDL_GetDisplayMode( 0, i, &mode ) ) continue;

    for( j = 0; j < count; j++ ) {
      if( modes[j].width == mode.w && modes[j].height == mode.h ) {
        if( sdl2display_compare_refresh( mode.refresh_rate,
                                         modes[j].refresh_rate ) < 0 ) {
          modes[j].refresh_rate = mode.refresh_rate;
        }
        break;
      }
    }

    if( j == count ) {
      modes[count].width = mode.w;
      modes[count].height = mode.h;
      modes[count].refresh_rate = mode.refresh_rate;
      modes[count].fit = sdl2display_mode_fit( current_scaler, image_width,
                                               image_height, mode.w, mode.h,
                                               supported, scales, SCALER_NUM );
      count++;
    }
  }

  qsort( modes, count, sizeof( *modes ), sdl2display_compare_fullscreen_modes );

  *modes_out = modes;
  *count_out = count;
  return 0;
}

static void
sdl2display_refresh_window_surface( void )
{
  sdl2_window_surface = SDL_GetWindowSurface( sdl2_window );
  if( !sdl2_window_surface ) {
    fprintf( stderr, "%s: couldn't get SDL2 window surface: %s\n",
             fuse_progname, SDL_GetError() );
    fuse_abort();
  }
}

static void
sdl2display_init_fullscreen_mode( void )
{
  sdl2_fullscreen_mode_info *modes;
  int num_modes;
  int i;
  int mw = 0, mh = 0, mn = 0;

  fullscreen_width = 0;
  fullscreen_height = 0;

  if( sdl2display_get_fullscreen_modes( &modes, &num_modes ) ) {
    modes = NULL;
    num_modes = 0;
  }

  if( !settings_current.sdl_fullscreen_mode ) {
    if( num_modes > 0 ) {
      fullscreen_width = modes[0].width;
      fullscreen_height = modes[0].height;
    }
    free( modes );
    return;
  }

  if( strcmp( settings_current.sdl_fullscreen_mode, "list" ) == 0 ) {
    fprintf( stderr,
             "=====================================================================\n"
             " List of available SDL2 fullscreen modes:\n"
             "---------------------------------------------------------------------\n"
             "  No. width height\n"
             "---------------------------------------------------------------------\n"
             );
    if( num_modes < 1 ) {
      fprintf( stderr, "  ** The modes list is empty...\n" );
    } else {
      for( i = 0; i < num_modes; i++ ) {
        fprintf( stderr, "% 3d  % 5d % 5d",
                 i + 1, modes[i].width, modes[i].height );
        if( modes[i].refresh_rate > 0 )
          fprintf( stderr, " % 4dHz", modes[i].refresh_rate );
        if( modes[i].fit > 0.0f )
          fprintf( stderr, "  fit:%3.0f%%", modes[i].fit * 100.0f );
        fprintf( stderr, "\n" );
      }
    }
    fprintf( stderr,
             "=====================================================================\n" );
    free( modes );
    fuse_exiting = 1;
    return;
  }

  if( sscanf( settings_current.sdl_fullscreen_mode, " %dx%d", &mw,
              &mh ) != 2 ){
    if( num_modes > 0 &&
        sscanf( settings_current.sdl_fullscreen_mode, " %d", &mn ) == 1 &&
        mn > 0 && mn <= num_modes ) {
      mw = modes[mn - 1].width;
      mh = modes[mn - 1].height;
    }
  }

  if( mh > 0 ) {
    fullscreen_width = mw;
    fullscreen_height = mh;
  }

  free( modes );
}

static void
sdl2display_allocate_colours( void )
{
  int i;

  for( i = 0; i < 16; i++ ) {
    Uint8 red = colour_palette[i].r;
    Uint8 green = colour_palette[i].g;
    Uint8 blue = colour_palette[i].b;
    Uint8 grey = ( 0.299 * red + 0.587 * green + 0.114 * blue ) + 0.5;

    colour_values[i] = SDL_MapRGB( tmp_screen->format, red, green, blue );
    bw_values[i] = SDL_MapRGB( tmp_screen->format, grey, grey, grey );
  }
}

static int
sdl2display_convert_icon( SDL_Surface *source, SDL_Surface **icon, int red )
{
  SDL_Surface *copy;
  int i;
  SDL_Color colors[ source->format->palette->ncolors ];

  copy = SDL_ConvertSurface( source, source->format, 0 );
  if( !copy ) return -1;

  for( i = 0; i < copy->format->palette->ncolors; i++ ) {
    colors[i].r = red ? copy->format->palette->colors[i].r : 0;
    colors[i].g = red ? 0 : copy->format->palette->colors[i].g;
    colors[i].b = 0;
    colors[i].a = SDL_ALPHA_OPAQUE;
  }

  if( SDL_SetPaletteColors( copy->format->palette, colors, 0, i ) ) {
    SDL_FreeSurface( copy );
    return -1;
  }

  icon[0] = SDL_ConvertSurface( copy, tmp_screen->format, 0 );
  SDL_FreeSurface( copy );
  if( !icon[0] ) return -1;

  icon[1] = SDL_CreateRGBSurface( 0,
                                  icon[0]->w << 1, icon[0]->h << 1,
                                  icon[0]->format->BitsPerPixel,
                                  icon[0]->format->Rmask,
                                  icon[0]->format->Gmask,
                                  icon[0]->format->Bmask,
                                  icon[0]->format->Amask );
  if( !icon[1] ) return -1;

  ( scaler_get_proc16( SCALER_DOUBLESIZE ) )(
    (libspectrum_byte *)icon[0]->pixels,
    icon[0]->pitch,
    (libspectrum_byte *)icon[1]->pixels,
    icon[1]->pitch,
    icon[0]->w,
    icon[0]->h
    );

  return 0;
}

static int
sdl2display_load_status_icon( const char *filename,
                              SDL_Surface **red,
                              SDL_Surface **green )
{
  char path[ PATH_MAX ];
  SDL_Surface *temp;

  if( utils_find_file_path( filename, path, UTILS_AUXILIARY_LIB ) ) {
    fprintf( stderr, "%s: Error getting path for icons\n", fuse_progname );
    return -1;
  }

  temp = SDL_LoadBMP( path );
  if( !temp ) {
    fprintf( stderr, "%s: Error loading icon \"%s\" text:%s\n", fuse_progname,
             path, SDL_GetError() );
    return -1;
  }

  if( !temp->format->palette ) {
    fprintf( stderr, "%s: Icon \"%s\" is not paletted\n", fuse_progname, path );
    SDL_FreeSurface( temp );
    return -1;
  }

  if( sdl2display_convert_icon( temp, red, 1 ) ||
      sdl2display_convert_icon( temp, green, 0 ) ) {
    SDL_FreeSurface( temp );
    return -1;
  }

  SDL_FreeSurface( temp );
  return 0;
}

static void
sdl2display_free_status_icons( void )
{
  int i;

  for( i = 0; i < 2; i++ ) {
    if( red_cassette[i] ) {
      SDL_FreeSurface( red_cassette[i] ); red_cassette[i] = NULL;
    }
    if( green_cassette[i] ) {
      SDL_FreeSurface( green_cassette[i] ); green_cassette[i] = NULL;
    }
    if( red_mdr[i] ) {
      SDL_FreeSurface( red_mdr[i] ); red_mdr[i] = NULL;
    }
    if( green_mdr[i] ) {
      SDL_FreeSurface( green_mdr[i] ); green_mdr[i] = NULL;
    }
    if( red_disk[i] ) {
      SDL_FreeSurface( red_disk[i] ); red_disk[i] = NULL;
    }
    if( green_disk[i] ) {
      SDL_FreeSurface( green_disk[i] ); green_disk[i] = NULL;
    }
  }
}

static void
sdl2display_add_scaled_rect( int x, int y, int w, int h )
{
  if( sdl2display_force_full_refresh ) return;

  if( num_rects ==
      (int)( sizeof( updated_rects ) / sizeof( updated_rects[0] ) ) ){
    sdl2display_force_full_refresh = 1;
    return;
  }

  updated_rects[num_rects].x = x;
  updated_rects[num_rects].y = y;
  updated_rects[num_rects].w = w;
  updated_rects[num_rects].h = h;
  num_rects++;
}

static void
sdl2display_queue_status_rects( void )
{
  int scale = machine_current->timex ? 2 : 1;

  if( red_disk[ machine_current->timex ] ) {
    sdl2display_add_scaled_rect( 243 * scale, 218 * scale,
                                 red_disk[ machine_current->timex ]->w,
                                 red_disk[ machine_current->timex ]->h );
  }
  if( red_mdr[ machine_current->timex ] ) {
    sdl2display_add_scaled_rect( 264 * scale, 218 * scale,
                                 red_mdr[ machine_current->timex ]->w,
                                 red_mdr[ machine_current->timex ]->h );
  }
  if( red_cassette[ machine_current->timex ] ) {
    sdl2display_add_scaled_rect( 285 * scale, 220 * scale,
                                 red_cassette[ machine_current->timex ]->w,
                                 red_cassette[ machine_current->timex ]->h );
  }
}

static void
sdl2display_status_icon( SDL_Surface **icon, int x, int y )
{
  SDL_Rect dst_rect;
  sdl2_display_rect rect;
  SDL_Surface *surface = icon[ machine_current->timex ];

  if( !surface ) return;

  sdl2display_icon_rect( x, y, surface->w, surface->h,
                         sdl2display_current_size,
                         fullscreen_x_off, fullscreen_y_off, &rect );
  dst_rect.x = rect.x;
  dst_rect.y = rect.y;
  dst_rect.w = rect.w;
  dst_rect.h = rect.h;

  SDL_BlitScaled( surface, NULL, scaled_screen, &dst_rect );
}

static void
sdl2display_icon_overlay( void )
{
  if( !settings_current.statusbar ) {
    sdl2_status_updated = 0;
    return;
  }

  switch( sdl2_disk_state ) {
  case UI_STATUSBAR_STATE_ACTIVE:
    sdl2display_status_icon( green_disk, 243, 218 );
    break;
  case UI_STATUSBAR_STATE_INACTIVE:
    sdl2display_status_icon( red_disk, 243, 218 );
    break;
  case UI_STATUSBAR_STATE_NOT_AVAILABLE:
    break;
  }

  switch( sdl2_mdr_state ) {
  case UI_STATUSBAR_STATE_ACTIVE:
    sdl2display_status_icon( green_mdr, 264, 218 );
    break;
  case UI_STATUSBAR_STATE_INACTIVE:
    sdl2display_status_icon( red_mdr, 264, 218 );
    break;
  case UI_STATUSBAR_STATE_NOT_AVAILABLE:
    break;
  }

  switch( sdl2_tape_state ) {
  case UI_STATUSBAR_STATE_ACTIVE:
    sdl2display_status_icon( green_cassette, 285, 220 );
    break;
  case UI_STATUSBAR_STATE_INACTIVE:
  case UI_STATUSBAR_STATE_NOT_AVAILABLE:
    sdl2display_status_icon( red_cassette, 285, 220 );
    break;
  }

  sdl2_status_updated = 0;
}

static void
sdl2display_create_tmp_screen( void )
{
  Uint16 *tmp_screen_pixels;

  if( tmp_screen ) {
    libspectrum_free( tmp_screen->pixels );
    SDL_FreeSurface( tmp_screen );
    tmp_screen = NULL;
  }

  tmp_screen_width = image_width + 3;
  tmp_screen_pixels = libspectrum_new0( Uint16,
                                        tmp_screen_width * ( image_height + 3 ) );
  if( !tmp_screen_pixels ) {
    fprintf( stderr, "%s: couldn't allocate SDL2 tmp_screen\n", fuse_progname );
    fuse_abort();
  }

  tmp_screen = SDL_CreateRGBSurfaceFrom( tmp_screen_pixels,
                                         tmp_screen_width,
                                         image_height + 3,
                                         16,
                                         tmp_screen_width * 2,
                                         0xf800,
                                         0x07e0,
                                         0x001f,
                                         0x0000 );
  if( !tmp_screen ) {
    fprintf( stderr, "%s: couldn't create SDL2 tmp_screen: %s\n",
             fuse_progname, SDL_GetError() );
    fuse_abort();
  }

  scaler_select_bitformat( 565 );
  sdl2display_allocate_colours();
}

static void
sdl2display_create_scaled_screen( int width, int height )
{
  if( scaled_screen ) {
    SDL_FreeSurface( scaled_screen );
    scaled_screen = NULL;
  }

  scaled_screen = SDL_CreateRGBSurface( 0, width, height, 16,
                                        0xf800, 0x07e0, 0x001f, 0x0000 );
  if( !scaled_screen ) {
    fprintf( stderr, "%s: couldn't create SDL2 scaled screen: %s\n",
             fuse_progname, SDL_GetError() );
    fuse_abort();
  }
}

static void
sdl2display_sync_presentation_surfaces( void )
{
  sdl2display_refresh_window_surface();

  if( !scaled_screen ||
      scaled_screen->w != sdl2_window_surface->w ||
      scaled_screen->h != sdl2_window_surface->h ) {
    sdl2display_create_scaled_screen( sdl2_window_surface->w,
                                      sdl2_window_surface->h );
    sdl2display_force_full_refresh = 1;
  }

  sdl2display_compute_offsets( sdl2_window_surface->w, sdl2_window_surface->h,
                               image_width, image_height,
                               sdl2display_current_size,
                               sdl2display_is_full_screen,
                               &fullscreen_x_off, &fullscreen_y_off );
}

static void
sdl2display_find_best_fullscreen_scaler( void )
{
  static scaler_type windowed_scaler = SCALER_NUM;
  SDL_DisplayMode mode;
  int display_height;
  scaler_type i, target_scaler;
  unsigned char supported[ SCALER_NUM ];
  float scales[ SCALER_NUM ];
  int preserve_windowed;

  if( settings_current.full_screen ) {
    display_height = 0;
    if( fullscreen_height ) {
      display_height = fullscreen_height;
    } else if( !SDL_GetDesktopDisplayMode( 0, &mode ) ) {
      display_height = mode.h;
    }
    if( !display_height ) return;

    for( i = 0; i < SCALER_NUM; i++ ) {
      supported[i] = scaler_is_supported( i );
      scales[i] = scaler_get_scaling_factor( i );
    }

    target_scaler = sdl2display_choose_fullscreen_scaler(
      current_scaler, sdl2display_current_size, image_height, display_height,
      supported, scales, SCALER_NUM, &preserve_windowed );

    if( preserve_windowed && windowed_scaler == SCALER_NUM )
      windowed_scaler = current_scaler;

    scaler_activate_scaler( target_scaler );
    sdl2display_current_size = scaler_get_scaling_factor( current_scaler );
  } else if( windowed_scaler != SCALER_NUM ) {
    scaler_activate_scaler( windowed_scaler );
    windowed_scaler = SCALER_NUM;
    sdl2display_current_size = scaler_get_scaling_factor( current_scaler );
  }
}

static void
sdl2display_create_window( void )
{
  Uint32 flags = SDL_WINDOW_SHOWN;
  int width;
  int height;

  if( sdl2_window ) {
    SDL_DestroyWindow( sdl2_window );
    sdl2_window = NULL;
    sdl2_window_surface = NULL;
  }

  sdl2display_current_size = scaler_get_scaling_factor( current_scaler );
  sdl2display_find_best_fullscreen_scaler();
  width = settings_current.full_screen && fullscreen_width ? fullscreen_width :
          image_width * sdl2display_current_size;
  height = settings_current.full_screen &&
           fullscreen_height ? fullscreen_height :
           image_height * sdl2display_current_size;

  sdl2_window = SDL_CreateWindow( "Fuse",
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  width,
                                  height,
                                  flags );
  if( !sdl2_window ) {
    fprintf( stderr, "%s: couldn't create SDL2 window: %s\n",
             fuse_progname, SDL_GetError() );
    fuse_abort();
  }

  if( settings_current.full_screen ) {
    if( fullscreen_width && fullscreen_height ) {
      SDL_DisplayMode mode;

      mode.w = fullscreen_width;
      mode.h = fullscreen_height;
      mode.format = 0;
      mode.refresh_rate = 0;
      mode.driverdata = NULL;

      if( SDL_SetWindowDisplayMode( sdl2_window, &mode ) ||
          SDL_SetWindowFullscreen( sdl2_window, SDL_WINDOW_FULLSCREEN ) ) {
        fprintf( stderr, "%s: couldn't set SDL2 fixed fullscreen mode: %s\n",
                 fuse_progname, SDL_GetError() );
        fuse_abort();
      }
    } else if( SDL_SetWindowFullscreen( sdl2_window,
                                        SDL_WINDOW_FULLSCREEN_DESKTOP ) ){
      fprintf( stderr, "%s: couldn't set SDL2 desktop fullscreen mode: %s\n",
               fuse_progname, SDL_GetError() );
      fuse_abort();
    }
  }

  sdl2display_is_full_screen = settings_current.full_screen;

  sdl2display_sync_presentation_surfaces();
}

static void
sdl2display_recreate( void )
{
  sdl2display_force_full_refresh = 1;
  sdl2display_create_window();
  sdl2display_create_tmp_screen();
  sdl2display_free_status_icons();
  sdl2display_load_status_icon( "cassette.bmp", red_cassette, green_cassette );
  sdl2display_load_status_icon( "microdrive.bmp", red_mdr, green_mdr );
  sdl2display_load_status_icon( "plus3disk.bmp", red_disk, green_disk );

  /* Reapply mouse grab state after recreating the SDL window. */
  if( ui_mouse_grabbed ) ui_mouse_grabbed = ui_mouse_grab( 0 );

  display_refresh_all();
}

SDL_Window *
sdl2display_get_window( void )
{
  return sdl2_window;
}

void
sdl2display_set_title( const char *title )
{
  if( sdl2_window ) SDL_SetWindowTitle( sdl2_window, title );
}

int
uidisplay_init( int width, int height )
{
  image_width = width;
  image_height = height;

  init_scalers();

  sdl2display_init_fullscreen_mode();
  if( fuse_exiting ) return 0;
  if( scaler_select_scaler( current_scaler ) )
    scaler_select_scaler( SCALER_NORMAL );

  sdl2display_recreate();
  display_ui_initialised = 1;

  return 0;
}

int
uidisplay_hotswap_gfx_mode( void )
{
  sdl2display_recreate();
  return 0;
}

void
uidisplay_frame_save( void )
{
  if( saved ) {
    SDL_FreeSurface( saved );
    saved = NULL;
  }

  saved = SDL_ConvertSurface( tmp_screen, tmp_screen->format, 0 );
}

void
uidisplay_frame_restore( void )
{
  if( saved ) {
    SDL_BlitSurface( saved, NULL, tmp_screen, NULL );
    sdl2display_force_full_refresh = 1;
  }
}

void
uidisplay_putpixel( int x, int y, int colour )
{
  libspectrum_word *dest_base, *dest;
  Uint32 *palette_values = settings_current.bw_tv ? bw_values : colour_values;
  Uint32 palette_colour = palette_values[ colour ];

  if( machine_current->timex ) {
    x <<= 1;
    y <<= 1;
    dest_base = dest =
      (libspectrum_word *)( (libspectrum_byte *)tmp_screen->pixels +
                            ( x + 1 ) * tmp_screen->format->BytesPerPixel +
                            ( y + 1 ) * tmp_screen->pitch );

    *( dest++ ) = palette_colour;
    *( dest++ ) = palette_colour;
    dest = (libspectrum_word *)( (libspectrum_byte *)dest_base +
                                 tmp_screen->pitch );
    *( dest++ ) = palette_colour;
    *( dest++ ) = palette_colour;
  } else {
    dest =
      (libspectrum_word *)( (libspectrum_byte *)tmp_screen->pixels +
                            ( x + 1 ) * tmp_screen->format->BytesPerPixel +
                            ( y + 1 ) * tmp_screen->pitch );
    *dest = palette_colour;
  }
}

void
uidisplay_plot8( int x, int y, libspectrum_byte data,
                 libspectrum_byte ink, libspectrum_byte paper )
{
  libspectrum_word *dest;
  Uint32 *palette_values = settings_current.bw_tv ? bw_values : colour_values;
  Uint32 palette_ink = palette_values[ ink ];
  Uint32 palette_paper = palette_values[ paper ];

  if( machine_current->timex ) {
    int i;
    libspectrum_word *dest_base;

    x <<= 4;
    y <<= 1;

    dest_base =
      (libspectrum_word *)( (libspectrum_byte *)tmp_screen->pixels +
                            ( x + 1 ) * tmp_screen->format->BytesPerPixel +
                            ( y + 1 ) * tmp_screen->pitch );

    for( i = 0; i < 2; i++ ) {
      dest = dest_base;

      *( dest++ ) = ( data & 0x80 ) ? palette_ink : palette_paper;
      *( dest++ ) = ( data & 0x80 ) ? palette_ink : palette_paper;
      *( dest++ ) = ( data & 0x40 ) ? palette_ink : palette_paper;
      *( dest++ ) = ( data & 0x40 ) ? palette_ink : palette_paper;
      *( dest++ ) = ( data & 0x20 ) ? palette_ink : palette_paper;
      *( dest++ ) = ( data & 0x20 ) ? palette_ink : palette_paper;
      *( dest++ ) = ( data & 0x10 ) ? palette_ink : palette_paper;
      *( dest++ ) = ( data & 0x10 ) ? palette_ink : palette_paper;
      *( dest++ ) = ( data & 0x08 ) ? palette_ink : palette_paper;
      *( dest++ ) = ( data & 0x08 ) ? palette_ink : palette_paper;
      *( dest++ ) = ( data & 0x04 ) ? palette_ink : palette_paper;
      *( dest++ ) = ( data & 0x04 ) ? palette_ink : palette_paper;
      *( dest++ ) = ( data & 0x02 ) ? palette_ink : palette_paper;
      *( dest++ ) = ( data & 0x02 ) ? palette_ink : palette_paper;
      *( dest++ ) = ( data & 0x01 ) ? palette_ink : palette_paper;
      *dest = ( data & 0x01 ) ? palette_ink : palette_paper;

      dest_base = (libspectrum_word *)( (libspectrum_byte *)dest_base +
                                        tmp_screen->pitch );
    }
  } else {
    x <<= 3;

    dest =
      (libspectrum_word *)( (libspectrum_byte *)tmp_screen->pixels +
                            ( x + 1 ) * tmp_screen->format->BytesPerPixel +
                            ( y + 1 ) * tmp_screen->pitch );

    *( dest++ ) = ( data & 0x80 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x40 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x20 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x10 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x08 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x04 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x02 ) ? palette_ink : palette_paper;
    *dest = ( data & 0x01 ) ? palette_ink : palette_paper;
  }
}

void
uidisplay_plot16( int x, int y, libspectrum_word data,
                  libspectrum_byte ink, libspectrum_byte paper )
{
  libspectrum_word *dest_base, *dest;
  int i;
  Uint32 *palette_values = settings_current.bw_tv ? bw_values : colour_values;
  Uint32 palette_ink = palette_values[ ink ];
  Uint32 palette_paper = palette_values[ paper ];

  x <<= 4;
  y <<= 1;
  dest_base =
    (libspectrum_word *)( (libspectrum_byte *)tmp_screen->pixels +
                          ( x + 1 ) * tmp_screen->format->BytesPerPixel +
                          ( y + 1 ) * tmp_screen->pitch );

  for( i = 0; i < 2; i++ ) {
    dest = dest_base;

    *( dest++ ) = ( data & 0x8000 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x4000 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x2000 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x1000 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x0800 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x0400 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x0200 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x0100 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x0080 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x0040 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x0020 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x0010 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x0008 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x0004 ) ? palette_ink : palette_paper;
    *( dest++ ) = ( data & 0x0002 ) ? palette_ink : palette_paper;
    *dest = ( data & 0x0001 ) ? palette_ink : palette_paper;

    dest_base = (libspectrum_word *)( (libspectrum_byte *)dest_base +
                                      tmp_screen->pitch );
  }
}

void
uidisplay_area( int x, int y, int width, int height )
{
  if( sdl2display_force_full_refresh ) return;

  if( num_rects ==
      (int)( sizeof( updated_rects ) / sizeof( updated_rects[0] ) ) ){
    sdl2display_force_full_refresh = 1;
    return;
  }

  if( scaler_flags & SCALER_FLAGS_EXPAND )
    scaler_expander( &x, &y, &width, &height, image_width, image_height );

  updated_rects[num_rects].x = x;
  updated_rects[num_rects].y = y;
  updated_rects[num_rects].w = width;
  updated_rects[num_rects].h = height;
  num_rects++;
}

void
uidisplay_frame_end( void )
{
  SDL_Rect *rects;
  int i;
  int full_refresh;

  if( !sdl2_window || !scaled_screen ) return;

  if( sdl2display_is_full_screen != settings_current.full_screen ) {
    if( uidisplay_hotswap_gfx_mode() ) {
      fprintf( stderr, "%s: Error switching to fullscreen\n", fuse_progname );
      fuse_abort();
    }
  }

  full_refresh = sdl2display_force_full_refresh;

  if( sdl2_status_updated ) sdl2display_queue_status_rects();

  if( sdl2display_force_full_refresh ) {
    num_rects = 1;
    updated_rects[0].x = 0;
    updated_rects[0].y = 0;
    updated_rects[0].w = image_width;
    updated_rects[0].h = image_height;
  }

  if( !( ui_widget_level >= 0 ) && num_rects == 0 &&
      !sdl2_status_updated ) return;

  sdl2display_sync_presentation_surfaces();

  if( SDL_MUSTLOCK( scaled_screen ) ) SDL_LockSurface( scaled_screen );

  if( full_refresh ) SDL_FillRect( scaled_screen, NULL, 0 );

  for( i = 0; i < num_rects; i++ ) {
    SDL_Rect *r = &updated_rects[i];
    sdl2_display_rect dst;

    scaler_proc16(
      (libspectrum_byte *)tmp_screen->pixels +
      ( r->x + 1 ) * tmp_screen->format->BytesPerPixel +
      ( r->y + 1 ) * tmp_screen->pitch,
      tmp_screen->pitch,
      (libspectrum_byte *)scaled_screen->pixels +
      ( fullscreen_x_off + (int)( r->x * sdl2display_current_size ) ) *
      scaled_screen->format->BytesPerPixel +
      ( fullscreen_y_off + (int)( r->y * sdl2display_current_size ) ) *
      scaled_screen->pitch,
      scaled_screen->pitch,
      r->w,
      r->h );

    sdl2display_update_rect( r->x, r->y, r->w, r->h,
                             sdl2display_current_size,
                             fullscreen_x_off, fullscreen_y_off, &dst );
    r->x = dst.x;
    r->y = dst.y;
    r->w = dst.w;
    r->h = dst.h;
  }

  if( SDL_MUSTLOCK( scaled_screen ) ) SDL_UnlockSurface( scaled_screen );

  sdl2display_icon_overlay();

  if( !full_refresh ) {
    for( i = 0; i < num_rects; i++ ) {
      SDL_Rect *r = &updated_rects[i];
      SDL_Rect dst_rect = *r;

      SDL_BlitSurface( scaled_screen, r, sdl2_window_surface, &dst_rect );
    }
  }

  if( full_refresh ) {
    SDL_BlitSurface( scaled_screen, NULL, sdl2_window_surface, NULL );
    updated_rects[0].x = 0;
    updated_rects[0].y = 0;
    updated_rects[0].w = sdl2_window_surface->w;
    updated_rects[0].h = sdl2_window_surface->h;
    num_rects = 1;
  }

  rects = num_rects ? updated_rects : NULL;
  SDL_UpdateWindowSurfaceRects( sdl2_window, rects, num_rects );

  num_rects = 0;
  sdl2display_force_full_refresh = 0;
}

int
uidisplay_end( void )
{
  display_ui_initialised = 0;

  if( tmp_screen ) {
    free( tmp_screen->pixels );
    SDL_FreeSurface( tmp_screen );
    tmp_screen = NULL;
  }

  if( scaled_screen ) {
    SDL_FreeSurface( scaled_screen );
    scaled_screen = NULL;
  }

  sdl2display_free_status_icons();

  if( saved ) {
    SDL_FreeSurface( saved );
    saved = NULL;
  }

  if( sdl2_window ) {
    SDL_DestroyWindow( sdl2_window );
    sdl2_window = NULL;
    sdl2_window_surface = NULL;
  }

  return 0;
}

int
ui_statusbar_update( ui_statusbar_item item, ui_statusbar_state state )
{
  switch( item ) {
  case UI_STATUSBAR_ITEM_DISK:
    sdl2_disk_state = state;
    sdl2_status_updated = 1;
    return 0;

  case UI_STATUSBAR_ITEM_PAUSED:
    return 0;

  case UI_STATUSBAR_ITEM_TAPE:
    sdl2_tape_state = state;
    sdl2_status_updated = 1;
    return 0;

  case UI_STATUSBAR_ITEM_MICRODRIVE:
    sdl2_mdr_state = state;
    sdl2_status_updated = 1;
    return 0;

  case UI_STATUSBAR_ITEM_MOUSE:
    return 0;
  }

  ui_error( UI_ERROR_ERROR, "Attempt to update unknown statusbar item %d",
            item );
  return 1;
}
