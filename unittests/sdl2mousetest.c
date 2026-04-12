/* sdl2mousetest.c: Tests for SDL2 mouse helpers
   Copyright (c) 2026 Fredrick Meunier

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 */

#include <config.h>

#include <stdio.h>

#include "ui/sdl2/sdl2_mouse_internal.h"

static int
startup_windowed_does_not_grab( void )
{
  sdl2_mouse_grab_plan plan;

  sdl2mouse_get_grab_plan( 1, 0, 1, &plan );

  if( plan.should_grab || plan.enable_relative_mode ||
      plan.enable_window_grab || !plan.show_cursor ) return 1;

  return 0;
}

static int
startup_fullscreen_grabs( void )
{
  sdl2_mouse_grab_plan plan;

  sdl2mouse_get_grab_plan( 1, 1, 1, &plan );

  if( !plan.should_grab || !plan.enable_relative_mode ||
      !plan.enable_window_grab || plan.show_cursor ) return 1;

  return 0;
}

static int
runtime_grab_requires_window( void )
{
  sdl2_mouse_grab_plan plan;

  sdl2mouse_get_grab_plan( 0, 0, 0, &plan );

  if( plan.should_grab || plan.enable_relative_mode ||
      plan.enable_window_grab || !plan.show_cursor ) return 1;

  return 0;
}

static int
runtime_grab_hides_cursor( void )
{
  sdl2_mouse_grab_plan plan;

  sdl2mouse_get_grab_plan( 0, 0, 1, &plan );

  if( !plan.should_grab || !plan.enable_relative_mode ||
      !plan.enable_window_grab || plan.show_cursor ) return 1;

  return 0;
}

static int
release_with_window_releases_grab( void )
{
  sdl2_mouse_grab_plan plan;

  sdl2mouse_get_release_plan( 1, &plan );

  if( plan.should_grab || plan.enable_relative_mode ) return 1;
  if( plan.enable_window_grab != 0 || !plan.show_cursor ) return 1;

  return 0;
}

static int
release_without_window_skips_window_grab_call( void )
{
  sdl2_mouse_grab_plan plan;

  sdl2mouse_get_release_plan( 0, &plan );

  if( plan.enable_window_grab != -1 || !plan.show_cursor ) return 1;

  return 0;
}

typedef int (*test_fn_t)( void );

struct test_t {
  const char *name;
  test_fn_t fn;
};

static const struct test_t tests[] = {
  { "startup_windowed_does_not_grab", startup_windowed_does_not_grab },
  { "startup_fullscreen_grabs", startup_fullscreen_grabs },
  { "runtime_grab_requires_window", runtime_grab_requires_window },
  { "runtime_grab_hides_cursor", runtime_grab_hides_cursor },
  { "release_with_window_releases_grab", release_with_window_releases_grab },
  { "release_without_window_skips_window_grab_call",
    release_without_window_skips_window_grab_call },
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
