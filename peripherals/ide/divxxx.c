/* divxxx.c: Shared DivIDE/DivMMC interface routines
   Copyright (c) 2005-2017 Matthew Westcott, Stuart Brady, Philip Kendall

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

   E-mail: Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#include <config.h>

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "divxxx.h"
#include "machine.h"

static const libspectrum_byte DIVXXX_CONTROL_CONMEM = 0x80;
static const libspectrum_byte DIVXXX_CONTROL_MAPRAM = 0x40;

/* DivIDE/DivMMC does not page in immediately on a reset condition (we do that by
   trapping PC instead); however, it needs to perform housekeeping tasks upon
   reset */
void
divxxx_reset( int enabled, int write_protect, int hard_reset, int *is_active, libspectrum_byte *control, int *is_automapped, int page_event, int unpage_event )
{
  *is_active = 0;

  if( !enabled ) return;

  if( hard_reset ) {
    *control = 0;
  } else {
    *control &= DIVXXX_CONTROL_MAPRAM;
  }
  *is_automapped = 0;
  divxxx_refresh_page_state( *control, *is_automapped, write_protect, is_active, page_event, unpage_event );
}

void
divxxx_control_write( libspectrum_byte *control, libspectrum_byte data, int is_automapped, int write_protect, int *is_active, int page_event, int unpage_event )
{
  int old_mapram;

  /* MAPRAM bit cannot be reset, only set */
  old_mapram = *control & DIVXXX_CONTROL_MAPRAM;
  divxxx_control_write_internal( control, data | old_mapram, is_automapped, write_protect, is_active, page_event, unpage_event );
}

void
divxxx_control_write_internal( libspectrum_byte *control, libspectrum_byte data, int is_automapped, int write_protect, int *is_active, int page_event, int unpage_event )
{
  *control = data;
  divxxx_refresh_page_state( *control, is_automapped, write_protect, is_active, page_event, unpage_event );
}

void
divxxx_set_automap( int *is_automapped, int automap, libspectrum_byte control, int write_protect, int *is_active, int page_event, int unpage_event )
{
  *is_automapped = automap;
  divxxx_refresh_page_state( control, *is_automapped, write_protect, is_active, page_event, unpage_event );
}

void
divxxx_refresh_page_state( libspectrum_byte control, int is_automapped, int write_protect, int *is_active, int page_event, int unpage_event )
{
  if( control & DIVXXX_CONTROL_CONMEM ) {
    /* always paged in if conmem enabled */
    divxxx_page( is_active, page_event );
  } else if( write_protect
    || ( control & DIVXXX_CONTROL_MAPRAM ) ) {
    /* automap in effect */
    if( is_automapped ) {
      divxxx_page( is_active, page_event );
    } else {
      divxxx_unpage( is_active, unpage_event );
    }
  } else {
    divxxx_unpage( is_active, unpage_event );
  }
}

void
divxxx_memory_map( int is_active, libspectrum_byte control, int write_protect, size_t page_count, memory_page *memory_map_eprom, memory_page memory_map_ram[][ MEMORY_PAGES_IN_8K ] )
{
  int i;
  int upper_ram_page;
  int lower_page_writable, upper_page_writable;
  memory_page *lower_page, *upper_page;

  if( !is_active ) return;

  upper_ram_page = control & (page_count - 1);
  
  if( control & DIVXXX_CONTROL_CONMEM ) {
    lower_page = memory_map_eprom;
    lower_page_writable = !write_protect;
    upper_page = memory_map_ram[ upper_ram_page ];
    upper_page_writable = 1;
  } else {
    if( control & DIVXXX_CONTROL_MAPRAM ) {
      lower_page = memory_map_ram[3];
      lower_page_writable = 0;
      upper_page = memory_map_ram[ upper_ram_page ];
      upper_page_writable = ( upper_ram_page != 3 );
    } else {
      lower_page = memory_map_eprom;
      lower_page_writable = 0;
      upper_page = memory_map_ram[ upper_ram_page ];
      upper_page_writable = 1;
    }
  }

  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ ) {
    lower_page[i].writable = lower_page_writable;
    upper_page[i].writable = upper_page_writable;
  }

  memory_map_romcs_8k( 0x0000, lower_page );
  memory_map_romcs_8k( 0x2000, upper_page );
}

void
divxxx_page( int *is_active, int page_event )
{
  *is_active = 1;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();

  debugger_event( page_event );
}

void
divxxx_unpage( int *is_active, int unpage_event )
{
  *is_active = 0;
  machine_current->ram.romcs = 0;
  machine_current->memory_map();

  debugger_event( unpage_event );
}
