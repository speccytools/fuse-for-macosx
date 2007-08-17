/* pokefinder.c: help with finding pokes
   Copyright (c) 2003-2004 Philip Kendall

   $Id$

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

#include <string.h>

#include <libspectrum.h>

#include "../memory.h"
#include "pokefinder.h"
#include "../spectrum.h"

libspectrum_byte pokefinder_possible[ 2 * SPECTRUM_RAM_PAGES ][0x2000];
libspectrum_byte pokefinder_impossible[ 2 * SPECTRUM_RAM_PAGES ][0x2000/8];
size_t pokefinder_count;

int
pokefinder_clear( void )
{
  size_t page;

  pokefinder_count = 0;
  for( page = 0; page < 2 * SPECTRUM_RAM_PAGES; ++page )
    if( memory_map_ram[page].writable ) {
      pokefinder_count += 8192;
      memcpy( pokefinder_possible[page], memory_map_ram[page].page, 8192 );
      memset( pokefinder_impossible[page], 0, 1024 );
    } else
      memset( pokefinder_impossible[page], 255, 1024 );

  return 0;
}

int
pokefinder_search( libspectrum_byte value )
{
  size_t page, offset;

  for( page = 0; page < 2 * SPECTRUM_RAM_PAGES; page++ )
    for( offset = 0; offset < 0x2000; offset++ ) {

      if( pokefinder_impossible[page][offset/8] & 1 << (offset & 7) ) continue;

      if( RAM[page][offset] != value ) {
	pokefinder_impossible[page][offset/8] |= 1 << (offset & 7);
	pokefinder_count--;
      }
    }

  return 0;
}

int
pokefinder_incremented( void )
{
  size_t page, offset;

  for( page = 0; page < 2 * SPECTRUM_RAM_PAGES; page++ ) {
    for( offset = 0; offset < 0x2000; offset++ ) {

      if( pokefinder_impossible[page][offset/8] & 1 << (offset & 7) ) continue;

      if( RAM[page][offset] > pokefinder_possible[page][offset] ) {
	pokefinder_possible[page][offset] = RAM[page][offset];
      } else {
	pokefinder_impossible[page][offset/8] |= 1 << (offset & 7);
	pokefinder_count--;
      }

    }
  }

  return 0;
}

int
pokefinder_decremented( void )
{
  size_t page, offset;

  for( page = 0; page < 2 * SPECTRUM_RAM_PAGES; page++ ) {
    for( offset = 0; offset < 0x2000; offset++ ) {

      if( pokefinder_impossible[page][offset/8] & 1 << (offset & 7) ) continue;

      if( RAM[page][offset] < pokefinder_possible[page][offset] ) {
	pokefinder_possible[page][offset] = RAM[page][offset];
      } else {
	pokefinder_impossible[page][offset/8] |= 1 << (offset & 7);
	pokefinder_count--;
      }

    }
  }

  return 0;
}
