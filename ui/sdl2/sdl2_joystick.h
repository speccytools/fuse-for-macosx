/* sdl2_joystick.h: SDL 2 joystick support
   Copyright (c) 2026 Fredrick Meunier

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 */

#ifndef FUSE_SDL2_JOYSTICK_H
#define FUSE_SDL2_JOYSTICK_H

#include "config.h"

#include <SDL.h>

#if defined USE_JOYSTICK && !defined HAVE_JSW_H
void sdl2joystick_buttonpress( SDL_JoyButtonEvent *buttonevent );
void sdl2joystick_buttonrelease( SDL_JoyButtonEvent *buttonevent );
void sdl2joystick_axismove( SDL_JoyAxisEvent *axisevent );
void sdl2joystick_hatmove( SDL_JoyHatEvent *hatevent );
#endif

#endif
