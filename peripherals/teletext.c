/* ttx2000s.c: Routines for handling the TTX200S teletext adapter
   Copyright (c) 2018 Alistair Cree

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

#include <config.h>

#include <libspectrum.h>

#include "compat.h"
#include "infrastructure/startup_manager.h"
#include "machine.h"
#include "memory_pages.h"
#include "module.h"
#include "periph.h"
#include "settings.h"
#include "unittests/unittests.h"
#include "teletext.h"

static memory_page teletext_memory_map_romcs_rom[ MEMORY_PAGES_IN_8K ];
static memory_page teletext_memory_map_romcs_ram[ MEMORY_PAGES_IN_8K ];
static libspectrum_byte teletext_ram[2048];	/* this should really be 1k */

static int teletext_rom_memory_source;
static int teletext_ram_memory_source;


int teletext_paged = 0;

static void teletext_write( libspectrum_word port, libspectrum_byte val );
static void teletext_reset( int hard_reset );
static void teletext_memory_map( void );

static module_info_t teletext_module_info = {

  /* .reset = */ teletext_reset,
  /* .romcs = */ teletext_memory_map,
  /* .snapshot_enabled = */ NULL,
  /* .snapshot_from = */ NULL,
  /* .snapshot_to = */ NULL,
};

static const periph_port_t teletext_ports[] = {
  { 0x0080, 0x0000, NULL, teletext_write },
  { 0, 0, NULL, NULL }
};

static const periph_t teletext_periph = {
  /* .option = */ &settings_current.teletext,
  /* .ports = */ teletext_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ NULL,
};

static int
teletext_init( void *context )
{
  int i;

  module_register( &teletext_module_info );

  teletext_rom_memory_source = memory_source_register( "Teletext ROM" );
  teletext_ram_memory_source = memory_source_register( "Teletext RAM" );
  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ )
    teletext_memory_map_romcs_rom[i].source = teletext_rom_memory_source;
  for( i = 0; i < MEMORY_PAGES_IN_2K; i++ )
    teletext_memory_map_romcs_ram[i].source = teletext_ram_memory_source;

  periph_register( PERIPH_TYPE_TELETEXT, &teletext_periph );
  
  return 0;
}

static void
teletext_end( void )
{
}

void
teletext_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_MEMORY,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_TELETEXT, dependencies,
                            ARRAY_SIZE( dependencies ), teletext_init, NULL,
                            teletext_end );
}

static void
teletext_reset( int hard_reset GCC_UNUSED )
{
  int i;
  
  teletext_paged = 0;
  
  if ( !(settings_current.teletext) ) return;
  
  if( machine_load_rom_bank( teletext_memory_map_romcs_rom, 0,
			     settings_current.rom_teletext,
			     settings_default.rom_teletext, 0x2000 ) ) {
    settings_current.teletext = 0;
    periph_activate_type( PERIPH_TYPE_TELETEXT, 0 );
    return;
  }
  
  periph_activate_type( PERIPH_TYPE_TELETEXT, 1 );
  
  for( i = 0; i < MEMORY_PAGES_IN_2K; i++ ) {
    struct memory_page *page = &teletext_memory_map_romcs_ram[ i ];
    page->page = &teletext_ram[ i * MEMORY_PAGE_SIZE ];
    page->offset = i * MEMORY_PAGE_SIZE;
    page->writable = 1;
  }
  
  teletext_paged = 1;
  machine_current->memory_map();
  machine_current->ram.romcs = 1;
}

static void
teletext_memory_map( void )
{
  if( !teletext_paged ) return;

  memory_map_romcs_8k( 0x0000, teletext_memory_map_romcs_rom );
  memory_map_romcs_2k( 0x2000, teletext_memory_map_romcs_ram );
  memory_map_romcs_2k( 0x2800, teletext_memory_map_romcs_ram );
  memory_map_romcs_2k( 0x3000, teletext_memory_map_romcs_ram );
  memory_map_romcs_2k( 0x3800, teletext_memory_map_romcs_ram );
}

static void
teletext_write( libspectrum_word port GCC_UNUSED, libspectrum_byte val )
{
  /* bits 0 and 1 select channel preset */
  /* bit 2 enables automatic frequency control */
  teletext_paged = (val & 8)?0:1; /* bit 3 pages out */
  machine_current->ram.romcs = teletext_paged;
  machine_current->memory_map();
}

int
teletext_unittest( void )
{
  int r = 0;
  
  teletext_paged = 1;
  teletext_memory_map();
  machine_current->ram.romcs = 1;
  
  r += unittests_assert_8k_page( 0x0000, teletext_rom_memory_source, 0 );
  r += unittests_assert_2k_page( 0x2000, teletext_ram_memory_source, 0 );
  r += unittests_assert_2k_page( 0x2800, teletext_ram_memory_source, 0 );
  r += unittests_assert_2k_page( 0x3000, teletext_ram_memory_source, 0 );
  r += unittests_assert_2k_page( 0x3800, teletext_ram_memory_source, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );
  
  teletext_paged = 0;
  machine_current->memory_map();
  machine_current->ram.romcs = 0;

  r += unittests_paging_test_48( 2 );

  return r;
}
