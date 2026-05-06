/* sdl2_display_internal.c: SDL2 presentation helpers
   Copyright (c) 2026 Fredrick Meunier

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 */

#include "config.h"

#include <limits.h>

#include "ui/scaler/scaler.h"

#include "sdl2_display_internal.h"

void
sdl2display_compute_offsets( int window_width, int window_height,
                             int image_width, int image_height,
                             float scale, int is_fullscreen,
                             int *x_off, int *y_off )
{
  int scaled_width;
  int scaled_height;

  if( !is_fullscreen ) {
    *x_off = 0;
    *y_off = 0;
    return;
  }

  scaled_width = (int)( image_width * scale );
  scaled_height = (int)( image_height * scale );

  *x_off = ( window_width - scaled_width ) / 2;
  *y_off = ( window_height - scaled_height ) / 2;
}

scaler_type
sdl2display_choose_fullscreen_scaler( scaler_type current,
                                      float current_scale,
                                      int image_height,
                                      int display_height,
                                      const unsigned char *supported,
                                      const float *scales,
                                      int scaler_count,
                                      int *preserve_windowed )
{
  scaler_type i;

  *preserve_windowed = 0;

  if( !display_height ) return current;

  if( image_height * current_scale > display_height / 2 &&
      image_height * current_scale <= display_height ) {
    return current;
  }

  *preserve_windowed = 1;

  for( i = 0; i < scaler_count; i++ ) {
    if( !supported[i] ) continue;

    if( image_height * scales[i] > display_height / 2 &&
        image_height * scales[i] <= display_height ) {
      return i;
    }
  }

  return SCALER_NORMAL;
}

void
sdl2display_get_preferred_refresh( int refresh_rate, int *tier, int *delta )
{
  if( refresh_rate <= 0 ) {
    *tier = 5;
    *delta = INT_MAX;
    return;
  }

  if( refresh_rate == 50 ) {
    *tier = 0;
    *delta = 0;
  } else if( refresh_rate > 50 && refresh_rate % 50 == 0 ) {
    *tier = 1;
    *delta = refresh_rate - 50;
  } else if( refresh_rate == 60 ) {
    *tier = 2;
    *delta = 0;
  } else if( refresh_rate > 60 && refresh_rate % 60 == 0 ) {
    *tier = 3;
    *delta = refresh_rate - 60;
  } else {
    int delta50 = refresh_rate > 50 ? refresh_rate - 50 : 50 - refresh_rate;
    int delta60 = refresh_rate > 60 ? refresh_rate - 60 : 60 - refresh_rate;

    *tier = 4;
    *delta = delta50 < delta60 ? delta50 : delta60;
  }
}

float
sdl2display_mode_fit( scaler_type current, int image_width, int image_height,
                      int width, int height,
                      const unsigned char *supported,
                      const float *scales, int scaler_count )
{
  int preserve_windowed;
  scaler_type scaler;
  float scale;

  scaler = sdl2display_choose_fullscreen_scaler( current, scales[current],
                                                 image_height, height,
                                                 supported, scales,
                                                 scaler_count,
                                                 &preserve_windowed );
  scale = scales[ scaler ];

  if( image_width * scale > width ||
      image_height * scale > height ) return 0.0f;

  return ( image_width * scale * image_height * scale ) / ( width * height );
}

int
sdl2display_compare_refresh( int left_refresh, int right_refresh )
{
  int left_tier, left_delta, right_tier, right_delta;

  sdl2display_get_preferred_refresh( left_refresh, &left_tier, &left_delta );
  sdl2display_get_preferred_refresh( right_refresh, &right_tier, &right_delta );

  if( left_tier != right_tier ) return left_tier - right_tier;
  if( left_delta != right_delta ) return left_delta - right_delta;

  return left_refresh - right_refresh;
}

int
sdl2display_compare_mode_info( const sdl2_fullscreen_mode_info *left,
                               const sdl2_fullscreen_mode_info *right )
{
  int refresh_cmp;

  if( left->fit < right->fit ) return 1;
  if( left->fit > right->fit ) return -1;

  refresh_cmp = sdl2display_compare_refresh( left->refresh_rate,
                                             right->refresh_rate );
  if( refresh_cmp ) return refresh_cmp;

  if( left->height != right->height ) return right->height - left->height;
  if( left->width != right->width ) return right->width - left->width;

  return 0;
}
