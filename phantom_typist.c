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

typedef enum phantom_typist_state_t {
  PHANTOM_TYPIST_STATE_INACTIVE,
  PHANTOM_TYPIST_STATE_WAITING,
  PHANTOM_TYPIST_STATE_LOAD,
  PHANTOM_TYPIST_STATE_QUOTE1,
  PHANTOM_TYPIST_STATE_QUOTE2,
  PHANTOM_TYPIST_STATE_ENTER
} phantom_typist_state_t;

static phantom_typist_state_t phantom_typist_state = PHANTOM_TYPIST_STATE_INACTIVE;
static phantom_typist_state_t next_phantom_typist_state = PHANTOM_TYPIST_STATE_WAITING;
static int count = 0;
static libspectrum_dword first_read = 100000;
static libspectrum_byte keyboard_ports_read = 0x00;

libspectrum_byte
phantom_typist_ula_read( libspectrum_word port )
{
  libspectrum_byte r = 0xff;
  libspectrum_byte high_byte = port >> 8;

  switch( phantom_typist_state ) {
    case PHANTOM_TYPIST_STATE_INACTIVE:
      /* Do nothing */
      break;

    case PHANTOM_TYPIST_STATE_WAITING:
      switch( high_byte ) {
        case 0xfe:
          keyboard_ports_read |= 0x01;
          break;
        case 0xfd:
          keyboard_ports_read |= 0x02;
          break;
        case 0xfb:
          keyboard_ports_read |= 0x04;
          break;
        case 0xf7:
          keyboard_ports_read |= 0x08;
          break;
        case 0xef:
          keyboard_ports_read |= 0x10;
          break;
        case 0xdf:
          keyboard_ports_read |= 0x20;
          break;
        case 0xbf:
          keyboard_ports_read |= 0x40;
          break;
        case 0x7f:
          keyboard_ports_read |= 0x80;
          break;
      }

      if( keyboard_ports_read == 0xff ) {
        next_phantom_typist_state = 2;
        count = 8;
      }
      break;

    case PHANTOM_TYPIST_STATE_LOAD:
      if( count == 0 && high_byte == 0xbf ) {
        r &= ~0x08;
        next_phantom_typist_state = 3;
      }
      break;

    case PHANTOM_TYPIST_STATE_QUOTE1:
      switch( high_byte ) {
        case 0x7f:
          r &= ~0x02;
          break;
        case 0xdf:
          r &= ~0x01;
          break;
      }
      next_phantom_typist_state = 4;
      count = 5;
      break;

    case PHANTOM_TYPIST_STATE_QUOTE2:
      if( count == 0 ) {
        switch( high_byte ) {
          case 0x7f:
            r &= ~0x02;
            break;
          case 0xdf:
            r &= ~0x01;
            break;
        }
        next_phantom_typist_state = 5;
      }
      break;

    case PHANTOM_TYPIST_STATE_ENTER:
      if( high_byte == 0xbf ) {
        r &= ~0x01;
        next_phantom_typist_state = 0;
      }
      break;
  }

  return r;
}

void
phantom_typist_frame( void )
{
  first_read = 0;
  keyboard_ports_read = 0x00;
  if( next_phantom_typist_state != phantom_typist_state ) {
    printf("Phantom typist entering state %d\n", next_phantom_typist_state);
    phantom_typist_state = next_phantom_typist_state;
  }
  if( count > 0 ) {
    count--;
  }
}
