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

typedef enum phantom_typist_mode_t {
  PHANTOM_TYPIST_MODE_JPP,
  PHANTOM_TYPIST_MODE_ENTER,
  PHANTOM_TYPIST_MODE_LOADPP
} phantom_typist_mode_t;

typedef enum phantom_typist_state_t {
  PHANTOM_TYPIST_STATE_INACTIVE,

  PHANTOM_TYPIST_STATE_WAITING,

  PHANTOM_TYPIST_STATE_LOAD,
  PHANTOM_TYPIST_STATE_QUOTE1,
  PHANTOM_TYPIST_STATE_QUOTE2,
  PHANTOM_TYPIST_STATE_ENTER,

  PHANTOM_TYPIST_STATE_ENTER_ONLY,

  PHANTOM_TYPIST_STATE_L,
  PHANTOM_TYPIST_STATE_O,
  PHANTOM_TYPIST_STATE_A,
  PHANTOM_TYPIST_STATE_D
} phantom_typist_state_t;

static int delay_before_state[] = { 0, 0, 8, 0, 5, 0, 3, 2, 0, 4, 4 };

static phantom_typist_mode_t phantom_typist_mode;
static phantom_typist_state_t phantom_typist_state = PHANTOM_TYPIST_STATE_INACTIVE;
static phantom_typist_state_t next_phantom_typist_state = PHANTOM_TYPIST_STATE_INACTIVE;
static int delay = 0;
static libspectrum_byte keyboard_ports_read = 0x00;

void
phantom_typist_activate( libspectrum_machine machine )
{
  switch( machine ) {
    /* Machines where you press J, SS + P, SS + P, Enter to load a game */
    case LIBSPECTRUM_MACHINE_16:
    case LIBSPECTRUM_MACHINE_48:
    case LIBSPECTRUM_MACHINE_48_NTSC:
    case LIBSPECTRUM_MACHINE_TC2048:
    case LIBSPECTRUM_MACHINE_TC2068:
    case LIBSPECTRUM_MACHINE_TS2068:
      phantom_typist_mode = PHANTOM_TYPIST_MODE_JPP;
      break;

    /* Machines where you just press Enter to load a game */
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

    /* Machines where you type L, O, A, D, SS + P, SS + P, Enter to load a
       game */
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

static libspectrum_byte
type_quote( libspectrum_byte high_byte )
{
  libspectrum_byte r = 0xff;

  switch( high_byte ) {
    case 0x7f:
      r &= ~0x02;
      break;
    case 0xdf:
      r &= ~0x01;
      break;
  }

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
      break;

    case PHANTOM_TYPIST_STATE_LOAD:
      if( high_byte == 0xbf ) {
        r &= ~0x08;
      }
      next_phantom_typist_state = PHANTOM_TYPIST_STATE_QUOTE1;
      break;

    case PHANTOM_TYPIST_STATE_QUOTE1:
      r &= type_quote( high_byte );
      next_phantom_typist_state = PHANTOM_TYPIST_STATE_QUOTE2;
      break;

    case PHANTOM_TYPIST_STATE_QUOTE2:
      r &= type_quote( high_byte );
      next_phantom_typist_state = PHANTOM_TYPIST_STATE_ENTER;
      break;

    case PHANTOM_TYPIST_STATE_ENTER:
    case PHANTOM_TYPIST_STATE_ENTER_ONLY:
      if( high_byte == 0xbf ) {
        r &= ~0x01;
      }
      next_phantom_typist_state = PHANTOM_TYPIST_STATE_INACTIVE;
      break;

    case PHANTOM_TYPIST_STATE_L:
      if( high_byte == 0xbf ) {
        r &= ~0x02;
      }
      next_phantom_typist_state = PHANTOM_TYPIST_STATE_O;
      break;

    case PHANTOM_TYPIST_STATE_O:
      if( high_byte == 0xdf ) {
        r &= ~0x02;
      }
      next_phantom_typist_state = PHANTOM_TYPIST_STATE_A;
      break;

    case PHANTOM_TYPIST_STATE_A:
      if( high_byte == 0xfd ) {
        r &= ~0x01;
      }
      next_phantom_typist_state = PHANTOM_TYPIST_STATE_D;
      break;

    case PHANTOM_TYPIST_STATE_D:
      if( high_byte == 0xfd ) {
        r &= ~0x04;
      }
      next_phantom_typist_state = PHANTOM_TYPIST_STATE_QUOTE1;
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
    delay = delay_before_state[ phantom_typist_state ];
  }
  if( delay > 0 ) {
    delay--;
  }
}
