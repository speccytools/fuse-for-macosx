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

/* Which RAM page contains the current screen */
int memory_current_screen;

/* Which bits to look at when working out where the screen is */
libspectrum_word memory_screen_mask;

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
  memory_page *mapping;
  libspectrum_byte *memory;

  bank = address >> 13; offset = address & 0x1fff;
  mapping = &memory_map[ bank ];
  memory = mapping->page;

  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_WRITE, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;

  if( mapping->contended ) tstates += spectrum_contention[ tstates ];
  tstates += 3;

  if( mapping->writable || settings_current.writable_roms ) {

    /* The offset into the 16Kb RAM page (as opposed to the 8Kb chunk) */
    libspectrum_word offset2 = offset + mapping->offset;

    /* If this is a write to the current screen (and it actually changes
       the destination), redraw that bit */
    if( mapping->reverse == memory_current_screen &&
	( offset2 & memory_screen_mask ) < 0x1b00 &&
	memory[ offset ] != b )
      display_dirty( offset2 );

    memory[ offset ] = b;
  }
}

void
writebyte_internal( libspectrum_word address, libspectrum_byte b )
{
  libspectrum_word bank, offset;
  memory_page *mapping;
  libspectrum_byte *memory;

  bank = address >> 13; offset = address & 0x1fff;
  mapping = &memory_map[ bank ];
  memory = mapping->page;

  if( memory_map[ bank ].writable || settings_current.writable_roms ) {

    /* The offset into the 16Kb RAM page (as opposed to the 8Kb chunk) */
    libspectrum_word offset2 = offset + mapping->offset;

    /* If this is a write to the current screen (and it actually changes
       the destination), redraw that bit */
    if( mapping->reverse == memory_current_screen &&
	( offset2 & memory_screen_mask ) < 0x1b00 &&
	memory[ offset ] != b )
      display_dirty( offset2 );

    memory[ offset ] = b;
  }
}
