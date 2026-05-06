/* sdl2_joystick_internal.h: SDL2 joystick helpers
   Copyright (c) 2026 Fredrick Meunier

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 */

#ifndef FUSE_SDL2_JOYSTICK_INTERNAL_H
#define FUSE_SDL2_JOYSTICK_INTERNAL_H

#include "config.h"

#include "input.h"
#include "ui/uijoystick.h"

int sdl2joystick_lookup_id( int which, int joystick1_id, int joystick2_id );
int sdl2joystick_button_event( int which, int button, input_event_type type,
                               input_event_t *event );
void sdl2joystick_axis_events( int which, int value, input_key negative,
                               input_key positive, input_event_t *event1,
                               input_event_t *event2 );
void sdl2joystick_hat_event( int which, int value, int mask,
                             input_key direction, input_event_t *event );

#endif
