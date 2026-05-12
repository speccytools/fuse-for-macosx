/* sdl2_keyboard.h: SDL 2 keyboard handling
   Copyright (c) 2026 Fredrick Meunier
*/

#ifndef FUSE_SDL2_KEYBOARD_H
#define FUSE_SDL2_KEYBOARD_H

#include <SDL.h>

#include "keyboard.h"

void sdl2keyboard_init( void );
void sdl2keyboard_end( void );
void sdl2keyboard_release_all( void );
void sdl2keyboard_keypress( SDL_KeyboardEvent *keyevent );
void sdl2keyboard_keyrelease( SDL_KeyboardEvent *keyevent );

#endif
