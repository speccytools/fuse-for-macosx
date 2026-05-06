/* sdl2_mouse_internal.h: SDL2 mouse helpers
   Copyright (c) 2026 Fredrick Meunier

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 */

#ifndef FUSE_SDL2_MOUSE_INTERNAL_H
#define FUSE_SDL2_MOUSE_INTERNAL_H

#include "config.h"

typedef struct sdl2_mouse_grab_plan {
  int should_grab;
  int enable_relative_mode;
  int enable_window_grab;
  int show_cursor;
} sdl2_mouse_grab_plan;

void sdl2mouse_get_grab_plan( int startup, int full_screen, int have_window,
                              sdl2_mouse_grab_plan *plan );
void sdl2mouse_get_release_plan( int have_window,
                                 sdl2_mouse_grab_plan *plan );

#endif
