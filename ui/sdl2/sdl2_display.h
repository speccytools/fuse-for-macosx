/* sdl2_display.h: SDL 2 display handling
   Copyright (c) 2026 Fredrick Meunier
*/

#ifndef FUSE_SDL2_DISPLAY_H
#define FUSE_SDL2_DISPLAY_H

#include <SDL.h>

SDL_Window *sdl2display_get_window( void );
void sdl2display_set_title( const char *title );

#endif
