/* unittests.c: unit testing framework for Fuse
   Copyright (c) 2008-2018 Philip Kendall
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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include "config.h"

#include <string.h>

#include "libspectrum.h"

#include "debugger/debugger.h"
#include "fuse.h"
#include "machine.h"
#include "mempool.h"
#include "periph.h"
#include "peripherals/disk/beta.h"
#include "peripherals/disk/didaktik.h"
#include "peripherals/disk/disciple.h"
#include "peripherals/disk/opus.h"
#include "peripherals/disk/plusd.h"
#include "peripherals/ide/divide.h"
#include "peripherals/ide/divmmc.h"
#include "peripherals/ide/zxatasp.h"
#include "peripherals/ide/zxcf.h"
#include "peripherals/if1.h"
#include "peripherals/if2.h"
#include "peripherals/multiface.h"
#include "peripherals/sound/uspeech.h"
#include "peripherals/speccyboot.h"
#include "peripherals/ttx2000s.h"
#include "peripherals/ula.h"
#include "peripherals/usource.h"
#include "settings.h"
#include "rectangle.h"
#include "unittests.h"
#include "utils.h"

static int
contention_test( void )
{
  libspectrum_dword i, checksum = 0, target;
  int error = 0;

  for( i = 0; i < ULA_CONTENTION_SIZE; i++ ) {
    /* Naive, but it will do for now */
    checksum += ula_contention[ i ] * ( i + 1 );
  }

  if( settings_current.late_timings ) {
    switch( machine_current->machine ) {
    case LIBSPECTRUM_MACHINE_16:
    case LIBSPECTRUM_MACHINE_48:
    case LIBSPECTRUM_MACHINE_SE:
      target = 2308927488UL;
      break;
    case LIBSPECTRUM_MACHINE_48_NTSC:
      target = 1962110976UL;
      break;
    case LIBSPECTRUM_MACHINE_128:
    case LIBSPECTRUM_MACHINE_PLUS2:
      target = 2335248384UL;
      break;
    case LIBSPECTRUM_MACHINE_PLUS2A:
    case LIBSPECTRUM_MACHINE_PLUS3:
    case LIBSPECTRUM_MACHINE_PLUS3E:
      target = 3113840640UL;
      break;
    case LIBSPECTRUM_MACHINE_TC2048:
    case LIBSPECTRUM_MACHINE_TC2068:
      target = 2307959808UL;
      break;
    case LIBSPECTRUM_MACHINE_TS2068:
      target = 1975593984UL;
      break;
    case LIBSPECTRUM_MACHINE_PENT:
    case LIBSPECTRUM_MACHINE_PENT512:
    case LIBSPECTRUM_MACHINE_PENT1024:
    case LIBSPECTRUM_MACHINE_SCORP:
      target = 0;
      break;
    default:
      target = -1;
      break;
    }
  } else {
    switch( machine_current->machine ) {
    case LIBSPECTRUM_MACHINE_16:
    case LIBSPECTRUM_MACHINE_48:
    case LIBSPECTRUM_MACHINE_SE:
      target = 2308862976UL;
      break;
    case LIBSPECTRUM_MACHINE_48_NTSC:
      target = 1962046464UL;
      break;
    case LIBSPECTRUM_MACHINE_128:
    case LIBSPECTRUM_MACHINE_PLUS2:
      target = 2335183872UL;
      break;
    case LIBSPECTRUM_MACHINE_PLUS2A:
    case LIBSPECTRUM_MACHINE_PLUS3:
    case LIBSPECTRUM_MACHINE_PLUS3E:
      target = 3113754624UL;
      break;
    case LIBSPECTRUM_MACHINE_TC2048:
    case LIBSPECTRUM_MACHINE_TC2068:
      target = 2307895296UL;
      break;
    case LIBSPECTRUM_MACHINE_TS2068:
      target = 1975529472UL;
      break;
    case LIBSPECTRUM_MACHINE_PENT:
    case LIBSPECTRUM_MACHINE_PENT512:
    case LIBSPECTRUM_MACHINE_PENT1024:
    case LIBSPECTRUM_MACHINE_SCORP:
      target = 0;
      break;
    default:
      target = -1;
      break;
    }
  }

  if( checksum != target ) {
    printf( "%s: contention test: checksum = %u, expected = %u\n", fuse_progname, checksum, target );
    error = 1;
  }

  return error;
}

static int
floating_bus_test( void )
{
  libspectrum_dword checksum = 0, target;
  libspectrum_word offset;
  int error = 0;

  for( offset = 0; offset < 8192; offset++ )
    RAM[ memory_current_screen ][ offset ] = offset % 0x100;

  for( tstates = 0; tstates < ULA_CONTENTION_SIZE; tstates++ )
    checksum += machine_current->unattached_port() * ( tstates + 1 );

  if( settings_current.late_timings ) {
    switch( machine_current->machine ) {
    case LIBSPECTRUM_MACHINE_16:
    case LIBSPECTRUM_MACHINE_48:
      target = 3426156480UL;
      break;
    case LIBSPECTRUM_MACHINE_48_NTSC:
      target = 3258908608UL;
      break;
    case LIBSPECTRUM_MACHINE_128:
    case LIBSPECTRUM_MACHINE_PLUS2:
      target = 2852995008UL;
      break;
    case LIBSPECTRUM_MACHINE_PLUS2A:
    case LIBSPECTRUM_MACHINE_PLUS3:
    case LIBSPECTRUM_MACHINE_PLUS3E:
    case LIBSPECTRUM_MACHINE_TC2048:
    case LIBSPECTRUM_MACHINE_TC2068:
    case LIBSPECTRUM_MACHINE_TS2068:
    case LIBSPECTRUM_MACHINE_SE:
    case LIBSPECTRUM_MACHINE_PENT:
    case LIBSPECTRUM_MACHINE_PENT512:
    case LIBSPECTRUM_MACHINE_PENT1024:
    case LIBSPECTRUM_MACHINE_SCORP:
      target = 4261381056UL;
      break;
    default:
      target = -1;
      break;
    }
  } else {
    switch( machine_current->machine ) {
    case LIBSPECTRUM_MACHINE_16:
    case LIBSPECTRUM_MACHINE_48:
      target = 3427723200UL;
      break;
    case LIBSPECTRUM_MACHINE_48_NTSC:
      target = 3260475328UL;
      break;
    case LIBSPECTRUM_MACHINE_128:
    case LIBSPECTRUM_MACHINE_PLUS2:
      target = 2854561728UL;
      break;
    case LIBSPECTRUM_MACHINE_PLUS2A:
    case LIBSPECTRUM_MACHINE_PLUS3:
    case LIBSPECTRUM_MACHINE_PLUS3E:
    case LIBSPECTRUM_MACHINE_TC2048:
    case LIBSPECTRUM_MACHINE_TC2068:
    case LIBSPECTRUM_MACHINE_TS2068:
    case LIBSPECTRUM_MACHINE_SE:
    case LIBSPECTRUM_MACHINE_PENT:
    case LIBSPECTRUM_MACHINE_PENT512:
    case LIBSPECTRUM_MACHINE_PENT1024:
    case LIBSPECTRUM_MACHINE_SCORP:
      target = 4261381056UL;
      break;
    default:
      target = -1;
      break;
    }
  }

  if( checksum != target ) {
    printf( "%s: floating bus test: checksum = %u, expected = %u\n", fuse_progname, checksum, target );
    error = 1;
  }

  return error;
}

#define TEST_ASSERT(x) do { if( !(x) ) { printf("Test assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #x ); return 1; } } while( 0 )

static int
floating_bus_merge_test( void )
{
  /* If peripherals asserted all lines, should see no change */
  TEST_ASSERT( periph_merge_floating_bus( 0xaa, 0xff, 0x00 ) == 0xaa ); 
  TEST_ASSERT( periph_merge_floating_bus( 0xaa, 0xff, 0xff ) == 0xaa ); 

  /* If peripherals asserted nothing, should pull all lines down */
  TEST_ASSERT( periph_merge_floating_bus( 0xaa, 0x00, 0x00 ) == 0x00 ); 
  TEST_ASSERT( periph_merge_floating_bus( 0xaa, 0x00, 0xff ) == 0xaa ); 

  /* Tests when peripherals asserted some lines */
  TEST_ASSERT( periph_merge_floating_bus( 0xaa, 0xf0, 0x00 ) == 0xa0 );
  TEST_ASSERT( periph_merge_floating_bus( 0xaa, 0xf0, 0xff ) == 0xaa );
  TEST_ASSERT( periph_merge_floating_bus( 0xaa, 0x0f, 0x00 ) == 0x0a );
  TEST_ASSERT( periph_merge_floating_bus( 0xaa, 0x0f, 0xff ) == 0xaa );

  /* Tests with complementary attached/floating_bus masks */
  TEST_ASSERT( periph_merge_floating_bus( 0xaa, 0xf0, 0x0f ) == 0xaa );
  TEST_ASSERT( periph_merge_floating_bus( 0xaa, 0x0f, 0xf0 ) == 0xaa );
  TEST_ASSERT( periph_merge_floating_bus( 0xaa, 0x55, 0x00 ) == 0x00 );
  TEST_ASSERT( periph_merge_floating_bus( 0xaa, 0x00, 0x55 ) == 0x00 );

  return 0;
}

static int
utils_safe_strdup_test( void )
{
  char *result;

  /* NULL input should return NULL (safe from crash unlike plain strdup) */
  TEST_ASSERT( utils_safe_strdup( NULL ) == NULL );

  /* Regular string should be copied correctly */
  result = utils_safe_strdup( "hello fuse" );
  TEST_ASSERT( result != NULL );
  TEST_ASSERT( strcmp( result, "hello fuse" ) == 0 );
  libspectrum_free( result );

  /* Empty string should produce an allocated, empty string */
  result = utils_safe_strdup( "" );
  TEST_ASSERT( result != NULL );
  TEST_ASSERT( strcmp( result, "" ) == 0 );
  libspectrum_free( result );

  return 0;
}

static int
mempool_test( void )
{
  int pool1, pool2;
  int initial_pools = mempool_get_pools();

  pool1 = mempool_register_pool();

  TEST_ASSERT( mempool_get_pools() == initial_pools + 1 );
  TEST_ASSERT( mempool_get_pool_size( pool1 ) == 0 );

  mempool_malloc( pool1, 23 );

  TEST_ASSERT( mempool_get_pool_size( pool1 ) == 1 );

  mempool_malloc_n( pool1, 42, 4 );

  TEST_ASSERT( mempool_get_pool_size( pool1 ) == 2 );

  mempool_new( pool1, libspectrum_word, 5 );

  TEST_ASSERT( mempool_get_pool_size( pool1 ) == 3 );

  mempool_free( pool1 );

  TEST_ASSERT( mempool_get_pool_size( pool1 ) == 0 );

  pool2 = mempool_register_pool();

  TEST_ASSERT( mempool_get_pools() == initial_pools + 2 );
  TEST_ASSERT( mempool_get_pool_size( pool2 ) == 0 );

  mempool_malloc( pool1, 23 );

  TEST_ASSERT( mempool_get_pool_size( pool2 ) == 0 );

  mempool_malloc_n( pool1, 6, 7 );

  TEST_ASSERT( mempool_get_pool_size( pool2 ) == 0 );

  mempool_new( pool1, libspectrum_byte, 5 );

  TEST_ASSERT( mempool_get_pool_size( pool2 ) == 0 );

  mempool_malloc( pool2, 42 );
  
  TEST_ASSERT( mempool_get_pool_size( pool2 ) == 1 );

  mempool_free( pool2 );

  TEST_ASSERT( mempool_get_pool_size( pool1 ) == 3 );
  TEST_ASSERT( mempool_get_pool_size( pool2 ) == 0 );
  
  mempool_free( pool1 );

  TEST_ASSERT( mempool_get_pool_size( pool1 ) == 0 );
  TEST_ASSERT( mempool_get_pool_size( pool2 ) == 0 );

  /* Test mempool_strdup: verify string content and pool tracking */
  {
    const char *test_string = "hello fuse";
    char *result = mempool_strdup( pool1, test_string );

    TEST_ASSERT( result != NULL );
    TEST_ASSERT( strcmp( result, test_string ) == 0 );
    TEST_ASSERT( mempool_get_pool_size( pool1 ) == 1 );

    mempool_free( pool1 );
    TEST_ASSERT( mempool_get_pool_size( pool1 ) == 0 );
  }

  /* Test mempool_strdup with an empty string */
  {
    char *result = mempool_strdup( pool1, "" );

    TEST_ASSERT( result != NULL );
    TEST_ASSERT( strcmp( result, "" ) == 0 );
    TEST_ASSERT( mempool_get_pool_size( pool1 ) == 1 );

    mempool_free( pool1 );
    TEST_ASSERT( mempool_get_pool_size( pool1 ) == 0 );
  }

  /* Test that out-of-range pool IDs return NULL */
  TEST_ASSERT( mempool_malloc( mempool_get_pools(), 23 ) == NULL );
  TEST_ASSERT( mempool_malloc( -2, 23 ) == NULL );

  return 0;
}

static int
assert_page( libspectrum_word base, libspectrum_word length, int source, int page )
{
  int base_index = base / MEMORY_PAGE_SIZE;
  int i;

  for( i = 0; i < length / MEMORY_PAGE_SIZE; i++ ) {
    TEST_ASSERT( memory_map_read[ base_index + i ].source == source );
    TEST_ASSERT( memory_map_read[ base_index + i ].page_num == page );
    TEST_ASSERT( memory_map_write[ base_index + i ].source == source );
    TEST_ASSERT( memory_map_write[ base_index + i ].page_num == page );
  }

  return 0;
}

int
unittests_assert_2k_page( libspectrum_word base, int source, int page )
{
  return assert_page( base, 0x0800, source, page );
}

int
unittests_assert_4k_page( libspectrum_word base, int source, int page )
{
  return assert_page( base, 0x1000, source, page );
}

int
unittests_assert_8k_page( libspectrum_word base, int source, int page )
{
  return assert_page( base, 0x2000, source, page );
}

int
unittests_assert_16k_page( libspectrum_word base, int source, int page )
{
  return assert_page( base, 0x4000, source, page );
}

static int
assert_16k_rom_page( libspectrum_word base, int page )
{
  return unittests_assert_16k_page( base, memory_source_rom, page );
}

int
unittests_assert_16k_ram_page( libspectrum_word base, int page )
{
  return unittests_assert_16k_page( base, memory_source_ram, page );
}

static int
assert_16k_pages( int rom, int ram4000, int ram8000, int ramc000 )
{
  int r = 0;

  r += assert_16k_rom_page( 0x0000, rom );
  r += unittests_assert_16k_ram_page( 0x4000, ram4000 );
  r += unittests_assert_16k_ram_page( 0x8000, ram8000 );
  r += unittests_assert_16k_ram_page( 0xc000, ramc000 );

  return r;
}

static int
assert_all_ram( int ram0000, int ram4000, int ram8000, int ramc000 )
{
  int r = 0;

  r += unittests_assert_16k_ram_page( 0x0000, ram0000 );
  r += unittests_assert_16k_ram_page( 0x4000, ram4000 );
  r += unittests_assert_16k_ram_page( 0x8000, ram8000 );
  r += unittests_assert_16k_ram_page( 0xc000, ramc000 );

  return r;
}

static int
paging_test_16( void )
{
  int r = 0;

  r += assert_16k_rom_page( 0x0000, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_page( 0x8000, memory_source_none, 0 );
  r += unittests_assert_16k_page( 0xc000, memory_source_none, 0 );

  return r;
}

int
unittests_paging_test_48( int ram8000 )
{
  int r = 0;

  r += assert_16k_pages( 0, 5, ram8000, 0 );
  TEST_ASSERT( memory_current_screen == 5 );

  return r;
}

static int
paging_test_128_unlocked( int ram8000 )
{
  int r = 0;

  TEST_ASSERT( machine_current->ram.locked == 0 );

  r += unittests_paging_test_48( ram8000 );

  writeport_internal( 0x7ffd, 0x07 );
  r += assert_16k_pages( 0, 5, ram8000, 7 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x7ffd, 0x08 );
  r += assert_16k_pages( 0, 5, ram8000, 0 );
  TEST_ASSERT( memory_current_screen == 7 );

  writeport_internal( 0x7ffd, 0x10 );
  r += assert_16k_pages( 1, 5, ram8000, 0 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x7ffd, 0x1f );
  r += assert_16k_pages( 1, 5, ram8000, 7 );
  TEST_ASSERT( memory_current_screen == 7 );

  return r;
}

static int
paging_test_128_locked( int ram8000 )
{
  int r = 0;

  writeport_internal( 0x7ffd, 0x20 );
  r += assert_16k_pages( 0, 5, ram8000, 0 );
  TEST_ASSERT( memory_current_screen == 5 );

  TEST_ASSERT( machine_current->ram.locked != 0 );

  writeport_internal( 0x7ffd, 0x1f );
  r += assert_16k_pages( 0, 5, ram8000, 0 );
  TEST_ASSERT( memory_current_screen == 5 );

  return r;
}

static int
paging_test_128( void )
{
  int r = 0;

  r += paging_test_128_unlocked( 2 );
  r += paging_test_128_locked( 2 );

  return r;
}

static int
paging_test_plus3( void )
{
  int r = 0;
  
  r += paging_test_128_unlocked( 2 );

  writeport_internal( 0x7ffd, 0x00 );
  writeport_internal( 0x1ffd, 0x04 );
  r += assert_16k_pages( 2, 5, 2, 0 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x7ffd, 0x10 );
  r += assert_16k_pages( 3, 5, 2, 0 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x1ffd, 0x01 );
  r += assert_all_ram( 0, 1, 2, 3 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x1ffd, 0x03 );
  r += assert_all_ram( 4, 5, 6, 7 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x1ffd, 0x05 );
  r += assert_all_ram( 4, 5, 6, 3 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x1ffd, 0x07 );
  r += assert_all_ram( 4, 7, 6, 3 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x1ffd, 0x00 );
  r += paging_test_128_locked( 2 );

  writeport_internal( 0x1ffd, 0x10 );
  r += assert_16k_pages( 0, 5, 2, 0 );
  TEST_ASSERT( memory_current_screen == 5 );

  return r;
}

static int
paging_test_scorpion( void )
{
  int r = 0;

  r += paging_test_128_unlocked( 2 );

  writeport_internal( 0x7ffd, 0x00 );
  writeport_internal( 0x1ffd, 0x01 );
  r += assert_all_ram( 0, 5, 2, 0 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x1ffd, 0x02 );
  r += assert_16k_pages( 2, 5, 2, 0 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x1ffd, 0x10 );
  r += assert_16k_pages( 0, 5, 2, 8 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x7ffd, 0x07 );
  r += assert_16k_pages( 0, 5, 2, 15 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x1ffd, 0x00 );
  r += paging_test_128_locked( 2 );

  return r;
}

static int
paging_test_pentagon512_unlocked( void )
{
  int r = 0;

  beta_unpage();

  r += paging_test_128_unlocked( 2 );

  writeport_internal( 0x7ffd, 0x40 );
  r += assert_16k_pages( 0, 5, 2, 8 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x7ffd, 0x47 );
  r += assert_16k_pages( 0, 5, 2, 15 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x7ffd, 0x80 );
  r += assert_16k_pages( 0, 5, 2, 16 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x7ffd, 0xc7 );
  r += assert_16k_pages( 0, 5, 2, 31 );
  TEST_ASSERT( memory_current_screen == 5 );

  return r;
}

static int
paging_test_pentagon512( void )
{
  int r = 0;

  r += paging_test_pentagon512_unlocked();
  r += paging_test_128_locked( 2 );

  return r;
}

static int
paging_test_pentagon1024( void )
{
  int r = 0;

  r += paging_test_pentagon512_unlocked();

  writeport_internal( 0x7ffd, 0x20 );
  r += assert_16k_pages( 0, 5, 2, 32 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x7ffd, 0x27 );
  r += assert_16k_pages( 0, 5, 2, 39 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x7ffd, 0x60 );
  r += assert_16k_pages( 0, 5, 2, 40 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x7ffd, 0xa0 );
  r += assert_16k_pages( 0, 5, 2, 48 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x7ffd, 0xe7 );
  r += assert_16k_pages( 0, 5, 2, 63 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x7ffd, 0x00 );
  writeport_internal( 0xeff7, 0x08 );
  r += assert_all_ram( 0, 5, 2, 0 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x7ffd, 0x00 );
  writeport_internal( 0xeff7, 0x04 );
  r += assert_16k_pages( 0, 5, 2, 0 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x7ffd, 0x40 );
  r += assert_16k_pages( 0, 5, 2, 0 );
  TEST_ASSERT( memory_current_screen == 5 );

  writeport_internal( 0x7ffd, 0x80 );
  r += assert_16k_pages( 0, 5, 2, 0 );
  TEST_ASSERT( memory_current_screen == 5 );

  r += paging_test_128_locked( 2 );

  return r;
}

static int
paging_test_timex( int ram8000, int dock_source, int exrom_source )
{
  int r = 0;

  r += unittests_paging_test_48( ram8000 );

  writeport_internal( 0x00f4, 0x01 );
  r += unittests_assert_8k_page( 0x0000, dock_source, 0 );
  r += unittests_assert_8k_page( 0x2000, memory_source_rom, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, ram8000 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  writeport_internal( 0x00f4, 0x04 );
  r += assert_16k_rom_page( 0x0000, 0 );
  r += unittests_assert_8k_page( 0x4000, dock_source, 2 );
  r += unittests_assert_8k_page( 0x6000, memory_source_ram, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, ram8000 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  writeport_internal( 0x00f4, 0xff );
  r += unittests_assert_8k_page( 0x0000, dock_source, 0 );
  r += unittests_assert_8k_page( 0x2000, dock_source, 1 );
  r += unittests_assert_8k_page( 0x4000, dock_source, 2 );
  r += unittests_assert_8k_page( 0x6000, dock_source, 3 );
  r += unittests_assert_8k_page( 0x8000, dock_source, 4 );
  r += unittests_assert_8k_page( 0xa000, dock_source, 5 );
  r += unittests_assert_8k_page( 0xc000, dock_source, 6 );
  r += unittests_assert_8k_page( 0xe000, dock_source, 7 );

  writeport_internal( 0x00ff, 0x80 );
  r += unittests_assert_8k_page( 0x0000, exrom_source, 0 );
  r += unittests_assert_8k_page( 0x2000, exrom_source, 1 );
  r += unittests_assert_8k_page( 0x4000, exrom_source, 2 );
  r += unittests_assert_8k_page( 0x6000, exrom_source, 3 );
  r += unittests_assert_8k_page( 0x8000, exrom_source, 4 );
  r += unittests_assert_8k_page( 0xa000, exrom_source, 5 );
  r += unittests_assert_8k_page( 0xc000, exrom_source, 6 );
  r += unittests_assert_8k_page( 0xe000, exrom_source, 7 );
  
  writeport_internal( 0x00f4, 0x00 );
  r += assert_16k_rom_page( 0x0000, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, ram8000 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  return r;
}

static int
paging_test_se( void )
{
  int r = 0;

  r += paging_test_128_unlocked( 8 );

  writeport_internal( 0x7ffd, 0x00 );
  r += paging_test_timex( 8, memory_source_dock, memory_source_exrom );

  writeport_internal( 0x7ffd, 0x01 );
  writeport_internal( 0x00f4, 0x0c );
  r += assert_16k_rom_page( 0x0000, 0 );
  r += unittests_assert_8k_page( 0x4000, memory_source_exrom, 2 );
  r += unittests_assert_8k_page( 0x6000, memory_source_exrom, 3 );
  r += unittests_assert_16k_ram_page( 0x8000, 8 );
  r += unittests_assert_8k_page( 0xc000, memory_source_exrom, 6 );
  r += unittests_assert_8k_page( 0xe000, memory_source_exrom, 7 );

  return r;
}

static int
paging_test( void )
{
  int r = 0;

  switch( machine_current->machine ) {
    case LIBSPECTRUM_MACHINE_16:
      r += paging_test_16();
      break;
    case LIBSPECTRUM_MACHINE_48:
    case LIBSPECTRUM_MACHINE_48_NTSC:
      r += unittests_paging_test_48( 2 );
      break;
    case LIBSPECTRUM_MACHINE_128:
    case LIBSPECTRUM_MACHINE_PLUS2:
    case LIBSPECTRUM_MACHINE_PENT:
      r += paging_test_128();
      break;
    case LIBSPECTRUM_MACHINE_PLUS2A:
    case LIBSPECTRUM_MACHINE_PLUS3:
    case LIBSPECTRUM_MACHINE_PLUS3E:
    case LIBSPECTRUM_MACHINE_128E:
      r += paging_test_plus3();
      break;
    case LIBSPECTRUM_MACHINE_SCORP:
      r += paging_test_scorpion();
      break;
    case LIBSPECTRUM_MACHINE_PENT512:
      r += paging_test_pentagon512();
      break;
    case LIBSPECTRUM_MACHINE_PENT1024:
      r += paging_test_pentagon1024();
      break;
    case LIBSPECTRUM_MACHINE_TC2048:
      r += paging_test_timex( 2, memory_source_none, memory_source_none );
      break;
    case LIBSPECTRUM_MACHINE_TC2068:
    case LIBSPECTRUM_MACHINE_TS2068:
      r += paging_test_timex( 2, memory_source_none, memory_source_exrom );
      break;
    case LIBSPECTRUM_MACHINE_SE:
      r += paging_test_se();
      break;
    case LIBSPECTRUM_MACHINE_UNKNOWN:
      printf( "%s:%d: unknown machine?\n", __FILE__, __LINE__ );
      break;
  }

  /* We don't run the peripheral unit tests with the 16K machine or the
     Spectrum SE so as to avoid the problem with them having different RAM
     pages at 0x8000 and/or 0xc000 */
  if( machine_current->machine != LIBSPECTRUM_MACHINE_16 &&
      machine_current->machine != LIBSPECTRUM_MACHINE_SE    )
  {
    r += if1_unittest();
    r += if2_unittest();
    r += multiface_unittest();
    r += speccyboot_unittest();
    r += ttx2000s_unittest();
    r += usource_unittest();
    r += uspeech_unittest();

    r += beta_unittest();
    r += didaktik80_unittest();
    r += disciple_unittest();
    r += opus_unittest();
    r += plusd_unittest();

    r += divide_unittest();
    r += divmmc_unittest();
    r += zxatasp_unittest();
    r += zxcf_unittest();
  }

  return r;
}

static int
rectangle_test( void )
{
  int saved_frame_rate = settings_current.frame_rate;

  /* --- Test 1: rectangle_add creates a new active rectangle --- */
  rectangle_reset();
  rectangle_add( 0, 0, 10 );
  TEST_ASSERT( rectangle_get_active_count() == 1 );
  TEST_ASSERT( rectangle_inactive_count == 0 );

  /* --- Test 2: rectangle_add extends a matching active rectangle --- */
  rectangle_add( 1, 0, 10 );
  TEST_ASSERT( rectangle_get_active_count() == 1 );

  /* --- Test 3: rectangle_add creates a second rect when x,w differ --- */
  rectangle_add( 1, 5, 8 );
  TEST_ASSERT( rectangle_get_active_count() == 2 );

  /* --- Test 4: rectangle_end_line keeps rects updated on this line --- */
  /* Both rects ended at line 1 (y+h = 2 = 1+1), so both should be kept. */
  rectangle_end_line( 1 );
  TEST_ASSERT( rectangle_get_active_count() == 2 );
  TEST_ASSERT( rectangle_inactive_count == 0 );

  /* --- Test 5: rectangle_end_line flushes stale rects to inactive --- */
  /* y=300 is beyond any rect; both move to inactive (frame_rate == 1). */
  settings_current.frame_rate = 1;
  rectangle_end_line( 300 );
  TEST_ASSERT( rectangle_get_active_count() == 0 );
  TEST_ASSERT( rectangle_inactive_count == 2 );

  /* --- Test 6 (frame skip): exact duplicate is discarded --- */
  rectangle_reset();
  settings_current.frame_rate = 2;

  /* Build inactive: {x=0, y=0, w=10, h=3} */
  rectangle_add( 0, 0, 10 );
  rectangle_add( 1, 0, 10 );
  rectangle_add( 2, 0, 10 );
  rectangle_end_line( 300 );
  TEST_ASSERT( rectangle_inactive_count == 1 );

  /* Exact same rect again — should be discarded, count stays 1 */
  rectangle_add( 0, 0, 10 );
  rectangle_add( 1, 0, 10 );
  rectangle_add( 2, 0, 10 );
  rectangle_end_line( 300 );
  TEST_ASSERT( rectangle_inactive_count == 1 );
  TEST_ASSERT( rectangle_inactive[0].h == 3 );

  /* --- Test 7 (frame skip): adjacent rows are merged --- */
  rectangle_reset();
  settings_current.frame_rate = 2;

  /* inactive: {x=0, y=0, w=10, h=3} (rows 0-2) */
  rectangle_add( 0, 0, 10 );
  rectangle_add( 1, 0, 10 );
  rectangle_add( 2, 0, 10 );
  rectangle_end_line( 300 );

  /* source: {x=0, y=3, w=10, h=1} (row 3) — touches row 2, should merge */
  rectangle_add( 3, 0, 10 );
  rectangle_end_line( 300 );
  TEST_ASSERT( rectangle_inactive_count == 1 );
  TEST_ASSERT( rectangle_inactive[0].y == 0 );
  TEST_ASSERT( rectangle_inactive[0].h == 4 );

  /* --- Test 8 (frame skip): same-y different-h merge (bug fix) --- */
  rectangle_reset();
  settings_current.frame_rate = 2;

  /* inactive: {x=0, y=0, w=10, h=3} */
  rectangle_add( 0, 0, 10 );
  rectangle_add( 1, 0, 10 );
  rectangle_add( 2, 0, 10 );
  rectangle_end_line( 300 );
  TEST_ASSERT( rectangle_inactive_count == 1 );

  /* source: {x=0, y=0, w=10, h=5} — same x,w,y but taller; must merge */
  rectangle_add( 0, 0, 10 );
  rectangle_add( 1, 0, 10 );
  rectangle_add( 2, 0, 10 );
  rectangle_add( 3, 0, 10 );
  rectangle_add( 4, 0, 10 );
  rectangle_end_line( 300 );
  TEST_ASSERT( rectangle_inactive_count == 1 );
  TEST_ASSERT( rectangle_inactive[0].y == 0 );
  TEST_ASSERT( rectangle_inactive[0].h == 5 );

  settings_current.frame_rate = saved_frame_rate;
  return 0;
}

int
unittests_run( void )
{
  int r = 0;

  r += contention_test();
  r += floating_bus_test();
  r += floating_bus_merge_test();
  r += utils_safe_strdup_test();
  r += mempool_test();
  r += paging_test();
  r += debugger_disassemble_unittest();
  r += rectangle_test();

  printf("Final return value: %d (should be 0)\n", r);

  return r;
}
