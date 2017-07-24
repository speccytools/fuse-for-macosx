/* divmmc.c: DivMMC interface routines
   Copyright (c) 2005-2017 Matthew Westcott, Philip Kendall
   Copyright (c) 2015 Stuart Brady

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

#include <string.h>

#include "debugger/debugger.h"
#include "ide.h"
#include "infrastructure/startup_manager.h"
#include "machine.h"
#include "module.h"
#include "periph.h"
#include "settings.h"
#include "ui/ui.h"
#include "unittests/unittests.h"
#include "divmmc.h"

/* Private function prototypes */

static void divmmc_control_write( libspectrum_word port, libspectrum_byte data );
static void divmmc_control_write_internal( libspectrum_byte data );
static void divmmc_card_select( libspectrum_word port, libspectrum_byte data );
static libspectrum_byte divmmc_mmc_read( libspectrum_word port, libspectrum_byte *attached );
static void divmmc_mmc_write( libspectrum_word port, libspectrum_byte data );
static void divmmc_page( void );
static void divmmc_unpage( void );
static void divmmc_activate( void );

/* Data */

static const periph_port_t divmmc_ports[] = {
  { 0x00ff, 0x00e3, NULL, divmmc_control_write },
  { 0x00ff, 0x00e7, NULL, divmmc_card_select },
  { 0x00ff, 0x00eb, divmmc_mmc_read, divmmc_mmc_write },
  { 0, 0, NULL, NULL }
};

static const periph_t divmmc_periph = {
  /* .option = */ &settings_current.divmmc_enabled,
  /* .ports = */ divmmc_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ divmmc_activate,
};

static const libspectrum_byte DIVMMC_CONTROL_CONMEM = 0x80;
static const libspectrum_byte DIVMMC_CONTROL_MAPRAM = 0x40;

int divmmc_automapping_enabled = 0;
int divmmc_active = 0;
static libspectrum_byte divmmc_control;

/* divmmc_automap tracks opcode fetches to entry and exit points to determine
   whether DivMMC memory *would* be paged in at this moment if mapram / wp
   flags allowed it */
static int divmmc_automap = 0;

static libspectrum_ide_channel *divmmc_idechn0;
static libspectrum_ide_channel *divmmc_idechn1;

/* *Our* DivMMC has 128 Kb of RAM - this is all that ESXDOS needs */
#define DIVMMC_PAGES 16
#define DIVMMC_PAGE_LENGTH 0x2000
static libspectrum_byte *divmmc_ram[ DIVMMC_PAGES ];
static libspectrum_byte *divmmc_eprom;
static memory_page divmmc_memory_map_eprom[ MEMORY_PAGES_IN_8K ];
static memory_page divmmc_memory_map_ram[ DIVMMC_PAGES ][ MEMORY_PAGES_IN_8K ];
static int memory_allocated = 0;
static int divmmc_memory_source_eprom;
static int divmmc_memory_source_ram;

static void divmmc_reset( int hard_reset );
static void divmmc_memory_map( void );
static void divmmc_enabled_snapshot( libspectrum_snap *snap );
static void divmmc_from_snapshot( libspectrum_snap *snap );
static void divmmc_to_snapshot( libspectrum_snap *snap );

static module_info_t divmmc_module_info = {

  /* .reset = */ divmmc_reset,
  /* .romcs = */ divmmc_memory_map,
  /* .snapshot_enabled = */ divmmc_enabled_snapshot,
  /* .snapshot_from = */ divmmc_from_snapshot,
  /* .snapshot_to = */ divmmc_to_snapshot,

};

/* Debugger events */
static const char * const event_type_string = "divmmc";
static int page_event, unpage_event;

/* Housekeeping functions */

static int
divmmc_init( void *context )
{
  int error, i, j;

  divmmc_idechn0 = libspectrum_ide_alloc( LIBSPECTRUM_IDE_DATA16 );
  divmmc_idechn1 = libspectrum_ide_alloc( LIBSPECTRUM_IDE_DATA16 );
  
  error = ide_init( divmmc_idechn0,
		    settings_current.divmmc_master_file,
		    UI_MENU_ITEM_MEDIA_IDE_DIVMMC_MASTER_EJECT,
		    settings_current.divmmc_slave_file,
		    UI_MENU_ITEM_MEDIA_IDE_DIVMMC_SLAVE_EJECT );
  if( error ) return error;

  module_register( &divmmc_module_info );

  divmmc_memory_source_eprom = memory_source_register( "DivMMC EPROM" );
  divmmc_memory_source_ram = memory_source_register( "DivMMC RAM" );

  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ ) {
    memory_page *page = &divmmc_memory_map_eprom[i];
    page->source = divmmc_memory_source_eprom;
    page->page_num = 0;
  }

  for( i = 0; i < DIVMMC_PAGES; i++ ) {
    for( j = 0; j < MEMORY_PAGES_IN_8K; j++ ) {
      memory_page *page = &divmmc_memory_map_ram[i][j];
      page->source = divmmc_memory_source_ram;
      page->page_num = i;
    }
  }

  periph_register( PERIPH_TYPE_DIVMMC, &divmmc_periph );
  periph_register_paging_events( event_type_string, &page_event,
                                 &unpage_event );

  return 0;
}

static void
divmmc_end( void )
{
  libspectrum_ide_free( divmmc_idechn0 );
  libspectrum_ide_free( divmmc_idechn1 );
}

void
divmmc_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DEBUGGER,
    STARTUP_MANAGER_MODULE_DISPLAY,
    STARTUP_MANAGER_MODULE_MEMORY,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_DIVMMC, dependencies,
                            ARRAY_SIZE( dependencies ), divmmc_init, NULL,
                            divmmc_end );
}

/* DivMMC does not page in immediately on a reset condition (we do that by
   trapping PC instead); however, it needs to perform housekeeping tasks upon
   reset */
static void
divmmc_reset( int hard_reset )
{
  divmmc_active = 0;

  if( !settings_current.divmmc_enabled ) return;

  if( hard_reset ) {
    divmmc_control = 0;
  } else {
    divmmc_control &= DIVMMC_CONTROL_MAPRAM;
  }
  divmmc_automap = 0;
  divmmc_refresh_page_state();

  libspectrum_ide_reset( divmmc_idechn0 );
  libspectrum_ide_reset( divmmc_idechn1 );
}

int
divmmc_insert( const char *filename, libspectrum_ide_unit unit )
{
  return ide_master_slave_insert(
    divmmc_idechn0, unit, filename,
    &settings_current.divmmc_master_file,
    UI_MENU_ITEM_MEDIA_IDE_DIVMMC_MASTER_EJECT,
    &settings_current.divmmc_slave_file,
    UI_MENU_ITEM_MEDIA_IDE_DIVMMC_SLAVE_EJECT );
}

int
divmmc_commit( libspectrum_ide_unit unit )
{
  int error;

  error = libspectrum_ide_commit( divmmc_idechn0, unit );

  return error;
}

int
divmmc_eject( libspectrum_ide_unit unit )
{
  return ide_master_slave_eject(
    divmmc_idechn0, unit,
    &settings_current.divmmc_master_file,
    UI_MENU_ITEM_MEDIA_IDE_DIVMMC_MASTER_EJECT,
    &settings_current.divmmc_slave_file,
    UI_MENU_ITEM_MEDIA_IDE_DIVMMC_SLAVE_EJECT );
}

/* Port read/writes */

static void
divmmc_control_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  int old_mapram;

  /* MAPRAM bit cannot be reset, only set */
  old_mapram = divmmc_control & DIVMMC_CONTROL_MAPRAM;
  divmmc_control_write_internal( data | old_mapram );
}

static void
divmmc_control_write_internal( libspectrum_byte data )
{
  divmmc_control = data;
  divmmc_refresh_page_state();
}

static void
divmmc_card_select( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  /* TODO */
}

static libspectrum_byte
divmmc_mmc_read( libspectrum_word port GCC_UNUSED, libspectrum_byte *attached )
{
  /* TODO */
  return 0xff;
}

static void
divmmc_mmc_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  /* TODO */
}

void
divmmc_set_automap( int state )
{
  divmmc_automap = state;
  divmmc_refresh_page_state();
}

void
divmmc_refresh_page_state( void )
{
  if( divmmc_control & DIVMMC_CONTROL_CONMEM ) {
    /* always paged in if conmem enabled */
    divmmc_page();
  } else if( settings_current.divmmc_wp
    || ( divmmc_control & DIVMMC_CONTROL_MAPRAM ) ) {
    /* automap in effect */
    if( divmmc_automap ) {
      divmmc_page();
    } else {
      divmmc_unpage();
    }
  } else {
    divmmc_unpage();
  }
}

static void
divmmc_page( void )
{
  divmmc_active = 1;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();

  debugger_event( page_event );
}

static void
divmmc_unpage( void )
{
  divmmc_active = 0;
  machine_current->ram.romcs = 0;
  machine_current->memory_map();

  debugger_event( unpage_event );
}

void
divmmc_memory_map( void )
{
  int i;
  int upper_ram_page;
  int lower_page_writable, upper_page_writable;
  memory_page *lower_page, *upper_page;

  if( !divmmc_active ) return;

  /* low bits of divmmc_control register give page number to use in upper
     bank; only lowest two bits on original 32K model */
  upper_ram_page = divmmc_control & (DIVMMC_PAGES - 1);
  
  if( divmmc_control & DIVMMC_CONTROL_CONMEM ) {
    lower_page = divmmc_memory_map_eprom;
    lower_page_writable = !settings_current.divmmc_wp;
    upper_page = divmmc_memory_map_ram[ upper_ram_page ];
    upper_page_writable = 1;
  } else {
    if( divmmc_control & DIVMMC_CONTROL_MAPRAM ) {
      lower_page = divmmc_memory_map_ram[3];
      lower_page_writable = 0;
      upper_page = divmmc_memory_map_ram[ upper_ram_page ];
      upper_page_writable = ( upper_ram_page != 3 );
    } else {
      lower_page = divmmc_memory_map_eprom;
      lower_page_writable = 0;
      upper_page = divmmc_memory_map_ram[ upper_ram_page ];
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

static void
divmmc_enabled_snapshot( libspectrum_snap *snap )
{
  /* TODO: not going to work yet!

  if( libspectrum_snap_divmmc_active( snap ) )
    settings_current.divmmc_enabled = 1;

  */
}

static void
divmmc_from_snapshot( libspectrum_snap *snap )
{
  /* TODO: not going to work yet!

  size_t i;

  if( !libspectrum_snap_divmmc_active( snap ) ) return;

  settings_current.divmmc_wp =
    libspectrum_snap_divmmc_eprom_writeprotect( snap );
  divmmc_control_write_internal( libspectrum_snap_divmmc_control( snap ) );

  if( libspectrum_snap_divmmc_eprom( snap, 0 ) ) {
    memcpy( divmmc_eprom,
	    libspectrum_snap_divmmc_eprom( snap, 0 ), DIVMMC_PAGE_LENGTH );
  }

  for( i = 0; i < libspectrum_snap_divmmc_pages( snap ); i++ )
    if( libspectrum_snap_divmmc_ram( snap, i ) )
      memcpy( divmmc_ram[ i ], libspectrum_snap_divmmc_ram( snap, i ),
	      DIVMMC_PAGE_LENGTH );

  if( libspectrum_snap_divmmc_paged( snap ) ) {
    divmmc_page();
  } else {
    divmmc_unpage();
  }

  */
}

static void
divmmc_to_snapshot( libspectrum_snap *snap )
{
  /* TODO: not going to work yet!

  size_t i;
  libspectrum_byte *buffer;

  if( !settings_current.divmmc_enabled ) return;

  libspectrum_snap_set_divmmc_active( snap, 1 );
  libspectrum_snap_set_divmmc_eprom_writeprotect( snap,
                                                  settings_current.divmmc_wp );
  libspectrum_snap_set_divmmc_paged( snap, divmmc_active );
  libspectrum_snap_set_divmmc_control( snap, divmmc_control );

  buffer = libspectrum_new( libspectrum_byte, DIVMMC_PAGE_LENGTH );

  memcpy( buffer, divmmc_eprom, DIVMMC_PAGE_LENGTH );
  libspectrum_snap_set_divmmc_eprom( snap, 0, buffer );

  libspectrum_snap_set_divmmc_pages( snap, DIVMMC_PAGES );

  for( i = 0; i < DIVMMC_PAGES; i++ ) {

    buffer = libspectrum_new( libspectrum_byte, DIVMMC_PAGE_LENGTH );

    memcpy( buffer, divmmc_ram[ i ], DIVMMC_PAGE_LENGTH );
    libspectrum_snap_set_divmmc_ram( snap, i, buffer );
  }

  */
}

static void
divmmc_activate( void )
{
  if( !memory_allocated ) {
    int i, j;
    libspectrum_byte *memory =
      memory_pool_allocate_persistent( DIVMMC_PAGES * DIVMMC_PAGE_LENGTH, 1 );

    for( i = 0; i < DIVMMC_PAGES; i++ ) {
      divmmc_ram[i] = memory + i * DIVMMC_PAGE_LENGTH;
      for( j = 0; j < MEMORY_PAGES_IN_8K; j++ ) {
        memory_page *page = &divmmc_memory_map_ram[i][j];
        page->page = divmmc_ram[i] + j * MEMORY_PAGE_SIZE;
        page->offset = j * MEMORY_PAGE_SIZE;
      }
    }

    divmmc_eprom = memory_pool_allocate_persistent( DIVMMC_PAGE_LENGTH, 1 );
    for( i = 0; i < MEMORY_PAGES_IN_8K; i++ ) {
      memory_page *page = &divmmc_memory_map_eprom[i];
      page->page = divmmc_eprom + i * MEMORY_PAGE_SIZE;
      page->offset = i * MEMORY_PAGE_SIZE;
    }

    memory_allocated = 1;
  }
}

int
divmmc_unittest( void )
{
  int r = 0;

  divmmc_set_automap( 1 );

  divmmc_control_write( 0x00e3, 0x80 );
  r += unittests_assert_8k_page( 0x0000, divmmc_memory_source_eprom, 0 );
  r += unittests_assert_8k_page( 0x2000, divmmc_memory_source_ram, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  divmmc_control_write( 0x00e3, 0x83 );
  r += unittests_assert_8k_page( 0x0000, divmmc_memory_source_eprom, 0 );
  r += unittests_assert_8k_page( 0x2000, divmmc_memory_source_ram, 3 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  divmmc_control_write( 0x00e3, 0x40 );
  r += unittests_assert_8k_page( 0x0000, divmmc_memory_source_ram, 3 );
  r += unittests_assert_8k_page( 0x2000, divmmc_memory_source_ram, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  divmmc_control_write( 0x00e3, 0x02 );
  r += unittests_assert_8k_page( 0x0000, divmmc_memory_source_ram, 3 );
  r += unittests_assert_8k_page( 0x2000, divmmc_memory_source_ram, 2 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  /* We have only 128 Kb of RAM (16 x 8 Kb pages), so setting all of bits 0-5
     results in page 15 being selected */
  divmmc_control_write( 0x00e3, 0x3f );
  r += unittests_assert_8k_page( 0x0000, divmmc_memory_source_ram, 3 );
  r += unittests_assert_8k_page( 0x2000, divmmc_memory_source_ram, 15 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  divmmc_set_automap( 0 );

  r += unittests_paging_test_48( 2 );

  return r;
}
