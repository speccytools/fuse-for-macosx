/* memory.c: Routines for accessing memory
   Copyright (c) 1999-2003 Philip Kendall

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "display.h"
#include "fuse.h"
#include "memory.h"
#include "settings.h"
#include "spectrum.h"

/* Each 8Kb RAM chunk accessible by the Z80 */
memory_page memory_map[8];

/* The base address of the chunks containing the screen */
libspectrum_byte *memory_screen_chunk1, *memory_screen_chunk2;

/* The highest offset within 'memory_screen_chunk' which is part of the
   screen */
libspectrum_word memory_screen_top;

libspectrum_byte
readbyte( libspectrum_word address )
{
  libspectrum_word bank;

  bank = address >> 13;

  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_READ, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;

  if( memory_map[ bank ].contended ) tstates += spectrum_contention[ tstates ];
  tstates += 3;

  return memory_map[ bank ].page[ address & 0x1fff ];
}

void
writebyte( libspectrum_word address, libspectrum_byte b )
{
  libspectrum_word bank, offset;
  libspectrum_byte *memory;

  bank = address >> 13; offset = address & 0x1fff;
  memory = memory_map[ bank ].page;

  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_WRITE, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;

  if( memory_map[ bank ].contended ) tstates += spectrum_contention[ tstates ];
  tstates += 3;

  if( memory_map[ bank ].writable || settings_current.writable_roms ) {

    if( ( memory == memory_screen_chunk1 || memory == memory_screen_chunk2 ) &&
	offset < memory_screen_top &&
	memory[ offset ] != b )
      display_dirty( address );

    memory[ offset ] = b;
  }
}

void
writebyte_internal( libspectrum_word address, libspectrum_byte b )
{
  libspectrum_word bank, offset;
  libspectrum_byte *memory;

  bank = address >> 13; offset = address & 0x1fff;
  memory = memory_map[ bank ].page;

  if( memory_map[ bank ].writable || settings_current.writable_roms ) {

    if( ( memory == memory_screen_chunk1 || memory == memory_screen_chunk2 ) &&
	offset < memory_screen_top &&
	memory[ offset ] != b )
      display_dirty( address );

    memory[ offset ] = b;
  }
}

#if 0
  
libspectrum_byte
FUNCTION( tc2068_readbyte )( libspectrum_word address )
{
  libspectrum_word offset = address & 0x1fff;
  int chunk = address >> 13;

#ifndef INTERNAL
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_READ, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;

  if( chunk == 2 || chunk == 3 ) tstates += contend_memory( address );
  tstates += 3;
#endif				/* #ifndef INTERNAL */

  return timex_memory[chunk].page[offset];
}

void
FUNCTION( tc2068_writebyte )( libspectrum_word address, libspectrum_byte b )
{
  libspectrum_word offset = address & 0x1fff;
  int chunk = address >> 13;

#ifndef INTERNAL
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_WRITE, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;

  if( chunk == 2 || chunk == 3 ) tstates += contend_memory( address );
  tstates += 3;
#endif				/* #ifndef INTERNAL */

/* Need to take check if write is to home bank screen area for display
   dirty checking */
  switch( chunk ) {
  case 2:
  case 3:
    if( timex_memory[chunk].page == timex_home[chunk].page &&
        timex_memory[chunk].page[offset] != b ) display_dirty( address );
    /* Fall through */
  default:
    if( timex_memory[chunk].writeable == 1 ) timex_memory[chunk].page[offset] = b;
  }
}

#endif				/* #if 0 */
