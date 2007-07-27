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

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <string.h>

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "display.h"
#include "fuse.h"
#include "machines/spec128.h"
#include "memory.h"
#include "module.h"
#include "settings.h"
#include "spectrum.h"
#include "ui/ui.h"
#include "ula.h"

/* Each 8Kb RAM chunk accessible by the Z80 */
memory_page memory_map_read[8];
memory_page memory_map_write[8];

/* Two 8Kb memory chunks accessible by the Z80 when /ROMCS is low */
memory_page memory_map_romcs[2];

/* Mappings for the 'home' (normal ROM/RAM) pages, the Timex DOCK and
   the Timex EXROM */
memory_page *memory_map_home[8];
memory_page *memory_map_dock[8];
memory_page *memory_map_exrom[8];

/* Standard mappings for the 'normal' RAM */
memory_page memory_map_ram[ 2 * SPECTRUM_RAM_PAGES ];

/* Standard mappings for the ROMs */
memory_page memory_map_rom[8];

/* All the memory we've allocated for this machine */
static GSList *pool;

/* Which RAM page contains the current screen */
int memory_current_screen;

/* Which bits to look at when working out where the screen is */
libspectrum_word memory_screen_mask;

static void memory_ram_from_snapshot( libspectrum_snap *snap );
static void memory_ram_to_snapshot( libspectrum_snap *snap );

static module_info_t memory_module_info = {

  NULL,
  NULL,
  memory_ram_from_snapshot,
  memory_ram_to_snapshot,

};

/* Set up the information about the normal page mappings.
   Memory contention and usable pages vary from machine to machine and must
   be set in the appropriate _reset function */
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
    mapping1->source = MEMORY_SOURCE_SYSTEM;

  }

  for( i = 0; i < SPECTRUM_RAM_PAGES; i++ ) {

    mapping1 = &memory_map_ram[ 2 * i     ];
    mapping2 = &memory_map_ram[ 2 * i + 1 ];

    mapping1->page = &RAM[i][ 0x0000 ];
    mapping2->page = &RAM[i][ MEMORY_PAGE_SIZE ];

    mapping1->writable = mapping2->writable = 0;
    mapping1->bank = mapping2->bank = MEMORY_BANK_HOME;
    mapping1->page_num = mapping2->page_num = i;

    mapping1->offset = 0x0000;
    mapping2->offset = MEMORY_PAGE_SIZE;

    mapping1->source = mapping2->source = MEMORY_SOURCE_SYSTEM;
  }

  /* Just initialise these with something */
  for( i = 0; i < 8; i++ )
    memory_map_home[i] = memory_map_dock[i] = memory_map_exrom[i] =
      &memory_map_ram[0];

  for( i = 0; i < 2; i++ ) memory_map_romcs[i].bank = MEMORY_BANK_ROMCS;

  module_register( &memory_module_info );

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
free_memory( gpointer data, gpointer user_data GCC_UNUSED )
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

const char*
memory_bank_name( memory_page *page )
{
  switch( page->bank ) {
  case MEMORY_BANK_NONE: return "Empty";
  case MEMORY_BANK_HOME: return page->writable ? "RAM" : "ROM";
  case MEMORY_BANK_DOCK: return "Dock";
  case MEMORY_BANK_EXROM: return "Exrom";
  case MEMORY_BANK_ROMCS: return "Chip Select";
  }

  return "[Undefined]";
}

libspectrum_byte
readbyte( libspectrum_word address )
{
  libspectrum_word bank;
  memory_page *mapping;

  bank = address >> 13;
  mapping = &memory_map_read[ bank ];

  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_READ, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;

  if( mapping->contended ) tstates += ula_contention[ tstates ];
  tstates += 3;

  return mapping->page[ address & 0x1fff ];
}

void
writebyte( libspectrum_word address, libspectrum_byte b )
{
  libspectrum_word bank;
  memory_page *mapping;

  bank = address >> 13;
  mapping = &memory_map_write[ bank ];

  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_WRITE, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;

  if( mapping->contended ) tstates += ula_contention[ tstates ];

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
  mapping = &memory_map_write[ bank ];
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
  /* Nothing changes if /ROMCS is not set */
  if( !machine_current->ram.romcs ) return;

  /* FIXME: what should we do if more than one of these devices is
     active? What happen in the real situation? e.g. if1+if2 with cartridge?
     
     OK. in the Interface 1 service manual: p.: 1.2 par.: 1.3.1
       All the additional software needed in IC2 (the if1 ROM). IC2 enable
       is discussed in paragraph 1.2.2 above. In addition to control from
       IC1 (the if1 ULA), the ROM maybe disabled by a device connected to
       the (if1's) expansion connector J1. ROMCS2 from (B25), for example,
       Interface 2 connected to J1 would disable both ROM IC2 (if1 ROM) and
       the Spectrum ROM, via isolating diodes D10 and D9 respectively.
     
     All comment in parenthesis added by me (Gergely Szasz).
     The ROMCS2 (B25 conn) in Interface 1 J1 edge connector is in the
     same position than ROMCS (B25 conn) in the Spectrum edge connector.
     
   */

  module_romcs();
}

static void
memory_ram_from_snapshot( libspectrum_snap *snap )
{
  size_t i;
  int capabilities = machine_current->capabilities;

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY )
    spec128_memoryport_write( 0x7ffd,
			      libspectrum_snap_out_128_memoryport( snap ) );

  if( ( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY ) ||
      ( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SCORP_MEMORY )    )
    specplus3_memoryport2_write(
      0x1ffd, libspectrum_snap_out_plus3_memoryport( snap )
    );

  for( i = 0; i < 16; i++ )
    if( libspectrum_snap_pages( snap, i ) )
      memcpy( RAM[i], libspectrum_snap_pages( snap, i ), 0x4000 );
}

static void
memory_ram_to_snapshot( libspectrum_snap *snap )
{
  size_t i;
  libspectrum_byte *buffer;

  libspectrum_snap_set_out_128_memoryport( snap,
					   machine_current->ram.last_byte );
  libspectrum_snap_set_out_plus3_memoryport( snap,
					     machine_current->ram.last_byte2 );

  for( i = 0; i < 16; i++ ) {
    if( RAM[i] != NULL ) {

      buffer = malloc( 0x4000 * sizeof( libspectrum_byte ) );
      if( !buffer ) {
	ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
		  __LINE__ );
	return;
      }

      memcpy( buffer, RAM[i], 0x4000 );
      libspectrum_snap_set_pages( snap, i, buffer );
    }
  }
}
