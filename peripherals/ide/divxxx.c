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
#include "periph.h"

static const libspectrum_byte DIVXXX_CONTROL_CONMEM = 0x80;
static const libspectrum_byte DIVXXX_CONTROL_MAPRAM = 0x40;

typedef struct divxxx_t {
  libspectrum_byte control;

  int active;

  /* automap tracks opcode fetches to entry and exit points to determine
     whether interface memory *would* be paged in at this moment if mapram / wp
     flags allowed it */
  int automap;

  /* Memory pages */
  size_t ram_page_count;

  /* The debugger paging events for this interface */
  int page_event;
  int unpage_event;

  /* References to the current settings for this interface */
  const int *enabled;
  const int *write_protect;

} divxxx_t;

divxxx_t*
divxxx_alloc( size_t ram_page_count, const char *event_type_string,
    const int *enabled, const int *write_protect )
{
  divxxx_t *divxxx = libspectrum_new( divxxx_t, 1 );

  divxxx->control = 0;
  divxxx->active = 0;
  divxxx->automap = 0;
  divxxx->ram_page_count = ram_page_count;

  periph_register_paging_events( event_type_string, &divxxx->page_event,
                                 &divxxx->unpage_event );

  divxxx->enabled = enabled;
  divxxx->write_protect = write_protect;

  return divxxx;
}

void
divxxx_free( divxxx_t *divxxx )
{
  libspectrum_free( divxxx );
}

libspectrum_byte
divxxx_get_control( divxxx_t *divxxx )
{
  return divxxx->control;
}

int
divxxx_get_active( divxxx_t *divxxx )
{
  return divxxx->active;
}

/* DivIDE/DivMMC does not page in immediately on a reset condition (we do that by
   trapping PC instead); however, it needs to perform housekeeping tasks upon
   reset */
void
divxxx_reset( divxxx_t *divxxx, int hard_reset )
{
  divxxx->active = 0;

  if( !*divxxx->enabled ) return;

  if( hard_reset ) {
    divxxx->control = 0;
  } else {
    divxxx->control &= DIVXXX_CONTROL_MAPRAM;
  }
  divxxx->automap = 0;
  divxxx_refresh_page_state( divxxx );
}

void
divxxx_control_write( divxxx_t *divxxx, libspectrum_byte data )
{
  int old_mapram;

  /* MAPRAM bit cannot be reset, only set */
  old_mapram = divxxx->control & DIVXXX_CONTROL_MAPRAM;
  divxxx_control_write_internal( divxxx, data | old_mapram );
}

void
divxxx_control_write_internal( divxxx_t *divxxx, libspectrum_byte data )
{
  divxxx->control = data;
  divxxx_refresh_page_state( divxxx );
}

void
divxxx_set_automap( divxxx_t *divxxx, int automap )
{
  divxxx->automap = automap;
  divxxx_refresh_page_state( divxxx );
}

void
divxxx_refresh_page_state( divxxx_t *divxxx )
{
  if( divxxx->control & DIVXXX_CONTROL_CONMEM ) {
    /* always paged in if conmem enabled */
    divxxx_page( divxxx );
  } else if( *divxxx->write_protect
    || ( divxxx->control & DIVXXX_CONTROL_MAPRAM ) ) {
    /* automap in effect */
    if( divxxx->automap ) {
      divxxx_page( divxxx );
    } else {
      divxxx_unpage( divxxx );
    }
  } else {
    divxxx_unpage( divxxx );
  }
}

void
divxxx_memory_map( divxxx_t *divxxx, memory_page *memory_map_eprom, memory_page memory_map_ram[][ MEMORY_PAGES_IN_8K ] )
{
  int i;
  int upper_ram_page;
  int lower_page_writable, upper_page_writable;
  memory_page *lower_page, *upper_page;

  if( !divxxx->active ) return;

  upper_ram_page = divxxx->control & (divxxx->ram_page_count - 1);
  
  if( divxxx->control & DIVXXX_CONTROL_CONMEM ) {
    lower_page = memory_map_eprom;
    lower_page_writable = !*divxxx->write_protect;
    upper_page = memory_map_ram[ upper_ram_page ];
    upper_page_writable = 1;
  } else {
    if( divxxx->control & DIVXXX_CONTROL_MAPRAM ) {
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
divxxx_page( divxxx_t *divxxx )
{
  divxxx->active = 1;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();

  debugger_event( divxxx->page_event );
}

void
divxxx_unpage( divxxx_t *divxxx )
{
  divxxx->active = 0;
  machine_current->ram.romcs = 0;
  machine_current->memory_map();

  debugger_event( divxxx->unpage_event );
}
