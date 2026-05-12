/* sdl2joysticktest.c: Tests for SDL2 joystick helpers
   Copyright (c) 2026 Fredrick Meunier

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 */

#include <config.h>

#include <stdio.h>

#include "input.h"
#include "ui/sdl2/sdl2_joystick_internal.h"

static int
lookup_maps_instance_ids( void )
{
  if( sdl2joystick_lookup_id( 11, 11, 22 ) != 0 ) return 1;
  if( sdl2joystick_lookup_id( 22, 11, 22 ) != 1 ) return 1;
  if( sdl2joystick_lookup_id( 33, 11, 22 ) != -1 ) return 1;

  return 0;
}

static int
button_event_maps_fire_button( void )
{
  input_event_t event;

  if( !sdl2joystick_button_event( 1, 3, INPUT_EVENT_JOYSTICK_PRESS, &event ) )
    return 1;

  if( event.type != INPUT_EVENT_JOYSTICK_PRESS ) return 1;
  if( event.types.joystick.which != 1 ) return 1;
  if( event.types.joystick.button != INPUT_JOYSTICK_FIRE_4 ) return 1;

  return 0;
}

static int
button_event_rejects_out_of_range_button( void )
{
  input_event_t event;

  return sdl2joystick_button_event( 0, NUM_JOY_BUTTONS,
                                    INPUT_EVENT_JOYSTICK_PRESS,
                                    &event ) ? 1 : 0;
}

static int
axis_positive_presses_positive_direction( void )
{
  input_event_t event1, event2;

  sdl2joystick_axis_events( 0, 20000, INPUT_JOYSTICK_LEFT,
                            INPUT_JOYSTICK_RIGHT, &event1, &event2 );

  if( event1.type != INPUT_EVENT_JOYSTICK_RELEASE ) return 1;
  if( event2.type != INPUT_EVENT_JOYSTICK_PRESS ) return 1;
  if( event1.types.joystick.button != INPUT_JOYSTICK_LEFT ) return 1;
  if( event2.types.joystick.button != INPUT_JOYSTICK_RIGHT ) return 1;

  return 0;
}

static int
axis_negative_presses_negative_direction( void )
{
  input_event_t event1, event2;

  sdl2joystick_axis_events( 0, -20000, INPUT_JOYSTICK_UP,
                            INPUT_JOYSTICK_DOWN, &event1, &event2 );

  if( event1.type != INPUT_EVENT_JOYSTICK_PRESS ) return 1;
  if( event2.type != INPUT_EVENT_JOYSTICK_RELEASE ) return 1;
  if( event1.types.joystick.button != INPUT_JOYSTICK_UP ) return 1;
  if( event2.types.joystick.button != INPUT_JOYSTICK_DOWN ) return 1;

  return 0;
}

static int
axis_neutral_releases_both_directions( void )
{
  input_event_t event1, event2;

  sdl2joystick_axis_events( 0, 0, INPUT_JOYSTICK_LEFT,
                            INPUT_JOYSTICK_RIGHT, &event1, &event2 );

  if( event1.type != INPUT_EVENT_JOYSTICK_RELEASE ) return 1;
  if( event2.type != INPUT_EVENT_JOYSTICK_RELEASE ) return 1;

  return 0;
}

static int
axis_at_positive_threshold_is_neutral( void )
{
  input_event_t event1, event2;

  /* value == 16384: not strictly > 16384, so neutral */
  sdl2joystick_axis_events( 0, 16384, INPUT_JOYSTICK_LEFT,
                            INPUT_JOYSTICK_RIGHT, &event1, &event2 );

  if( event1.type != INPUT_EVENT_JOYSTICK_RELEASE ) return 1;
  if( event2.type != INPUT_EVENT_JOYSTICK_RELEASE ) return 1;

  return 0;
}

static int
axis_at_negative_threshold_is_neutral( void )
{
  input_event_t event1, event2;

  /* value == -16384: not strictly < -16384, so neutral */
  sdl2joystick_axis_events( 0, -16384, INPUT_JOYSTICK_UP,
                            INPUT_JOYSTICK_DOWN, &event1, &event2 );

  if( event1.type != INPUT_EVENT_JOYSTICK_RELEASE ) return 1;
  if( event2.type != INPUT_EVENT_JOYSTICK_RELEASE ) return 1;

  return 0;
}

static int
axis_just_above_threshold_presses_positive( void )
{
  input_event_t event1, event2;

  /* value == 16385: just over threshold, should press positive */
  sdl2joystick_axis_events( 0, 16385, INPUT_JOYSTICK_LEFT,
                            INPUT_JOYSTICK_RIGHT, &event1, &event2 );

  if( event1.type != INPUT_EVENT_JOYSTICK_RELEASE ) return 1;
  if( event2.type != INPUT_EVENT_JOYSTICK_PRESS ) return 1;

  return 0;
}

static int
hat_event_sets_press_and_release( void )
{
  input_event_t event;

  sdl2joystick_hat_event( 1, 0x01, 0x01, INPUT_JOYSTICK_UP, &event );
  if( event.type != INPUT_EVENT_JOYSTICK_PRESS ) return 1;
  if( event.types.joystick.which != 1 ) return 1;
  if( event.types.joystick.button != INPUT_JOYSTICK_UP ) return 1;

  sdl2joystick_hat_event( 1, 0x00, 0x01, INPUT_JOYSTICK_UP, &event );
  if( event.type != INPUT_EVENT_JOYSTICK_RELEASE ) return 1;

  return 0;
}

typedef int (*test_fn_t)( void );

struct test_t {
  const char *name;
  test_fn_t fn;
};

static const struct test_t tests[] = {
  { "lookup_maps_instance_ids", lookup_maps_instance_ids },
  { "button_event_maps_fire_button", button_event_maps_fire_button },
  { "button_event_rejects_out_of_range_button",
    button_event_rejects_out_of_range_button },
  { "axis_positive_presses_positive_direction",
    axis_positive_presses_positive_direction },
  { "axis_negative_presses_negative_direction",
    axis_negative_presses_negative_direction },
  { "axis_neutral_releases_both_directions",
    axis_neutral_releases_both_directions },
  { "axis_at_positive_threshold_is_neutral",
    axis_at_positive_threshold_is_neutral },
  { "axis_at_negative_threshold_is_neutral",
    axis_at_negative_threshold_is_neutral },
  { "axis_just_above_threshold_presses_positive",
    axis_just_above_threshold_presses_positive },
  { "hat_event_sets_press_and_release", hat_event_sets_press_and_release },
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
