/* sdl2_joystick_internal.c: SDL2 joystick helpers
   Copyright (c) 2026 Fredrick Meunier

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 */

#include "config.h"

#include "input.h"

#include "sdl2_joystick_internal.h"

int
sdl2joystick_lookup_id( int which, int joystick1_id, int joystick2_id )
{
  if( which == joystick1_id ) return 0;
  if( which == joystick2_id ) return 1;
  return -1;
}

int
sdl2joystick_button_event( int which, int button, input_event_type type,
                           input_event_t *event )
{
  if( button >= NUM_JOY_BUTTONS ) return 0;

  event->type = type;
  event->types.joystick.which = which;
  event->types.joystick.button = INPUT_JOYSTICK_FIRE_1 + button;

  return 1;
}

void
sdl2joystick_axis_events( int which, int value, input_key negative,
                          input_key positive, input_event_t *event1,
                          input_event_t *event2 )
{
  event1->types.joystick.which = event2->types.joystick.which = which;
  event1->types.joystick.button = negative;
  event2->types.joystick.button = positive;

  if( value > 16384 ) {
    event1->type = INPUT_EVENT_JOYSTICK_RELEASE;
    event2->type = INPUT_EVENT_JOYSTICK_PRESS;
  } else if( value < -16384 ) {
    event1->type = INPUT_EVENT_JOYSTICK_PRESS;
    event2->type = INPUT_EVENT_JOYSTICK_RELEASE;
  } else {
    event1->type = INPUT_EVENT_JOYSTICK_RELEASE;
    event2->type = INPUT_EVENT_JOYSTICK_RELEASE;
  }
}

void
sdl2joystick_hat_event( int which, int value, int mask,
                        input_key direction, input_event_t *event )
{
  event->types.joystick.which = which;
  event->types.joystick.button = direction;
  event->type = ( value & mask ) ?
                INPUT_EVENT_JOYSTICK_PRESS : INPUT_EVENT_JOYSTICK_RELEASE;
}
