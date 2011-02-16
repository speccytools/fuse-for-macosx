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
#include "machine.h"
#include "memory.h"
#include "module.h"
#include "periph.h"
#include "settings.h"

/* TODO: these are 8K pages - but the Spectranet uses 4K pages */
static memory_page spectranet_map[2];

/* TODO: split these into ROM, SRAM, DRAM and unused */
#define SPECTRANET_PAGES 128
#define SPECTRANET_PAGE_LENGTH 0x2000
static libspectrum_byte *spectranet_memory[ SPECTRANET_PAGES ];
static int spectranet_memory_allocated = 0;

static int spectranet_paged;

static void
spectranet_reset( int hard_reset GCC_UNUSED )
{
  spectranet_paged = 1;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();
}

static void
spectranet_memory_map( void )
{
  if ( !spectranet_paged ) return;

  memory_map_read[0] = memory_map_write[0] = spectranet_map[0];
  memory_map_read[1] = memory_map_write[1] = spectranet_map[1];
}

static void
spectranet_activate( void )
{
  if( !spectranet_memory_allocated ) {
    int i;
    libspectrum_byte *memory =
      memory_pool_allocate_persistent( SPECTRANET_PAGES * SPECTRANET_PAGE_LENGTH, 1 );
    for( i = 0; i < SPECTRANET_PAGES; i++ )
      spectranet_memory[i] = memory + i * SPECTRANET_PAGE_LENGTH;
    spectranet_memory_allocated = 1;

    spectranet_map[0].page = spectranet_memory[0];
    spectranet_map[1].page = spectranet_memory[1];
  }
}

static module_info_t spectranet_module_info = {
  spectranet_reset,
  spectranet_memory_map,
  NULL,
  NULL,
  NULL
};

/* TODO: add actual ports here */
static const periph_port_t spectranet_ports[] = {
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
  int i;

  module_register( &spectranet_module_info );

  for( i = 0; i < 2; i++ ) {
    memory_page *page = &spectranet_map[i];

    page->writable = 1;
    page->contended = 1;
    page->bank = MEMORY_BANK_ROMCS;
    page->page_num = i;
    page->offset = 0;
    page->source = MEMORY_SOURCE_PERIPHERAL;
  }

  periph_register( PERIPH_TYPE_SPECTRANET, &spectranet_periph );

  return 0;
}
