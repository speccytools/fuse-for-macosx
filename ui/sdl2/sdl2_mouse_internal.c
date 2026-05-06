/* sdl2_mouse_internal.c: SDL2 mouse helpers
   Copyright (c) 2026 Fredrick Meunier

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 */

#include "config.h"

#include "sdl2_mouse_internal.h"

void
sdl2mouse_get_grab_plan( int startup, int full_screen, int have_window,
                         sdl2_mouse_grab_plan *plan )
{
  plan->should_grab = 0;
  plan->enable_relative_mode = 0;
  plan->enable_window_grab = 0;
  plan->show_cursor = 1;

  if( startup && !full_screen ) return;
  if( !have_window ) return;

  plan->should_grab = 1;
  plan->enable_relative_mode = 1;
  plan->enable_window_grab = 1;
  plan->show_cursor = 0;
}

void
sdl2mouse_get_release_plan( int have_window, sdl2_mouse_grab_plan *plan )
{
  plan->should_grab = 0;
  plan->enable_relative_mode = 0;
  plan->enable_window_grab = have_window ? 0 : -1;
  plan->show_cursor = 1;
}
