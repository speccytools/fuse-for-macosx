/* sdl2displaytest.c: Tests for SDL2 display helpers
   Copyright (c) 2026 Fredrick Meunier

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 */

#include <config.h>

#include <stdio.h>

#include "ui/scaler/scaler.h"
#include "ui/sdl2/sdl2_display_internal.h"

static void
fill_test_scales( float *scales )
{
  int i;

  for( i = 0; i < SCALER_NUM; i++ ) scales[i] = 0.0f;

  scales[ SCALER_HALF ] = 0.5f;
  scales[ SCALER_HALFSKIP ] = 0.5f;
  scales[ SCALER_NORMAL ] = 1.0f;
  scales[ SCALER_DOUBLESIZE ] = 2.0f;
  scales[ SCALER_TRIPLESIZE ] = 3.0f;
  scales[ SCALER_QUADSIZE ] = 4.0f;
}

static int
compute_offsets_windowed( void )
{
  int x_off, y_off;

  sdl2display_compute_offsets( 960, 720, 320, 240, 3.0f, 0, &x_off, &y_off );

  if( x_off != 0 || y_off != 0 ) {
    fprintf( stderr, "windowed offsets: expected 0,0 got %d,%d\n", x_off,
             y_off );
    return 1;
  }

  return 0;
}

static int
compute_offsets_fullscreen( void )
{
  int x_off, y_off;

  sdl2display_compute_offsets( 1920, 1080, 320, 240, 3.0f, 1, &x_off, &y_off );

  if( x_off != 480 || y_off != 180 ) {
    fprintf( stderr, "fullscreen offsets: expected 480,180 got %d,%d\n", x_off,
             y_off );
    return 1;
  }

  return 0;
}

static int
scale_rect_applies_offset( void )
{
  sdl2_display_rect src, dst;

  src.x = 10;
  src.y = 20;
  src.w = 30;
  src.h = 40;

  sdl2display_scale_rect( &src, 2.0f, 100, 50, &dst );

  if( dst.x != 120 || dst.y != 90 || dst.w != 60 || dst.h != 80 ) {
    fprintf( stderr, "scaled rect: expected 120,90 60x80 got %d,%d %dx%d\n",
             dst.x, dst.y, dst.w, dst.h );
    return 1;
  }

  return 0;
}

static int
icon_rect_matches_scaled_rect( void )
{
  sdl2_display_rect dst;

  sdl2display_icon_rect( 243, 218, 8, 6, 2.0f, 100, 50, &dst );

  if( dst.x != 586 || dst.y != 486 || dst.w != 16 || dst.h != 12 ) {
    fprintf( stderr, "icon rect: expected 586,486 16x12 got %d,%d %dx%d\n",
             dst.x, dst.y, dst.w, dst.h );
    return 1;
  }

  return 0;
}

static int
update_rect_windowed( void )
{
  sdl2_display_rect dst;

  sdl2display_update_rect( 12, 34, 56, 78, 2.0f, 0, 0, &dst );

  if( dst.x != 24 || dst.y != 68 || dst.w != 112 || dst.h != 156 ) {
    fprintf( stderr,
             "update rect windowed: expected 24,68 112x156 got %d,%d %dx%d\n",
             dst.x, dst.y, dst.w, dst.h );
    return 1;
  }

  return 0;
}

static int
update_rect_fullscreen_offset( void )
{
  sdl2_display_rect dst;

  sdl2display_update_rect( 12, 34, 56, 78, 2.0f, 100, 50, &dst );

  if( dst.x != 124 || dst.y != 118 || dst.w != 112 || dst.h != 156 ) {
    fprintf( stderr,
             "update rect fullscreen: expected 124,118 112x156 got %d,%d %dx%d\n",
             dst.x, dst.y, dst.w, dst.h );
    return 1;
  }

  return 0;
}

static int
surface_view_initialises_fields( void )
{
  sdl2_surface_view view;
  int pixels;

  sdl2display_surface_view_init( &view, &pixels, 320, 240, 640, 2 );

  if( view.pixels != &pixels || view.width != 320 || view.height != 240 ||
      view.pitch != 640 || view.bytes_per_pixel != 2 ) {
    fprintf( stderr, "surface view init: unexpected field values\n" );
    return 1;
  }

  return 0;
}

static int
fullscreen_scaler_keeps_current_when_usable( void )
{
  unsigned char supported[ SCALER_NUM ] = { 0 };
  float scales[ SCALER_NUM ];
  int preserve;
  scaler_type choice;
  int i;

  fill_test_scales( scales );
  for( i = 0; i < SCALER_NUM; i++ ) supported[i] = 1;

  choice = sdl2display_choose_fullscreen_scaler( SCALER_TRIPLESIZE, 3.0f,
                                                 240, 1080,
                                                 supported, scales,
                                                 SCALER_NUM, &preserve );

  if( choice != SCALER_TRIPLESIZE || preserve ) {
    fprintf( stderr,
             "usable current scaler: expected keep 3x without preserve\n" );
    return 1;
  }

  return 0;
}

static int
fullscreen_scaler_picks_larger_supported_scale( void )
{
  unsigned char supported[ SCALER_NUM ] = { 0 };
  float scales[ SCALER_NUM ];
  int preserve;
  scaler_type choice;
  fill_test_scales( scales );

  supported[ SCALER_NORMAL ] = 1;
  supported[ SCALER_DOUBLESIZE ] = 1;
  supported[ SCALER_TRIPLESIZE ] = 1;

  choice = sdl2display_choose_fullscreen_scaler( SCALER_NORMAL, 1.0f,
                                                 240, 1080,
                                                 supported, scales,
                                                 SCALER_NUM, &preserve );

  if( choice != SCALER_TRIPLESIZE || !preserve ) {
    fprintf( stderr, "fullscreen scaler choice: expected 3x with preserve\n" );
    return 1;
  }

  return 0;
}

static int
fullscreen_scaler_falls_back_to_normal( void )
{
  unsigned char supported[ SCALER_NUM ] = { 0 };
  float scales[ SCALER_NUM ];
  int preserve;
  scaler_type choice;
  fill_test_scales( scales );

  supported[ SCALER_NORMAL ] = 1;

  choice = sdl2display_choose_fullscreen_scaler( SCALER_QUADSIZE, 4.0f,
                                                 240, 300,
                                                 supported, scales,
                                                 SCALER_NUM, &preserve );

  if( choice != SCALER_NORMAL || !preserve ) {
    fprintf( stderr, "fullscreen fallback: expected normal with preserve\n" );
    return 1;
  }

  return 0;
}

static int
fullscreen_scaler_keeps_current_when_display_height_zero( void )
{
  unsigned char supported[ SCALER_NUM ] = { 0 };
  float scales[ SCALER_NUM ];
  int preserve;
  scaler_type choice;
  int i;

  fill_test_scales( scales );
  for( i = 0; i < SCALER_NUM; i++ ) supported[i] = 1;

  choice = sdl2display_choose_fullscreen_scaler( SCALER_DOUBLESIZE, 2.0f,
                                                 240, 0,
                                                 supported, scales,
                                                 SCALER_NUM, &preserve );

  if( choice != SCALER_DOUBLESIZE || preserve ) {
    fprintf( stderr,
             "zero display height: expected keep current without preserve\n" );
    return 1;
  }

  return 0;
}

static int
refresh_prefers_50hz_over_60hz( void )
{
  if( sdl2display_compare_refresh( 50, 60 ) >= 0 ) {
    fprintf( stderr, "refresh ordering: expected 50Hz before 60Hz\n" );
    return 1;
  }

  return 0;
}

static int
refresh_prefers_multiple_of_50_over_60hz( void )
{
  if( sdl2display_compare_refresh( 100, 60 ) >= 0 ) {
    fprintf( stderr, "refresh ordering: expected 100Hz before 60Hz\n" );
    return 1;
  }

  return 0;
}

static int
refresh_prefers_multiple_of_60_over_odd_rate( void )
{
  if( sdl2display_compare_refresh( 120, 75 ) >= 0 ) {
    fprintf( stderr, "refresh ordering: expected 120Hz before 75Hz\n" );
    return 1;
  }

  return 0;
}

static int
refresh_treats_zero_or_negative_as_worst( void )
{
  if( sdl2display_compare_refresh( 50, 0 ) >= 0 ) {
    fprintf( stderr, "refresh ordering: expected 50Hz before 0Hz\n" );
    return 1;
  }

  return 0;
}

static int
mode_compare_breaks_tie_by_refresh( void )
{
  sdl2_fullscreen_mode_info left, right;

  left.width = 1920;
  left.height = 1080;
  left.refresh_rate = 50;
  left.fit = 0.53f;

  right.width = 1920;
  right.height = 1080;
  right.refresh_rate = 60;
  right.fit = 0.53f;

  if( sdl2display_compare_mode_info( &left, &right ) >= 0 ) {
    fprintf( stderr,
             "mode compare: expected 50Hz mode before 60Hz mode (same fit)\n" );
    return 1;
  }

  return 0;
}

static int
mode_fit_uses_chosen_scaler( void )
{
  unsigned char supported[ SCALER_NUM ] = { 0 };
  float scales[ SCALER_NUM ];
  float fit_960x600;
  float fit_1512x982;

  fill_test_scales( scales );
  supported[ SCALER_NORMAL ] = 1;
  supported[ SCALER_DOUBLESIZE ] = 1;
  supported[ SCALER_TRIPLESIZE ] = 1;
  supported[ SCALER_QUADSIZE ] = 1;

  fit_960x600 = sdl2display_mode_fit( SCALER_DOUBLESIZE, 320, 240,
                                      960, 600, supported, scales,
                                      SCALER_NUM );
  fit_1512x982 = sdl2display_mode_fit( SCALER_DOUBLESIZE, 320, 240,
                                       1512, 982, supported, scales,
                                       SCALER_NUM );

  if( fit_960x600 <= fit_1512x982 ) {
    fprintf( stderr,
             "mode fit: expected 960x600 to beat 1512x982 for 2x scaler\n" );
    return 1;
  }

  return 0;
}

static int
mode_compare_prefers_better_fit_before_size( void )
{
  sdl2_fullscreen_mode_info left, right;

  left.width = 960;
  left.height = 600;
  left.refresh_rate = 50;
  left.fit = 0.53f;

  right.width = 3024;
  right.height = 1964;
  right.refresh_rate = 50;
  right.fit = 0.21f;

  if( sdl2display_compare_mode_info( &left, &right ) >= 0 ) {
    fprintf( stderr,
             "mode compare: expected better-fit mode before larger mode\n" );
    return 1;
  }

  return 0;
}

typedef int (*test_fn_t)( void );

struct test_t {
  const char *name;
  test_fn_t fn;
};

static const struct test_t tests[] = {
  { "compute_offsets_windowed", compute_offsets_windowed },
  { "compute_offsets_fullscreen", compute_offsets_fullscreen },
  { "scale_rect_applies_offset", scale_rect_applies_offset },
  { "icon_rect_matches_scaled_rect", icon_rect_matches_scaled_rect },
  { "update_rect_windowed", update_rect_windowed },
  { "update_rect_fullscreen_offset", update_rect_fullscreen_offset },
  { "surface_view_initialises_fields", surface_view_initialises_fields },
  { "fullscreen_scaler_keeps_current_when_usable",
    fullscreen_scaler_keeps_current_when_usable },
  { "fullscreen_scaler_picks_larger_supported_scale",
    fullscreen_scaler_picks_larger_supported_scale },
  { "fullscreen_scaler_falls_back_to_normal",
    fullscreen_scaler_falls_back_to_normal },
  { "fullscreen_scaler_keeps_current_when_display_height_zero",
    fullscreen_scaler_keeps_current_when_display_height_zero },
  { "refresh_prefers_50hz_over_60hz", refresh_prefers_50hz_over_60hz },
  { "refresh_prefers_multiple_of_50_over_60hz",
    refresh_prefers_multiple_of_50_over_60hz },
  { "refresh_prefers_multiple_of_60_over_odd_rate",
    refresh_prefers_multiple_of_60_over_odd_rate },
  { "refresh_treats_zero_or_negative_as_worst",
    refresh_treats_zero_or_negative_as_worst },
  { "mode_fit_uses_chosen_scaler", mode_fit_uses_chosen_scaler },
  { "mode_compare_prefers_better_fit_before_size",
    mode_compare_prefers_better_fit_before_size },
  { "mode_compare_breaks_tie_by_refresh", mode_compare_breaks_tie_by_refresh },
  { NULL, NULL }
};

#ifdef main
/* SDL headers redefine main on Windows, but this test needs a normal entry point. */
#undef main
#endif

int
main( void )
{
  const struct test_t *test;

  for( test = tests; test->fn; test++ ) {
    if( test->fn() ) {
      fprintf( stderr, "Test failed: %s\n", test->name );
      return 1;
    }
  }

  return 0;
}
