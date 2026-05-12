/* sdl2_keyboard.c: SDL 2 keyboard handling
   Copyright (c) 2026 Fredrick Meunier

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "config.h"

#include <SDL.h>

#include "input.h"
#include "keyboard.h"
#include "ui/ui.h"
#include "sdl2_keyboard.h"

keysyms_map_t keysyms_map[] = {
  { SDLK_TAB,         INPUT_KEY_Tab },
  { SDLK_RETURN,      INPUT_KEY_Return },
  { SDLK_ESCAPE,      INPUT_KEY_Escape },
  { SDLK_SPACE,       INPUT_KEY_space },
  { SDLK_EXCLAIM,     INPUT_KEY_exclam },
  { SDLK_QUOTEDBL,    INPUT_KEY_quotedbl },
  { SDLK_HASH,        INPUT_KEY_numbersign },
  { SDLK_DOLLAR,      INPUT_KEY_dollar },
  { SDLK_PERCENT,     INPUT_KEY_percent },
  { SDLK_AMPERSAND,   INPUT_KEY_ampersand },
  { SDLK_QUOTE,       INPUT_KEY_apostrophe },
  { SDLK_LEFTPAREN,   INPUT_KEY_parenleft },
  { SDLK_RIGHTPAREN,  INPUT_KEY_parenright },
  { SDLK_ASTERISK,    INPUT_KEY_asterisk },
  { SDLK_PLUS,        INPUT_KEY_plus },
  { SDLK_COMMA,       INPUT_KEY_comma },
  { SDLK_MINUS,       INPUT_KEY_minus },
  { SDLK_PERIOD,      INPUT_KEY_period },
  { SDLK_SLASH,       INPUT_KEY_slash },
  { SDLK_0,           INPUT_KEY_0 },
  { SDLK_1,           INPUT_KEY_1 },
  { SDLK_2,           INPUT_KEY_2 },
  { SDLK_3,           INPUT_KEY_3 },
  { SDLK_4,           INPUT_KEY_4 },
  { SDLK_5,           INPUT_KEY_5 },
  { SDLK_6,           INPUT_KEY_6 },
  { SDLK_7,           INPUT_KEY_7 },
  { SDLK_8,           INPUT_KEY_8 },
  { SDLK_9,           INPUT_KEY_9 },
  { SDLK_COLON,       INPUT_KEY_colon },
  { SDLK_SEMICOLON,   INPUT_KEY_semicolon },
  { SDLK_LESS,        INPUT_KEY_less },
  { SDLK_EQUALS,      INPUT_KEY_equal },
  { SDLK_GREATER,     INPUT_KEY_greater },
  { SDLK_QUESTION,    INPUT_KEY_question },
  { SDLK_AT,          INPUT_KEY_at },
  { SDLK_LEFTBRACKET, INPUT_KEY_bracketleft },
  { SDLK_BACKSLASH,   INPUT_KEY_backslash },
  { SDLK_RIGHTBRACKET, INPUT_KEY_bracketright },
  { SDLK_CARET,       INPUT_KEY_asciicircum },
  { SDLK_UNDERSCORE,  INPUT_KEY_underscore },
  { SDLK_a,           INPUT_KEY_a },
  { SDLK_b,           INPUT_KEY_b },
  { SDLK_c,           INPUT_KEY_c },
  { SDLK_d,           INPUT_KEY_d },
  { SDLK_e,           INPUT_KEY_e },
  { SDLK_f,           INPUT_KEY_f },
  { SDLK_g,           INPUT_KEY_g },
  { SDLK_h,           INPUT_KEY_h },
  { SDLK_i,           INPUT_KEY_i },
  { SDLK_j,           INPUT_KEY_j },
  { SDLK_k,           INPUT_KEY_k },
  { SDLK_l,           INPUT_KEY_l },
  { SDLK_m,           INPUT_KEY_m },
  { SDLK_n,           INPUT_KEY_n },
  { SDLK_o,           INPUT_KEY_o },
  { SDLK_p,           INPUT_KEY_p },
  { SDLK_q,           INPUT_KEY_q },
  { SDLK_r,           INPUT_KEY_r },
  { SDLK_s,           INPUT_KEY_s },
  { SDLK_t,           INPUT_KEY_t },
  { SDLK_u,           INPUT_KEY_u },
  { SDLK_v,           INPUT_KEY_v },
  { SDLK_w,           INPUT_KEY_w },
  { SDLK_x,           INPUT_KEY_x },
  { SDLK_y,           INPUT_KEY_y },
  { SDLK_z,           INPUT_KEY_z },
  { SDLK_BACKQUOTE,   INPUT_KEY_asciitilde },
  { SDLK_BACKSPACE,   INPUT_KEY_BackSpace },
  { SDLK_KP_ENTER,    INPUT_KEY_KP_Enter },
  { SDLK_UP,          INPUT_KEY_Up },
  { SDLK_DOWN,        INPUT_KEY_Down },
  { SDLK_LEFT,        INPUT_KEY_Left },
  { SDLK_RIGHT,       INPUT_KEY_Right },
  { SDLK_INSERT,      INPUT_KEY_Insert },
  { SDLK_DELETE,      INPUT_KEY_Delete },
  { SDLK_HOME,        INPUT_KEY_Home },
  { SDLK_END,         INPUT_KEY_End },
  { SDLK_PAGEUP,      INPUT_KEY_Page_Up },
  { SDLK_PAGEDOWN,    INPUT_KEY_Page_Down },
  { SDLK_CAPSLOCK,    INPUT_KEY_Caps_Lock },
  { SDLK_F1,          INPUT_KEY_F1 },
  { SDLK_F2,          INPUT_KEY_F2 },
  { SDLK_F3,          INPUT_KEY_F3 },
  { SDLK_F4,          INPUT_KEY_F4 },
  { SDLK_F5,          INPUT_KEY_F5 },
  { SDLK_F6,          INPUT_KEY_F6 },
  { SDLK_F7,          INPUT_KEY_F7 },
  { SDLK_F8,          INPUT_KEY_F8 },
  { SDLK_F9,          INPUT_KEY_F9 },
  { SDLK_F10,         INPUT_KEY_F10 },
  { SDLK_F11,         INPUT_KEY_F11 },
  { SDLK_F12,         INPUT_KEY_F12 },
  { SDLK_LSHIFT,      INPUT_KEY_Shift_L },
  { SDLK_RSHIFT,      INPUT_KEY_Shift_R },
  { SDLK_LCTRL,       INPUT_KEY_Control_L },
  { SDLK_RCTRL,       INPUT_KEY_Control_R },
  { SDLK_LALT,        INPUT_KEY_Alt_L },
  { SDLK_RALT,        INPUT_KEY_Alt_R },
  { SDLK_LGUI,        INPUT_KEY_Super_L },
  { SDLK_RGUI,        INPUT_KEY_Super_R },
  { SDLK_MODE,        INPUT_KEY_Mode_switch },
  { 0, 0 }
};

static input_key
sdl2keyboard_physical_map( const SDL_KeyboardEvent *keyevent )
{
  switch( keyevent->keysym.scancode ) {
  case SDL_SCANCODE_A: return INPUT_KEY_a;
  case SDL_SCANCODE_B: return INPUT_KEY_b;
  case SDL_SCANCODE_C: return INPUT_KEY_c;
  case SDL_SCANCODE_D: return INPUT_KEY_d;
  case SDL_SCANCODE_E: return INPUT_KEY_e;
  case SDL_SCANCODE_F: return INPUT_KEY_f;
  case SDL_SCANCODE_G: return INPUT_KEY_g;
  case SDL_SCANCODE_H: return INPUT_KEY_h;
  case SDL_SCANCODE_I: return INPUT_KEY_i;
  case SDL_SCANCODE_J: return INPUT_KEY_j;
  case SDL_SCANCODE_K: return INPUT_KEY_k;
  case SDL_SCANCODE_L: return INPUT_KEY_l;
  case SDL_SCANCODE_M: return INPUT_KEY_m;
  case SDL_SCANCODE_N: return INPUT_KEY_n;
  case SDL_SCANCODE_O: return INPUT_KEY_o;
  case SDL_SCANCODE_P: return INPUT_KEY_p;
  case SDL_SCANCODE_Q: return INPUT_KEY_q;
  case SDL_SCANCODE_R: return INPUT_KEY_r;
  case SDL_SCANCODE_S: return INPUT_KEY_s;
  case SDL_SCANCODE_T: return INPUT_KEY_t;
  case SDL_SCANCODE_U: return INPUT_KEY_u;
  case SDL_SCANCODE_V: return INPUT_KEY_v;
  case SDL_SCANCODE_W: return INPUT_KEY_w;
  case SDL_SCANCODE_X: return INPUT_KEY_x;
  case SDL_SCANCODE_Y: return INPUT_KEY_y;
  case SDL_SCANCODE_Z: return INPUT_KEY_z;
  case SDL_SCANCODE_1: return INPUT_KEY_1;
  case SDL_SCANCODE_2: return INPUT_KEY_2;
  case SDL_SCANCODE_3: return INPUT_KEY_3;
  case SDL_SCANCODE_4: return INPUT_KEY_4;
  case SDL_SCANCODE_5: return INPUT_KEY_5;
  case SDL_SCANCODE_6: return INPUT_KEY_6;
  case SDL_SCANCODE_7: return INPUT_KEY_7;
  case SDL_SCANCODE_8: return INPUT_KEY_8;
  case SDL_SCANCODE_9: return INPUT_KEY_9;
  case SDL_SCANCODE_0: return INPUT_KEY_0;
  case SDL_SCANCODE_MINUS: return INPUT_KEY_minus;
  case SDL_SCANCODE_EQUALS: return INPUT_KEY_equal;
  case SDL_SCANCODE_LEFTBRACKET: return INPUT_KEY_bracketleft;
  case SDL_SCANCODE_RIGHTBRACKET: return INPUT_KEY_bracketright;
  case SDL_SCANCODE_BACKSLASH: return INPUT_KEY_backslash;
  case SDL_SCANCODE_SEMICOLON: return INPUT_KEY_semicolon;
  case SDL_SCANCODE_APOSTROPHE: return INPUT_KEY_apostrophe;
  case SDL_SCANCODE_GRAVE: return INPUT_KEY_asciitilde;
  case SDL_SCANCODE_COMMA: return INPUT_KEY_comma;
  case SDL_SCANCODE_PERIOD: return INPUT_KEY_period;
  case SDL_SCANCODE_SLASH: return INPUT_KEY_slash;
  default: return INPUT_KEY_NONE;
  }
}

static input_key
sdl2keyboard_native_map( const SDL_KeyboardEvent *keyevent )
{
  return keysyms_remap( keyevent->keysym.sym );
}

static void
sdl2keyboard_dispatch( input_event_type type, SDL_KeyboardEvent *keyevent )
{
  input_key native_keysym, spectrum_keysym;
  input_event_t fuse_event;

  native_keysym = sdl2keyboard_native_map( keyevent );
  spectrum_keysym = sdl2keyboard_physical_map( keyevent );
  if( spectrum_keysym == INPUT_KEY_NONE ) spectrum_keysym = native_keysym;

  if( native_keysym == INPUT_KEY_NONE &&
      spectrum_keysym == INPUT_KEY_NONE ) return;

  fuse_event.type = type;
  fuse_event.types.key.native_key = native_keysym;
  fuse_event.types.key.spectrum_key = spectrum_keysym;

  input_event( &fuse_event );
}

void
sdl2keyboard_init( void )
{
}

void
sdl2keyboard_end( void )
{
}

void
sdl2keyboard_release_all( void )
{
  keyboard_release_all();
}

void
sdl2keyboard_keypress( SDL_KeyboardEvent *keyevent )
{
  sdl2keyboard_dispatch( INPUT_EVENT_KEYPRESS, keyevent );
}

void
sdl2keyboard_keyrelease( SDL_KeyboardEvent *keyevent )
{
  sdl2keyboard_dispatch( INPUT_EVENT_KEYRELEASE, keyevent );
}
