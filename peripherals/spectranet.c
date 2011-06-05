/* spectranet.c: Spectranet emulation
   Copyright (c) 2011 Philip Kendall
   
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

#include "config.h"

#include "compat.h"
#include "debugger/debugger.h"
#include "machine.h"
#include "memory.h"
#include "module.h"
#include "periph.h"
#include "settings.h"

#define SPECTRANET_PAGES 256
#define SPECTRANET_PAGE_LENGTH 0x1000

#define SPECTRANET_ROM_LENGTH 0x4000  /* TODO: should really be 128K */
#define SPECTRANET_ROM_BASE 0

#define SPECTRANET_BUFFER_LENGTH 0x8000
#define SPECTRANET_BUFFER_BASE 0x40

#define SPECTRANET_RAM_LENGTH 0x20000
#define SPECTRANET_RAM_BASE 0xc0

static memory_page spectranet_full_map[SPECTRANET_PAGES * MEMORY_PAGES_IN_4K];
static memory_page spectranet_current_map[MEMORY_PAGES_IN_16K];
static int spectranet_memory_allocated = 0;

int spectranet_available = 0;
static int spectranet_paged;

static int spectranet_source;

/* Debugger events */
static const char *event_type_string = "spectranet";
static int page_event, unpage_event;

void
spectranet_page( void )
{
  if( spectranet_paged )
    return;

  spectranet_paged = 1;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();
  debugger_event( page_event );
}

void
spectranet_unpage( void )
{
  if( !spectranet_paged )
    return;

  spectranet_paged = 0;
  machine_current->ram.romcs = 0;
  machine_current->memory_map();
  debugger_event( unpage_event );
}

static void
spectranet_map_page( int dest, int source )
{
  int i;

  for( i = 0; i < MEMORY_PAGES_IN_4K; i++ )
    spectranet_current_map[dest * MEMORY_PAGES_IN_4K + i] =
      spectranet_full_map[source * MEMORY_PAGES_IN_4K + i];
}

static void
spectranet_reset( int hard_reset GCC_UNUSED )
{
  int i, j;

  if( !periph_is_active( PERIPH_TYPE_SPECTRANET ) )
    return;

  spectranet_available = 0;
  spectranet_paged = 1;

  if( machine_load_rom_bank( spectranet_full_map, 0,
			     settings_current.rom_spectranet,
			     settings_default.rom_spectranet,
			     SPECTRANET_ROM_LENGTH ) ) {
    settings_current.spectranet = 0;
    periph_activate_type( PERIPH_TYPE_SPECTRANET, 0 );
    return;
  }

  /* machine_load_rom_bank() assumes 16K pages, so we have to fix things up */
  for( i = 0; i < SPECTRANET_ROM_LENGTH / SPECTRANET_PAGE_LENGTH; i++ )
    for( j = 0; j < MEMORY_PAGES_IN_4K; j++ )
      spectranet_full_map[i * MEMORY_PAGES_IN_4K + j].page_num = i;

  spectranet_map_page( 0, 0x00 ); /* 0x0000 to 0x0fff is always chip 0, page 0 */
  spectranet_map_page( 1, 0xff ); /* And map something into 0x1000 to 0x1fff */
  spectranet_map_page( 2, 0xff ); /* And 0x2000 to 0x2fff */
  spectranet_map_page( 3, 0xc0 ); /* 0x3000 to 0x3fff is always chip 3, page 0 */

  machine_current->ram.romcs = 1;
  machine_current->memory_map();

  spectranet_available = 1;
}

static void
spectranet_memory_map( void )
{
  if( !spectranet_paged ) return;

  memory_map_romcs( spectranet_current_map );
}

static void
spectranet_activate( void )
{
  if( !spectranet_memory_allocated ) {

    int i, j;

    libspectrum_byte *fake_bank =
      memory_pool_allocate_persistent( 0x1000, 1 );
    memset( fake_bank, 0xff, 0x1000 );

    /* Start of by mapping the fake data in everywhere */
    for( i = 0; i < SPECTRANET_PAGES; i++ )
      for( j = 0; j < MEMORY_PAGES_IN_4K; j++ ) {
        memory_page *page = &spectranet_full_map[i * MEMORY_PAGES_IN_4K + j];
        page->writable = 0;
        page->contended = 0;
        page->source = spectranet_source;
        page->save_to_snapshot = 0;
        page->page_num = i;
        page->offset = j * MEMORY_PAGE_SIZE;
        page->page = fake_bank + page->offset;
      }

    /* Pages 0x00 to 0x1f is the ROM, which is loaded on reset */

    libspectrum_byte *w5100_buffer =
      memory_pool_allocate_persistent( SPECTRANET_BUFFER_LENGTH, 1 );

    for( i = 0; i < SPECTRANET_BUFFER_LENGTH / SPECTRANET_PAGE_LENGTH; i++ ) {
      int base = (SPECTRANET_BUFFER_BASE + i) * MEMORY_PAGES_IN_4K;
      for( j = 0; j < MEMORY_PAGES_IN_4K; j++ ) {
        memory_page *page = &spectranet_full_map[base + j];
        page->writable = 1;
        page->page = w5100_buffer + (i * MEMORY_PAGES_IN_4K + j) * MEMORY_PAGE_SIZE;
      }
    }

    libspectrum_byte *ram =
      memory_pool_allocate_persistent( SPECTRANET_RAM_LENGTH, 1 );

    for( i = 0; i < SPECTRANET_RAM_LENGTH / SPECTRANET_PAGE_LENGTH; i++ ) {
      int base = (SPECTRANET_RAM_BASE + i) * MEMORY_PAGES_IN_4K;
      for( j = 0; j < MEMORY_PAGES_IN_4K; j++ ) {
        memory_page *page = &spectranet_full_map[base + j];
        page->writable = 1;
        page->page = ram + (i * MEMORY_PAGES_IN_4K + j) * MEMORY_PAGE_SIZE;
      }
    }

    spectranet_memory_allocated = 1;
  }
}

static module_info_t spectranet_module_info = {
  spectranet_reset,
  spectranet_memory_map,
  NULL,
  NULL,
  NULL
};

static void
spectranet_page_a( libspectrum_word port, libspectrum_byte data )
{
  spectranet_map_page( 1, data );
  memory_map_romcs( spectranet_current_map );
}

static void
spectranet_page_b( libspectrum_word port, libspectrum_byte data )
{
  spectranet_map_page( 2, data );
  memory_map_romcs( spectranet_current_map );
}

static void
spectranet_trap( libspectrum_word port, libspectrum_byte data )
{
}

static libspectrum_byte
spectranet_control_read( libspectrum_word port, int *attached )
{
  return 0xff;
}

static void
spectranet_control_write( libspectrum_word port, libspectrum_byte data )
{
}

static const periph_port_t spectranet_ports[] = {
  { 0xffff, 0x003b, NULL, spectranet_page_a },
  { 0xffff, 0x013b, NULL, spectranet_page_b },
  { 0xffff, 0x023b, NULL, spectranet_trap },
  { 0xffff, 0x033b, spectranet_control_read, spectranet_control_write },
  { 0, 0, NULL, NULL }
};

static const periph_t spectranet_periph = {
  &settings_current.spectranet,
  spectranet_ports,
  spectranet_activate
};

int
spectranet_init( void )
{
  module_register( &spectranet_module_info );
  spectranet_source = memory_source_register( "Spectranet" );
  periph_register( PERIPH_TYPE_SPECTRANET, &spectranet_periph );
  if( periph_register_paging_events( event_type_string, &page_event,
				     &unpage_event ) )
    return 1;

  return 0;
}
