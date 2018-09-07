/* ttx2000s.c: Routines for handling the TTX2000S teletext adapter
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
#include "ttx2000s.h"

static memory_page ttx2000s_memory_map_romcs_rom[ MEMORY_PAGES_IN_8K ];
static memory_page ttx2000s_memory_map_romcs_ram[ MEMORY_PAGES_IN_8K ];
static libspectrum_byte ttx2000s_ram[2048];	/* this should really be 1k */

static int ttx2000s_rom_memory_source;
static int ttx2000s_ram_memory_source;

int ttx2000s_paged = 0;

static void ttx2000s_write( libspectrum_word port, libspectrum_byte val );
static void ttx2000s_reset( int hard_reset );
static void ttx2000s_memory_map( void );

static module_info_t ttx2000s_module_info = {
  /* .reset = */ ttx2000s_reset,
  /* .romcs = */ ttx2000s_memory_map,
  /* .snapshot_enabled = */ NULL,
  /* .snapshot_from = */ NULL,
  /* .snapshot_to = */ NULL,
};

static const periph_port_t ttx2000s_ports[] = {
  { 0x0080, 0x0000, NULL, ttx2000s_write },
  { 0, 0, NULL, NULL }
};

static const periph_t ttx2000s_periph = {
  /* .option = */ &settings_current.ttx2000s,
  /* .ports = */ ttx2000s_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ NULL,
};

static int
ttx2000s_init( void *context )
{
  int i;

  module_register( &ttx2000s_module_info );

  ttx2000s_rom_memory_source = memory_source_register( "TTX2000S ROM" );
  ttx2000s_ram_memory_source = memory_source_register( "TTX2000S RAM" );
  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ )
    ttx2000s_memory_map_romcs_rom[i].source = ttx2000s_rom_memory_source;
  for( i = 0; i < MEMORY_PAGES_IN_2K; i++ )
    ttx2000s_memory_map_romcs_ram[i].source = ttx2000s_ram_memory_source;

  periph_register( PERIPH_TYPE_TTX2000S, &ttx2000s_periph );

  return 0;
}

static void
ttx2000s_end( void )
{
}

void
ttx2000s_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_MEMORY,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_TTX2000S, dependencies,
                            ARRAY_SIZE( dependencies ), ttx2000s_init, NULL,
                            ttx2000s_end );
}

static void
ttx2000s_reset( int hard_reset GCC_UNUSED )
{
  int i;

  ttx2000s_paged = 0;

  if ( !(settings_current.ttx2000s) ) return;

  if( machine_load_rom_bank( ttx2000s_memory_map_romcs_rom, 0,
			     settings_current.rom_ttx2000s,
			     settings_default.rom_ttx2000s, 0x2000 ) ) {
    settings_current.ttx2000s = 0;
    periph_activate_type( PERIPH_TYPE_TTX2000S, 0 );
    return;
  }

  periph_activate_type( PERIPH_TYPE_TTX2000S, 1 );

  for( i = 0; i < MEMORY_PAGES_IN_2K; i++ ) {
    struct memory_page *page = &ttx2000s_memory_map_romcs_ram[ i ];
    page->page = &ttx2000s_ram[ i * MEMORY_PAGE_SIZE ];
    page->offset = i * MEMORY_PAGE_SIZE;
    page->writable = 1;
  }

  ttx2000s_paged = 1;
  machine_current->memory_map();
  machine_current->ram.romcs = 1;
}

static void
ttx2000s_memory_map( void )
{
  if( !ttx2000s_paged ) return;

  memory_map_romcs_8k( 0x0000, ttx2000s_memory_map_romcs_rom );
  memory_map_romcs_2k( 0x2000, ttx2000s_memory_map_romcs_ram );
  memory_map_romcs_2k( 0x2800, ttx2000s_memory_map_romcs_ram );
  memory_map_romcs_2k( 0x3000, ttx2000s_memory_map_romcs_ram );
  memory_map_romcs_2k( 0x3800, ttx2000s_memory_map_romcs_ram );
}

static void
ttx2000s_write( libspectrum_word port GCC_UNUSED, libspectrum_byte val )
{
  /* bits 0 and 1 select channel preset */
  /* bit 2 enables automatic frequency control */
  ttx2000s_paged = (val & 8)?0:1; /* bit 3 pages out */
  machine_current->ram.romcs = ttx2000s_paged;
  machine_current->memory_map();
}

int
ttx2000s_unittest( void )
{
  int r = 0;

  ttx2000s_paged = 1;
  ttx2000s_memory_map();
  machine_current->ram.romcs = 1;

  r += unittests_assert_8k_page( 0x0000, ttx2000s_rom_memory_source, 0 );
  r += unittests_assert_2k_page( 0x2000, ttx2000s_ram_memory_source, 0 );
  r += unittests_assert_2k_page( 0x2800, ttx2000s_ram_memory_source, 0 );
  r += unittests_assert_2k_page( 0x3000, ttx2000s_ram_memory_source, 0 );
  r += unittests_assert_2k_page( 0x3800, ttx2000s_ram_memory_source, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  ttx2000s_paged = 0;
  machine_current->memory_map();
  machine_current->ram.romcs = 0;

  r += unittests_paging_test_48( 2 );

  return r;
}
