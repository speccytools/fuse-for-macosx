/* sdl2_display_internal.h: SDL2 presentation helpers
   Copyright (c) 2026 Fredrick Meunier

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 */

#ifndef FUSE_SDL2_DISPLAY_INTERNAL_H
#define FUSE_SDL2_DISPLAY_INTERNAL_H

#include "config.h"

#include "ui/scaler/scaler.h"

typedef struct sdl2_surface_view {
  void *pixels;
  int width;
  int height;
  int pitch;
  int bytes_per_pixel;
} sdl2_surface_view;

typedef struct sdl2_display_rect {
  int x;
  int y;
  int w;
  int h;
} sdl2_display_rect;

typedef struct sdl2_fullscreen_mode_info {
  int width;
  int height;
  int refresh_rate;
  float fit;
} sdl2_fullscreen_mode_info;

static inline void
sdl2display_surface_view_init( sdl2_surface_view *view, void *pixels,
                               int width, int height, int pitch,
                               int bytes_per_pixel )
{
  view->pixels = pixels;
  view->width = width;
  view->height = height;
  view->pitch = pitch;
  view->bytes_per_pixel = bytes_per_pixel;
}

#ifdef UI_SDL2
#include <SDL.h>

static inline void
sdl2display_surface_view_from_sdl( SDL_Surface *surface,
                                   sdl2_surface_view *view )
{
  sdl2display_surface_view_init( view, surface->pixels, surface->w,
                                 surface->h, surface->pitch,
                                 surface->format->BytesPerPixel );
}
#endif

static inline void
sdl2display_scale_rect( const sdl2_display_rect *src, float scale,
                        int x_off, int y_off, sdl2_display_rect *dst )
{
  dst->x = x_off + (int)( src->x * scale );
  dst->y = y_off + (int)( src->y * scale );
  dst->w = (int)( src->w * scale );
  dst->h = (int)( src->h * scale );
}

static inline void
sdl2display_icon_rect( int x, int y, int width, int height, float scale,
                       int x_off, int y_off, sdl2_display_rect *dst )
{
  sdl2_display_rect src;

  src.x = x;
  src.y = y;
  src.w = width;
  src.h = height;

  sdl2display_scale_rect( &src, scale, x_off, y_off, dst );
}

static inline void
sdl2display_update_rect( int x, int y, int width, int height, float scale,
                         int x_off, int y_off, sdl2_display_rect *dst )
{
  sdl2_display_rect src;

  src.x = x;
  src.y = y;
  src.w = width;
  src.h = height;

  sdl2display_scale_rect( &src, scale, x_off, y_off, dst );
}

void sdl2display_compute_offsets( int window_width, int window_height,
                                  int image_width, int image_height,
                                  float scale, int is_fullscreen,
                                  int *x_off, int *y_off );

scaler_type sdl2display_choose_fullscreen_scaler( scaler_type current,
                                                  float current_scale,
                                                  int image_height,
                                                  int display_height,
                                                  const unsigned char *supported,
                                                  const float *scales,
                                                  int scaler_count,
                                                  int *preserve_windowed );

void sdl2display_get_preferred_refresh( int refresh_rate, int *tier,
                                        int *delta );
float sdl2display_mode_fit( scaler_type current, int image_width,
                            int image_height,
                            int width, int height,
                            const unsigned char *supported,
                            const float *scales, int scaler_count );
int sdl2display_compare_refresh( int left_refresh, int right_refresh );
int sdl2display_compare_mode_info( const sdl2_fullscreen_mode_info *left,
                                   const sdl2_fullscreen_mode_info *right );

#endif
