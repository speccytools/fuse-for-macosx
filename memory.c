/* memory.c: Routines for accessing memory
   Copyright (c) 1999-2004 Philip Kendall

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
#include "ui/ui.h"

/* Each 8Kb RAM chunk accessible by the Z80 */
memory_page memory_map[8];

/* Two 8Kb memory chunks accessible by the Z80 when /ROMCS is low */
memory_page memory_map_romcs[2];

/* Mappings for the 'home' (normal ROM/RAM) pages, the Timex DOCK and
   the Timex EXROM */
memory_page *memory_map_home[8];
memory_page *memory_map_dock[8];
memory_page *memory_map_exrom[8];

/* Standard mappings for the 'normal' RAM */
memory_page memory_map_ram[32];

/* Standard mappings for the ROMs */
memory_page memory_map_rom[8];

/* All the memory we've allocated for this machine */
static GSList *pool;

/* Which RAM page contains the current screen */
int memory_current_screen;

/* Which bits to look at when working out where the screen is */
libspectrum_word memory_screen_mask;

/* Set up the information about the normal page mappings.
   Memory contention varies from machine to machine and must be set in
   the appropriate _reset function */
int
memory_init( void )
{
  size_t i;
  memory_page *mapping1, *mapping2;

  /* Nothing in the memory pool as yet */
  pool = NULL;

  for( i = 0; i < 8; i++ ) {

    mapping1 = &memory_map_rom[ i ];

    mapping1->page = NULL;
    mapping1->writable = 0;
    mapping1->bank = MEMORY_BANK_HOME;
    mapping1->page_num = i;

  }

  for( i = 0; i < 16; i++ ) {

    mapping1 = &memory_map_ram[ 2 * i     ];
    mapping2 = &memory_map_ram[ 2 * i + 1 ];

    mapping1->page = &RAM[i][ 0x0000 ];
    mapping2->page = &RAM[i][ MEMORY_PAGE_SIZE ];

    mapping1->writable = mapping2->writable = 1;
    mapping1->bank = mapping2->bank = MEMORY_BANK_HOME;
    mapping1->page_num = mapping2->page_num = i;

    mapping1->offset = 0x0000;
    mapping2->offset = MEMORY_PAGE_SIZE;

  }

  /* Just initialise these with something */
  for( i = 0; i < 8; i++ )
    memory_map_home[i] = memory_map_dock[i] = memory_map_exrom[i] =
      &memory_map_ram[0];

  return 0;
}

/* Allocate some memory from the pool */
libspectrum_byte*
memory_pool_allocate( size_t length )
{
  libspectrum_byte *ptr;

  ptr = malloc( length * sizeof( libspectrum_byte ) );
  if( !ptr ) {
    ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
    return NULL;
  }

  pool = g_slist_prepend( pool, ptr );

  return ptr;
}

static void
free_memory( gpointer data, gpointer user_data )
{
  free( data );
}

void
memory_pool_free( void )
{
  g_slist_foreach( pool, free_memory, NULL );
  g_slist_free( pool );
  pool = NULL;
}

libspectrum_byte
readbyte( libspectrum_word address )
{
  libspectrum_word bank;
  memory_page *mapping;

  bank = address >> 13;
  mapping = &memory_map[ bank ];

  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_READ, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;

  if( mapping->contended ) tstates += spectrum_contention[ tstates ];
  tstates += 3;

  return mapping->page[ address & 0x1fff ];
}

void
writebyte( libspectrum_word address, libspectrum_byte b )
{
  libspectrum_word bank;
  memory_page *mapping;

  bank = address >> 13;
  mapping = &memory_map[ bank ];

  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_WRITE, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;

  if( mapping->contended ) tstates += spectrum_contention[ tstates ];
  tstates += 3;

  writebyte_internal( address, b );
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

  if( mapping->writable || settings_current.writable_roms ) {

    /* The offset into the 16Kb RAM page (as opposed to the 8Kb chunk) */
    libspectrum_word offset2 = offset + mapping->offset;

    /* If this is a write to the current screen (and it actually changes
       the destination), redraw that bit */
    if( mapping->bank == MEMORY_BANK_HOME && 
	mapping->page_num == memory_current_screen &&
	( offset2 & memory_screen_mask ) < 0x1b00 &&
	memory[ offset ] != b )
      display_dirty( offset2 );

    memory[ offset ] = b;
  }
}

void
memory_romcs_map( void )
{
  if( machine_current->ram.romcs ) {
    memory_map[0] = memory_map_romcs[0];
    memory_map[1] = memory_map_romcs[1];
  }
}
