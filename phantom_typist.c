/* phantom_typist.c: starting game loading automatically
   Copyright (c) 2017 Philip Kendall

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <libspectrum.h>

#include "keyboard.h"

/* The various strings of keypresses that the phantom typist can use */
typedef enum phantom_typist_mode_t {
  PHANTOM_TYPIST_MODE_JPP, /* J, SS + P, SS + P, Enter */
  PHANTOM_TYPIST_MODE_ENTER, /* Enter only */
  PHANTOM_TYPIST_MODE_LOADPP /* L, O, A, D, SS + P, SS + P, Enter */
} phantom_typist_mode_t;

typedef enum phantom_typist_state_t {
  /* The phantom typist is inactive */
  PHANTOM_TYPIST_STATE_INACTIVE,

  /* The phantom typist is waiting for the keyboard to be read */
  PHANTOM_TYPIST_STATE_WAITING,

  /* JPP mode */
  PHANTOM_TYPIST_STATE_LOAD,
  PHANTOM_TYPIST_STATE_QUOTE1,
  PHANTOM_TYPIST_STATE_QUOTE2,
  PHANTOM_TYPIST_STATE_ENTER,

  /* Enter mode */
  PHANTOM_TYPIST_STATE_ENTER_ONLY,

  /* LOAD mode - chains into the quotes of JPP mode */
  PHANTOM_TYPIST_STATE_L,
  PHANTOM_TYPIST_STATE_O,
  PHANTOM_TYPIST_STATE_A,
  PHANTOM_TYPIST_STATE_D
} phantom_typist_state_t;

/* States for the phantom typist state machine */
struct state_info_t {
  /* The keys to be pressed in this state */
  keyboard_key_name keys_to_press[2];

  /* The next state to move to */
  phantom_typist_state_t next_state;

  /* The number of frames to wait before acting in this state */
  int delay_before_state;
};

/* Definitions for the phantom typist's state machine */
static struct state_info_t state_info[] = {
  /* INACTIVE and WAITING - data not used */
  { { KEYBOARD_NONE, KEYBOARD_NONE }, PHANTOM_TYPIST_STATE_INACTIVE, 0 },
  { { KEYBOARD_NONE, KEYBOARD_NONE }, PHANTOM_TYPIST_STATE_INACTIVE, 0 },

  { { KEYBOARD_j, KEYBOARD_NONE }, PHANTOM_TYPIST_STATE_QUOTE1, 8 },
  { { KEYBOARD_Symbol, KEYBOARD_p }, PHANTOM_TYPIST_STATE_QUOTE2, 0 },
  { { KEYBOARD_Symbol, KEYBOARD_p }, PHANTOM_TYPIST_STATE_ENTER, 5 },
  { { KEYBOARD_Enter, KEYBOARD_NONE }, PHANTOM_TYPIST_STATE_INACTIVE, 0 },
  { { KEYBOARD_Enter, KEYBOARD_NONE }, PHANTOM_TYPIST_STATE_INACTIVE, 3 },
  { { KEYBOARD_l, KEYBOARD_NONE }, PHANTOM_TYPIST_STATE_O, 2 },
  { { KEYBOARD_o, KEYBOARD_NONE }, PHANTOM_TYPIST_STATE_A, 0 },
  { { KEYBOARD_a, KEYBOARD_NONE }, PHANTOM_TYPIST_STATE_D, 4 },
  { { KEYBOARD_d, KEYBOARD_NONE }, PHANTOM_TYPIST_STATE_QUOTE1, 4 }
};

static phantom_typist_mode_t phantom_typist_mode;
static phantom_typist_state_t phantom_typist_state = PHANTOM_TYPIST_STATE_INACTIVE;
static phantom_typist_state_t next_phantom_typist_state = PHANTOM_TYPIST_STATE_INACTIVE;
static int delay = 0;
static libspectrum_byte keyboard_ports_read = 0x00;

void
phantom_typist_activate( libspectrum_machine machine )
{
  switch( machine ) {
    case LIBSPECTRUM_MACHINE_16:
    case LIBSPECTRUM_MACHINE_48:
    case LIBSPECTRUM_MACHINE_48_NTSC:
    case LIBSPECTRUM_MACHINE_TC2048:
    case LIBSPECTRUM_MACHINE_TC2068:
    case LIBSPECTRUM_MACHINE_TS2068:
      phantom_typist_mode = PHANTOM_TYPIST_MODE_JPP;
      break;

    case LIBSPECTRUM_MACHINE_128:
    case LIBSPECTRUM_MACHINE_128E:
    case LIBSPECTRUM_MACHINE_PLUS2:
    case LIBSPECTRUM_MACHINE_PLUS2A:
    case LIBSPECTRUM_MACHINE_PLUS3:
    case LIBSPECTRUM_MACHINE_PLUS3E:
    case LIBSPECTRUM_MACHINE_PENT:
    case LIBSPECTRUM_MACHINE_PENT512:
    case LIBSPECTRUM_MACHINE_PENT1024:
    case LIBSPECTRUM_MACHINE_SCORP:
      phantom_typist_mode = PHANTOM_TYPIST_MODE_ENTER;
      break;

    case LIBSPECTRUM_MACHINE_SE:
      phantom_typist_mode = PHANTOM_TYPIST_MODE_LOADPP;
      break;

    /* Anything else, lets try the JPP set as it ends with Enter anyway */
    default:
      phantom_typist_mode = PHANTOM_TYPIST_MODE_JPP;
      break;
  }

  phantom_typist_state = PHANTOM_TYPIST_STATE_WAITING;
  next_phantom_typist_state = PHANTOM_TYPIST_STATE_WAITING;
}

static void
process_waiting_state( libspectrum_byte high_byte )
{
  switch( high_byte ) {
    case 0xfe:
    case 0xfd:
    case 0xfb:
    case 0xf7:
    case 0xef:
    case 0xdf:
    case 0xbf:
    case 0x7f:
      keyboard_ports_read |= ~high_byte;
      break;
  }

  if( keyboard_ports_read == 0xff ) {
    switch( phantom_typist_mode ) {
      case PHANTOM_TYPIST_MODE_JPP:
        next_phantom_typist_state = PHANTOM_TYPIST_STATE_LOAD;
        break;

      case PHANTOM_TYPIST_MODE_ENTER:
        next_phantom_typist_state = PHANTOM_TYPIST_STATE_ENTER_ONLY;
        break;

      case PHANTOM_TYPIST_MODE_LOADPP:
        next_phantom_typist_state = PHANTOM_TYPIST_STATE_L;
        break;

      default:
        next_phantom_typist_state = PHANTOM_TYPIST_STATE_INACTIVE;
        break;
    }
  }
}

static libspectrum_byte
process_state( libspectrum_byte high_byte )
{
  libspectrum_byte r = 0xff;
  struct state_info_t *this_state = &state_info[phantom_typist_state];

  r &= keyboard_simulate_keypress( high_byte, this_state->keys_to_press[0] );
  r &= keyboard_simulate_keypress( high_byte, this_state->keys_to_press[1] );
  next_phantom_typist_state = this_state->next_state;

  return r;
}

libspectrum_byte
phantom_typist_ula_read( libspectrum_word port )
{
  libspectrum_byte r = 0xff;
  libspectrum_byte high_byte = port >> 8;

  if( delay != 0 ) {
    return r;
  }

  switch( phantom_typist_state ) {
    case PHANTOM_TYPIST_STATE_INACTIVE:
      /* Do nothing */
      break;

    case PHANTOM_TYPIST_STATE_WAITING:
      process_waiting_state( high_byte );
      break;

     default: 
      r &= process_state( high_byte );
      break;
  }

  return r;
}

void
phantom_typist_frame( void )
{
  keyboard_ports_read = 0x00;
  if( next_phantom_typist_state != phantom_typist_state ) {
    phantom_typist_state = next_phantom_typist_state;
    delay = state_info[phantom_typist_state].delay_before_state;
  }
  if( delay > 0 ) {
    delay--;
  }
}
