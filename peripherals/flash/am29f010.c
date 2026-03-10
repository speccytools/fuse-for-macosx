/* am29f010.c 1Mbit flash chip emulation
   Emulates the AMD 29F010 flash chip

   Copyright (c) 2011-2018 Alistair Cree, Philip Kendall
   
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
 
*/

#include "config.h"

#include <string.h>

#include "am29f010.h"
#include "fuse.h"
#include "ui/ui.h"

#define SIZE_OF_FLASH_ROM 0x20000 /* 128kB */
#define SIZE_OF_FLASH_PAGE 0x4000 /* 16kB */

typedef enum am29f010_flash_state {
  FLASH_STATE_RESET,
  FLASH_STATE_CYCLE2,
  FLASH_STATE_CYCLE3,
  FLASH_STATE_CYCLE4,
  FLASH_STATE_CYCLE5,
  FLASH_STATE_CYCLE6,
  FLASH_STATE_PROGRAM,
  FLASH_STATE_AUTOSELECT,
} am29f010_flash_state;

struct flash_am29f010_t {
  am29f010_flash_state flash_state;
  libspectrum_byte *memory;
};

flash_am29f010_t*
flash_am29f010_alloc( void )
{
  return libspectrum_new( flash_am29f010_t, 1 );
}

void
flash_am29f010_free( flash_am29f010_t *self )
{
  libspectrum_free( self );
}

void
flash_am29f010_init( flash_am29f010_t *self, libspectrum_byte *memory )
{
  self->flash_state = FLASH_STATE_RESET;
  self->memory = memory;
}

static void
flash_am29f010_chip_erase( flash_am29f010_t *self )
{
  memset( self->memory, 0xff, SIZE_OF_FLASH_ROM );
}

static void
flash_am29f010_sector_erase( flash_am29f010_t *self, libspectrum_byte page )
{
  memset( self->memory + ( page * SIZE_OF_FLASH_PAGE ), 0xff, SIZE_OF_FLASH_PAGE );
}

static void
flash_am29f010_program( flash_am29f010_t *self, libspectrum_byte page, libspectrum_word address, libspectrum_byte b )
{
  libspectrum_dword flash_offset = page * SIZE_OF_FLASH_PAGE + address;
  self->memory[ flash_offset ] = b;
}

libspectrum_byte
flash_am29f010_read( flash_am29f010_t *self, libspectrum_byte page, libspectrum_word address )
{
  if( self->flash_state == FLASH_STATE_AUTOSELECT ) {
    switch( address & 0xff ) {
      case 0x00: /* manufacturer ID */
        return 0x01; /* AMD */
      case 0x01: /* device ID */
        return 0x20; /* Am29F010B */
      case 0x02: /* sector protect verify */
        return 0x00; /* always unprotected */
      default:
        return 0xff; /* undefined - don't know what this should return */
    }
  }
  
  libspectrum_dword flash_offset = page * SIZE_OF_FLASH_PAGE + address;
  return self->memory[ flash_offset ];
}

void
flash_am29f010_write( flash_am29f010_t *self, libspectrum_byte page, libspectrum_word address, libspectrum_byte b )
{
  libspectrum_word flash_address = address & 0x7ff;

  /* We implement only the reset, program, chip erase, sector erase
     and autoselect commands for now */
  switch (self->flash_state) {
    case FLASH_STATE_RESET:
      if( flash_address == 0x555 && b == 0xaa )
        self->flash_state = FLASH_STATE_CYCLE2;
      break;
    case FLASH_STATE_CYCLE2:
      if( flash_address == 0x2aa && b == 0x55 )
        self->flash_state = FLASH_STATE_CYCLE3;
      break;
    case FLASH_STATE_CYCLE3:
      if( flash_address == 0x555 ) {
        if( b == 0xa0 )
          self->flash_state = FLASH_STATE_PROGRAM;
        else if( b == 0x80 )
          self->flash_state = FLASH_STATE_CYCLE4;
        else if( b == 0x90 )
          self->flash_state = FLASH_STATE_AUTOSELECT;
      }
      break;
    case FLASH_STATE_CYCLE4:
      if( flash_address == 0x555 && b == 0xaa )
        self->flash_state = FLASH_STATE_CYCLE5;
      break;
    case FLASH_STATE_CYCLE5:
      if( flash_address == 0x2aa && b == 0x55 )
        self->flash_state = FLASH_STATE_CYCLE6;
      break;
    case FLASH_STATE_CYCLE6:
      if( flash_address == 0x555 && b == 0x10 ) {
        flash_am29f010_chip_erase( self );
        self->flash_state = FLASH_STATE_RESET;
      } else if( b == 0x30 ) {
        flash_am29f010_sector_erase( self, page );
        self->flash_state = FLASH_STATE_RESET;
      }
      break;
    case FLASH_STATE_PROGRAM:
      flash_am29f010_program( self, page, address, b );
      self->flash_state = FLASH_STATE_RESET;
      break;
    case FLASH_STATE_AUTOSELECT:
      break;
  }

  if( b == 0xf0 )
    self->flash_state = FLASH_STATE_RESET;
}

