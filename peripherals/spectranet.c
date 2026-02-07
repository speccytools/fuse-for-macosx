/* spectranet.c: Spectranet emulation
   Copyright (c) 2011-2016 Philip Kendall
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

#include "compat.h"
#include "debugger/debugger.h"
#include "flash/am29f010.h"
#include "infrastructure/startup_manager.h"
#include "machine.h"
#include "memory_pages.h"
#include "module.h"
#include "nic/w5100.h"
#include "periph.h"
#include "peripherals/ula.h"
#include "peripherals/nic/dns_resolver.h"
#include "peripherals/fs/xfs.h"
#include "peripherals/fs/xfs_worker.h"
#include "peripherals/nic/spectranext_config.h"
#include "settings.h"
#include "spectranet.h"
#include "utils.h"
#include "ui/ui.h"

#include "config.h"

#include "peripherals/spectranet.h"

#include <string.h>
#include <stddef.h>

#ifdef BUILD_SPECTRANET

#define SPECTRANET_PAGES 256
#define SPECTRANET_PAGE_LENGTH 0x1000

#define SPECTRANET_ROM_LENGTH 0x20000
#define SPECTRANET_ROM_BASE 0

#define SPECTRANET_BUFFER_LENGTH 0x8000
#define SPECTRANET_BUFFER_BASE 0x40

#define SPECTRANET_RAM_LENGTH 0x20000
#define SPECTRANET_RAM_BASE 0xc0

static const int SPECTRANET_CPLD_VERSION = 0x03;

static memory_page spectranet_full_map[SPECTRANET_PAGES * MEMORY_PAGES_IN_4K];
static memory_page spectranet_current_map[MEMORY_PAGES_IN_16K];
static int spectranet_memory_allocated = 0;

static nic_w5100_t *w5100;
static flash_am29f010_t *flash_rom;

#endif

int spectranet_available = 0;
int spectranet_paged;
int spectranet_paged_via_io;
int spectranet_w5100_paged_a = 0, spectranet_w5100_paged_b = 0;
int spectranet_xfs_paged_a = 0, spectranet_xfs_paged_b = 0;
int spectranet_spectranext_config_paged_a = 0, spectranet_spectranext_config_paged_b = 0;

/* Whether the programmable trap is active */
int spectranet_programmable_trap_active;

/* Where the programmable trap will trigger if active */
libspectrum_word spectranet_programmable_trap;

/* True if the next write to 0x023b will set the MSB of the programmable trap */
static int trap_write_msb;

/* Whether the Spectranet's "suppress NMI" flipflop is set */
static int nmi_flipflop = 0;

static int spectranet_source;

/* Debugger events */
static const char * const event_type_string = "spectranet";
static int page_event, unpage_event;

void
spectranet_page( int via_io )
{
  if( spectranet_paged )
    return;

  spectranet_paged = 1;
  spectranet_paged_via_io = via_io;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();
  debugger_event( page_event );
}

void
spectranet_nmi( void )
{
  nmi_flipflop = 1;
  spectranet_page( 0 );
}

void
spectranet_unpage( void )
{
  if( !spectranet_paged )
    return;

  spectranet_paged = 0;
  spectranet_paged_via_io = 0;
  machine_current->ram.romcs = 0;
  machine_current->memory_map();
  debugger_event( unpage_event );
}

void
spectranet_retn( void )
{
  nmi_flipflop = 0;
}

int
spectranet_nmi_flipflop( void )
{
  return nmi_flipflop;
}

static void
spectranet_map_page( int dest, int source )
{
  int i;
  int w5100_page = source >= 0x40 && source < 0x48;
  int xfs_page = (source == XFS_SPECTRANET_PAGE);
  int spectranext_config_page = (source == SPECTRANEXT_CONTROLLER_PAGE);

  for( i = 0; i < MEMORY_PAGES_IN_4K; i++ )
    spectranet_current_map[dest * MEMORY_PAGES_IN_4K + i] =
      spectranet_full_map[source * MEMORY_PAGES_IN_4K + i];

  switch( dest )
  {
    case 1: 
      spectranet_w5100_paged_a = w5100_page;
      spectranet_xfs_paged_a = xfs_page;
      spectranet_spectranext_config_paged_a = spectranext_config_page;
      break;
    case 2: 
      spectranet_w5100_paged_b = w5100_page;
      spectranet_xfs_paged_b = xfs_page;
      spectranet_spectranext_config_paged_b = spectranext_config_page;
      break;
  }
}

static void
spectranet_hard_reset( void )
{
  spectranet_map_page( 1, 0xff ); /* Map something into 0x1000 to 0x1fff */
  spectranet_map_page( 2, 0xff ); /* And 0x2000 to 0x2fff */

  spectranet_programmable_trap = 0x0000;
  spectranet_programmable_trap_active = 0;
  trap_write_msb = 0;

  nmi_flipflop = 0;
}  

static void
spectranet_reset( int hard_reset )
{
  if( !periph_is_active( PERIPH_TYPE_SPECTRANET ) ) {
    spectranet_available = 0;
    spectranet_paged = 0;
    return;
  }

  spectranet_available = 1;
  spectranet_paged = !settings_current.spectranet_disable;

  if( hard_reset ) {
    spectranet_hard_reset();
  }
  
  // Reset XFS filesystem whenever Spectranet resets (hard or soft)
  // This ensures XFS state is clean on Spectrum reboot
  xfs_reset();

  if( spectranet_paged ) {
    machine_current->ram.romcs = 1;
    machine_current->memory_map();
  }

  nic_w5100_reset( w5100 );
}

static void
spectranet_memory_map( void )
{
  if( !spectranet_paged ) return;

  memory_map_romcs_full( spectranet_current_map );
}

static void
spectranet_activate( void )
{
  if( !spectranet_memory_allocated ) {

    int i, j;
    libspectrum_byte *rom;
    libspectrum_byte *ram;

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

    /* Pages 0x00 to 0x1f are the flash ROM */
    rom = memory_pool_allocate_persistent( SPECTRANET_ROM_LENGTH, 1 );
    memset( rom, 0xff, SPECTRANET_ROM_LENGTH );

    for( i = 0; i < SPECTRANET_ROM_LENGTH / SPECTRANET_PAGE_LENGTH; i++ ) {
      int base = (SPECTRANET_ROM_BASE + i) * MEMORY_PAGES_IN_4K;
      for( j = 0; j < MEMORY_PAGES_IN_4K; j++ ) {
        memory_page *page = &spectranet_full_map[base + j];
        page->page = rom + (i * MEMORY_PAGES_IN_4K + j) * MEMORY_PAGE_SIZE;
      }
    }

    flash_am29f010_init( flash_rom, rom );

    /* Pages 0x40 to 0x47 are the W5100 registers - handled in readbyte()
       and writebyte() */

    /* Pages 0xc0 to 0xff are the RAM */
    ram = memory_pool_allocate_persistent( SPECTRANET_RAM_LENGTH, 1 );
    
    utils_file spectranet_rom;
    if( utils_read_auxiliary_file( "spectranet.rom", &spectranet_rom, UTILS_AUXILIARY_ROM ) != -1 ) {
      memcpy(rom, spectranet_rom.buffer, SPECTRANET_ROM_LENGTH);
      utils_close_file(&spectranet_rom);
    }

    for( i = 0; i < SPECTRANET_RAM_LENGTH / SPECTRANET_PAGE_LENGTH; i++ ) {
      int base = (SPECTRANET_RAM_BASE + i) * MEMORY_PAGES_IN_4K;
      for( j = 0; j < MEMORY_PAGES_IN_4K; j++ ) {
        memory_page *page = &spectranet_full_map[base + j];
        page->writable = 1;
        page->page = ram + (i * MEMORY_PAGES_IN_4K + j) * MEMORY_PAGE_SIZE;
      }
    }

    spectranet_memory_allocated = 1;

    spectranet_map_page( 0, 0x00 );
    spectranet_map_page( 3, 0xc0 );

    spectranet_hard_reset();
  }
}

static void
spectranet_enabled_snapshot( libspectrum_snap *snap )
{
  settings_current.spectranet = libspectrum_snap_spectranet_active( snap );
}

static void
spectranet_from_snapshot( libspectrum_snap *snap )
{
  if( !libspectrum_snap_spectranet_active( snap ) )
    return;

  if( periph_is_active( PERIPH_TYPE_SPECTRANET ) ) {

    spectranet_programmable_trap =
      libspectrum_snap_spectranet_programmable_trap( snap );
    spectranet_programmable_trap_active = 
      libspectrum_snap_spectranet_programmable_trap_active( snap );
    trap_write_msb =
      libspectrum_snap_spectranet_programmable_trap_msb( snap );

    settings_current.spectranet_disable =
      libspectrum_snap_spectranet_all_traps_disabled( snap );

    spectranet_map_page( 1, libspectrum_snap_spectranet_page_a( snap ) );
    spectranet_map_page( 2, libspectrum_snap_spectranet_page_b( snap ) );

    nmi_flipflop = libspectrum_snap_spectranet_nmi_flipflop( snap );

    if( libspectrum_snap_spectranet_paged( snap ) ) {
      spectranet_page( libspectrum_snap_spectranet_paged_via_io( snap ) );
      memory_map_romcs_full( spectranet_current_map );
    }
    else
      spectranet_unpage();

    nic_w5100_from_snapshot( w5100,
      libspectrum_snap_spectranet_w5100( snap, 0 ) );

    memcpy(
      spectranet_full_map[SPECTRANET_ROM_BASE * MEMORY_PAGES_IN_4K].page,
      libspectrum_snap_spectranet_flash( snap, 0 ), SPECTRANET_ROM_LENGTH );
    memcpy(
      spectranet_full_map[SPECTRANET_RAM_BASE * MEMORY_PAGES_IN_4K].page,
      libspectrum_snap_spectranet_ram( snap, 0 ), SPECTRANET_RAM_LENGTH );
  }
}

static void
spectranet_to_snapshot( libspectrum_snap *snap )
{
  libspectrum_byte *snap_buffer, *src;

  int active = periph_is_active( PERIPH_TYPE_SPECTRANET );
  libspectrum_snap_set_spectranet_active( snap, active );

  if( !active )
    return;

  libspectrum_snap_set_spectranet_paged( snap, spectranet_paged );
  libspectrum_snap_set_spectranet_paged_via_io( snap, spectranet_paged_via_io );
  libspectrum_snap_set_spectranet_programmable_trap( snap,
    spectranet_programmable_trap );
  libspectrum_snap_set_spectranet_programmable_trap_active( snap,
    spectranet_programmable_trap_active );
  libspectrum_snap_set_spectranet_programmable_trap_msb( snap, trap_write_msb );
  libspectrum_snap_set_spectranet_deny_downstream_a15( snap, 0 );
  libspectrum_snap_set_spectranet_nmi_flipflop( snap, nmi_flipflop );

  libspectrum_snap_set_spectranet_all_traps_disabled( snap,
    settings_current.spectranet_disable );
  libspectrum_snap_set_spectranet_rst8_trap_disabled( snap, 0 );
  libspectrum_snap_set_spectranet_page_a(
    snap, spectranet_current_map[1 * MEMORY_PAGES_IN_4K].page_num );
  libspectrum_snap_set_spectranet_page_b(
    snap, spectranet_current_map[2 * MEMORY_PAGES_IN_4K].page_num );

  libspectrum_snap_set_spectranet_w5100( snap, 0,
    nic_w5100_to_snapshot( w5100 ) );

  snap_buffer = libspectrum_new( libspectrum_byte, SPECTRANET_ROM_LENGTH );

  src = spectranet_full_map[SPECTRANET_ROM_BASE * MEMORY_PAGES_IN_4K].page;
  memcpy( snap_buffer, src, SPECTRANET_ROM_LENGTH );
  libspectrum_snap_set_spectranet_flash( snap, 0, snap_buffer );

  snap_buffer = libspectrum_new( libspectrum_byte, SPECTRANET_RAM_LENGTH );

  src = spectranet_full_map[SPECTRANET_RAM_BASE * MEMORY_PAGES_IN_4K].page;
  memcpy( snap_buffer, src, SPECTRANET_RAM_LENGTH );
  libspectrum_snap_set_spectranet_ram( snap, 0, snap_buffer );
}

static module_info_t spectranet_module_info = {

  /* .reset = */ spectranet_reset,
  /* .romcs = */ spectranet_memory_map,
  /* .snapshot_enabled = */ spectranet_enabled_snapshot,
  /* .snapshot_from = */ spectranet_from_snapshot,
  /* .snapshot_to = */ spectranet_to_snapshot,

};

static void
spectranet_page_a( libspectrum_word port, libspectrum_byte data )
{
  spectranet_map_page( 1, data );
  memory_map_romcs_full( spectranet_current_map );
}

static void
spectranet_page_b( libspectrum_word port, libspectrum_byte data )
{
  spectranet_map_page( 2, data );
  memory_map_romcs_full( spectranet_current_map );
}

static libspectrum_byte
spectranet_cpld_version( libspectrum_word port, libspectrum_byte *attached )
{
  *attached = 0xff; /* TODO: check this */
  return SPECTRANET_CPLD_VERSION;
}

static void
spectranet_trap( libspectrum_word port, libspectrum_byte data )
{
  if( trap_write_msb )
    spectranet_programmable_trap =
      (spectranet_programmable_trap & 0x00ff) | (data << 8);
  else
    spectranet_programmable_trap =
      (spectranet_programmable_trap & 0xff00) | data;

  trap_write_msb = !trap_write_msb;
}

static libspectrum_byte
spectranet_control_read( libspectrum_word port, libspectrum_byte *attached )
{
  libspectrum_byte b = ula_last_byte() & 0x07;
  if( spectranet_programmable_trap_active )
    b |= 0x08;
  if( ( machine_current->capabilities &
    LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) &&
    machine_current->ram.last_byte & 0x08 )
    b |= 0x10;

  *attached = 0xff; /* TODO: check this */

  return b;
}

static void
spectranet_control_write( libspectrum_word port, libspectrum_byte data )
{
  if( data & 0x01 )
    spectranet_page( 1 );
  else if( spectranet_paged_via_io )
    spectranet_unpage();

  spectranet_programmable_trap_active = data & 0x08;
}

static const periph_port_t spectranet_ports[] = {
  { 0xffff, 0x003b, NULL, spectranet_page_a },
  { 0xffff, 0x013b, NULL, spectranet_page_b },
  { 0xffff, 0x023b, spectranet_cpld_version, spectranet_trap },
  { 0xffff, 0x033b, spectranet_control_read, spectranet_control_write },
  { 0, 0, NULL, NULL }
};

static const periph_t spectranet_periph = {
  /* .option = */ &settings_current.spectranet,
  /* .ports = */ spectranet_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ spectranet_activate,
};

static int
spectranet_init( void *context )
{
  module_register( &spectranet_module_info );
  spectranet_source = memory_source_register( "Spectranet" );
  periph_register( PERIPH_TYPE_SPECTRANET, &spectranet_periph );
  periph_register_paging_events( event_type_string, &page_event,
				 &unpage_event );

  dns_resolver_init();
  spectranext_config_init();
  xfs_init();
  w5100 = nic_w5100_alloc();
  flash_rom = flash_am29f010_alloc();

  return 0;
}

static void
spectranet_end( void )
{
  nic_w5100_free( w5100 );
  flash_am29f010_free( flash_rom );
}

void
spectranet_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DEBUGGER,
    STARTUP_MANAGER_MODULE_MEMORY,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_SPECTRANET, dependencies,
                            ARRAY_SIZE( dependencies ), spectranet_init, NULL,
                            spectranet_end );
}

static libspectrum_word
get_w5100_register( memory_page *page, libspectrum_word address )
{
  libspectrum_word base_address =
    ( page->page_num - SPECTRANET_BUFFER_BASE ) * SPECTRANET_PAGE_LENGTH;
  return base_address + ( address & 0xfff );
}


libspectrum_byte
spectranet_w5100_read( memory_page *page, libspectrum_word address )
{
  return nic_w5100_read( w5100, get_w5100_register( page, address ) );
}

void
spectranet_w5100_write( memory_page *page, libspectrum_word address, libspectrum_byte b )
{
  address &= 0xfff;
  nic_w5100_write( w5100, get_w5100_register( page, address ), b );
}

libspectrum_byte
spectranet_xfs_read( memory_page *page, libspectrum_word address )
{
  return xfs_read( page, address );
}

void
spectranet_xfs_write( memory_page *page, libspectrum_word address, libspectrum_byte b )
{
  xfs_write( page, address, b );
}

libspectrum_byte
spectranet_spectranext_config_read( memory_page *page, libspectrum_word address )
{
  return spectranext_config_read( page, address );
}

void
spectranet_spectranext_config_write( memory_page *page, libspectrum_word address, libspectrum_byte b )
{
  spectranext_config_write( page, address, b );
}

void
spectranet_flash_rom_write( libspectrum_word address, libspectrum_byte b )
{
  int pageb_page = spectranet_current_map[2 * MEMORY_PAGES_IN_4K].page_num;
  
  if( pageb_page < SPECTRANET_ROM_LENGTH / SPECTRANET_PAGE_LENGTH ) {
    /* Which 16Kb flash page are we accessing */
    int flash_page = pageb_page / 4;
    /* And at what offset into that page */
    libspectrum_word flash_address = (pageb_page % 4) * SPECTRANET_PAGE_LENGTH + (address & 0xfff);
    flash_am29f010_write( flash_rom, flash_page, flash_address, b );
  }
}

libspectrum_byte*
spectranet_get_config_page( void )
{
  if( !spectranet_memory_allocated ) {
    return NULL;
  }

  // Configuration page is 0x1F (31)
  // Get the first memory page of the configuration page
  const memory_page* first_page = &spectranet_full_map[0x1F * MEMORY_PAGES_IN_4K];
  
  // The page->page pointer points to the start of the 4KB ROM page
  return first_page->page;
}


// Access Spectranet ROM page 0x1F (configuration page) via memory map
const uint8_t* spectranet_config_get_memory(void)
{
    return (const uint8_t*)spectranet_get_config_page();
}

// Read 16-bit little-endian value from memory
static uint16_t read_uint16(const uint8_t* memory, uint16_t offset)
{
    return memory[offset] | (memory[offset + 1] << 8);
}

// Write 16-bit little-endian value to memory
static void write_uint16(uint8_t* memory, uint16_t offset, uint16_t value)
{
    memory[offset] = value & 0xFF;
    memory[offset + 1] = (value >> 8) & 0xFF;
}

// Find a section in the configuration
static const uint8_t* find_section(const uint8_t* memory, uint16_t section_id, uint16_t* section_size)
{
    uint16_t total_size = read_uint16(memory, 0);
    uint16_t pointer = 2;

    while (pointer < total_size && pointer < SPECTRANET_CONFIG_PAGE_SIZE - 4)
    {
        uint16_t current_section_id = read_uint16(memory, pointer);
        pointer += 2;
        
        if (current_section_id == section_id)
        {
            *section_size = read_uint16(memory, pointer);
            return memory + pointer;
        }
        
        // Skip this section
        uint16_t skip_size = read_uint16(memory, pointer);
        pointer += 2 + skip_size;
    }

    return NULL;
}

// Find insertion point for a new section (returns offset where section should be inserted)
static uint16_t find_section_insertion_point(uint8_t* memory, uint16_t section_id)
{
    uint16_t total_size = read_uint16(memory, 0);
    
    // Handle empty or uninitialized configuration
    if (total_size == 0 || total_size == 0xFFFF || total_size >= SPECTRANET_CONFIG_PAGE_SIZE)
    {
        return 2; // Insert right after total_size field
    }
    
    uint16_t pointer = 2;

    while (pointer < total_size && pointer < SPECTRANET_CONFIG_PAGE_SIZE - 4)
    {
        uint16_t current_section_id = read_uint16(memory, pointer);
        pointer += 2;
        
        if (current_section_id == section_id)
        {
            return pointer - 2; // Section already exists
        }
        
        // Skip this section
        uint16_t skip_size = read_uint16(memory, pointer);
        pointer += 2 + skip_size;
    }

    return pointer; // Insert at end
}

// Create a new section if it doesn't exist, return pointer to section
static uint8_t* ensure_section_exists(uint8_t* memory, uint16_t section_id, uint16_t* section_size_out)
{
    uint16_t section_size;
    const uint8_t* existing = find_section(memory, section_id, &section_size);
    if (existing != NULL)
    {
        *section_size_out = section_size;
        return (uint8_t*)existing;
    }

    // Need to create new section
    uint16_t total_size = read_uint16(memory, 0);
    
    // Initialize configuration if empty
    if (total_size == 0 || total_size == 0xFFFF)
    {
        total_size = 2; // Just the total_size field
        write_uint16(memory, 0, total_size);
    }
    
    uint16_t insert_offset = find_section_insertion_point(memory, section_id);
    
    // Check if we have space (section header + terminator)
    if (insert_offset + 4 + 2 > SPECTRANET_CONFIG_PAGE_SIZE - 2)
    {
        return NULL; // Not enough space
    }

    // Create empty section: [section_id] [size=0] (no items yet)
    // section-size is the size of items data only, not including section-id and section-size fields
    write_uint16(memory, insert_offset, section_id);
    write_uint16(memory, insert_offset + 2, 0); // Size = 0 (no items yet)
    
    // Update total size
    // Total size = insert_offset + 2 (section_id) + 2 (section_size) + 0 (items) = insert_offset + 4
    uint16_t new_total_size = insert_offset + 4;
    write_uint16(memory, 0, new_total_size);
    
    // Ensure terminator is present
    memory[new_total_size] = 0xFF;
    memory[new_total_size + 1] = 0xFF;
    
    *section_size_out = 0; // Empty section has 0 items
    return memory + insert_offset + 2;
}

// Append a new item to a section
static int append_item_to_section(uint8_t* section_start, uint16_t* section_size, uint8_t item_id, const void* data, uint16_t data_size)
{
    // section_start points to section_size field
    // section_size is the size of items data only (item IDs + cfg data)
    uint8_t* section_data_start = section_start + 2; // After section_size field, where items start
    uint16_t current_items_size = *section_size; // Current size of items data
    
    // Check if we have space
    uint16_t new_item_size = 1 + data_size; // item_id (1 byte) + data
    uint16_t new_section_size = *section_size + new_item_size; // New size of items data
    
    // Calculate absolute offset in memory
    uint8_t* memory = (uint8_t*)spectranet_config_get_memory();
    uint16_t section_size_offset = (uint16_t)(section_start - memory); // Offset to section_size field
    uint16_t section_id_offset = section_size_offset - 2; // Offset to section_id field (section_size is 2 bytes after section_id)
    uint16_t total_size = read_uint16(memory, 0);
    
    // Check bounds: section_id (2) + section_size (2) + section_data (new_section_size)
    if (section_id_offset + 2 + 2 + new_section_size > SPECTRANET_CONFIG_PAGE_SIZE)
    {
        return -1; // Not enough space
    }
    
    // Append item
    uint8_t* item_ptr = section_data_start + current_items_size;
    *item_ptr = item_id;
    memcpy(item_ptr + 1, data, data_size);
    
    // Update section size
    write_uint16(memory, section_size_offset, new_section_size);
    *section_size = new_section_size;
    
    // Update total size if section grew
    // section_start points to section_size field, so:
    // - section_id is at section_size_offset - 2
    // - section_size is at section_size_offset
    // - section_data starts at section_size_offset + 2
    // - section_data ends at section_size_offset + 2 + new_section_size
    // So total_size = section_size_offset + 2 + new_section_size
    uint16_t new_total_size = section_size_offset + 2 + new_section_size;
    if (new_total_size > total_size)
    {
        write_uint16(memory, 0, new_total_size);
        // Ensure terminator
        memory[new_total_size] = 0xFF;
        memory[new_total_size + 1] = 0xFF;
    }
    
    return 0;
}

// Find an item within a section
static const uint8_t* find_item(const uint8_t* section_start, uint16_t section_size, uint8_t item_id, uint8_t* item_type, uint16_t* item_data_size)
{
    const uint8_t* pointer = section_start + 2; // Skip section size
    const uint8_t* section_end = section_start + 2 + section_size;

    while (pointer < section_end)
    {
        uint8_t current_item_id = *pointer;
        pointer++;

        if (current_item_id == item_id) // Exact match (type bits are part of the ID)
        {
            *item_type = current_item_id & CONFIG_ITEM_TYPE_MASK;
            
            switch (*item_type)
            {
                case CONFIG_ITEM_TYPE_STRING:
                {
                    // Find null terminator
                    const uint8_t* str_start = pointer;
                    while (pointer < section_end && *pointer != 0)
                    {
                        pointer++;
                    }
                    *item_data_size = (pointer - str_start) + 1; // Include null terminator
                    return str_start;
                }
                case CONFIG_ITEM_TYPE_BYTE:
                    *item_data_size = 1;
                    return pointer;
                case CONFIG_ITEM_TYPE_INT:
                    *item_data_size = 2;
                    return pointer;
                default:
                    return NULL;
            }
        }

        // Skip this item
        uint8_t skip_type = current_item_id & CONFIG_ITEM_TYPE_MASK;
        switch (skip_type)
        {
            case CONFIG_ITEM_TYPE_STRING:
                while (pointer < section_end && *pointer != 0)
                {
                    pointer++;
                }
                pointer++; // Skip null terminator
                break;
            case CONFIG_ITEM_TYPE_BYTE:
                pointer++;
                break;
            case CONFIG_ITEM_TYPE_INT:
                pointer += 2;
                break;
        }
    }

    return NULL;
}

// Get total configuration size
uint16_t spectranet_config_get_total_size(void)
{
    const uint8_t* memory = spectranet_config_get_memory();
    if (memory == NULL)
    {
        return 0;
    }
    return read_uint16(memory, 0);
}

// Check if configuration is valid
bool spectranet_config_is_valid(void)
{
    const uint8_t* memory = spectranet_config_get_memory();
    if (memory == NULL)
    {
        return false;
    }
    
    uint16_t total_size = read_uint16(memory, 0);
    
    // Check for terminator (0xFF 0xFF) at end
    if (total_size + 2 <= SPECTRANET_CONFIG_PAGE_SIZE)
    {
        if (memory[total_size] == 0xFF && memory[total_size + 1] == 0xFF)
        {
            return true;
        }
    }
    
    return false;
}

// Read string configuration item
int spectranet_config_get_string(uint16_t section_id, uint8_t item_id, char* buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0)
    {
        return -1;
    }

    const uint8_t* memory = spectranet_config_get_memory();
    if (memory == NULL)
    {
        return -1;
    }
    
    uint16_t section_size;
    const uint8_t* section = find_section(memory, section_id, &section_size);
    
    if (section == NULL)
    {
        return -1;
    }

    uint8_t item_type;
    uint16_t item_data_size;
    const uint8_t* item_data = find_item(section, section_size, item_id, &item_type, &item_data_size);
    
    if (item_data == NULL || item_type != CONFIG_ITEM_TYPE_STRING)
    {
        return -1;
    }

    // Copy string (item_data_size includes null terminator)
    size_t copy_size = (item_data_size < buffer_size) ? item_data_size : buffer_size - 1;
    memcpy(buffer, item_data, copy_size);
    buffer[copy_size] = '\0'; // Ensure null termination
    
    return 0;
}

// Read byte configuration item
int spectranet_config_get_byte(uint16_t section_id, uint8_t item_id, uint8_t* value)
{
    if (value == NULL)
    {
        return -1;
    }

    const uint8_t* memory = spectranet_config_get_memory();
    if (memory == NULL)
    {
        return -1;
    }
    
    uint16_t section_size;
    const uint8_t* section = find_section(memory, section_id, &section_size);
    
    if (section == NULL)
    {
        return -1;
    }

    uint8_t item_type;
    uint16_t item_data_size;
    const uint8_t* item_data = find_item(section, section_size, item_id, &item_type, &item_data_size);
    
    if (item_data == NULL || item_type != CONFIG_ITEM_TYPE_BYTE)
    {
        return -1;
    }

    *value = *item_data;
    return 0;
}

// Read int configuration item
int spectranet_config_get_int(uint16_t section_id, uint8_t item_id, uint16_t* value)
{
    if (value == NULL)
    {
        return -1;
    }

    const uint8_t* memory = spectranet_config_get_memory();
    if (memory == NULL)
    {
        return -1;
    }
    
    uint16_t section_size;
    const uint8_t* section = find_section(memory, section_id, &section_size);
    
    if (section == NULL)
    {
        return -1;
    }

    uint8_t item_type;
    uint16_t item_data_size;
    const uint8_t* item_data = find_item(section, section_size, item_id, &item_type, &item_data_size);
    
    if (item_data == NULL || item_type != CONFIG_ITEM_TYPE_INT)
    {
        return -1;
    }

    *value = read_uint16(item_data, 0);
    return 0;
}

// Write string configuration item (modifies ROM memory - use with caution!)
int spectranet_config_set_string(uint16_t section_id, uint8_t item_id, const char* value)
{
    if (value == NULL)
    {
        return -1;
    }

    uint8_t* memory = (uint8_t*)spectranet_config_get_memory(); // Cast away const for writing
    if (memory == NULL)
    {
        return -1;
    }
    
    uint16_t section_size;
    uint8_t* section = ensure_section_exists(memory, section_id, &section_size);
    
    if (section == NULL)
    {
        return -1; // Failed to create section
    }

    uint8_t item_type;
    uint16_t item_data_size;
    uint8_t* item_data = (uint8_t*)find_item(section, section_size, item_id, &item_type, &item_data_size);
    
    if (item_data == NULL)
    {
        // Item doesn't exist - create it
        size_t value_len = strlen(value);
        uint16_t data_size = (uint16_t)(value_len + 1); // Include null terminator
        
        // Ensure item_id has correct type bits
        uint8_t typed_item_id = item_id | CONFIG_ITEM_TYPE_STRING;
        
        if (append_item_to_section(section, &section_size, typed_item_id, value, data_size) != 0)
        {
            return -1; // Failed to append item
        }
        return 0;
    }

    if (item_type != CONFIG_ITEM_TYPE_STRING)
    {
        return -1; // Wrong type
    }

    size_t value_len = strlen(value);
    if (value_len + 1 > item_data_size)
    {
        return -1; // New value too long for existing space
    }

    // Write new string
    memcpy(item_data, value, value_len);
    item_data[value_len] = '\0';
    
    return 0;
}

// Write byte configuration item
int spectranet_config_set_byte(uint16_t section_id, uint8_t item_id, uint8_t value)
{
    uint8_t* memory = (uint8_t*)spectranet_config_get_memory();
    if (memory == NULL)
    {
        return -1;
    }
    
    uint16_t section_size;
    uint8_t* section = ensure_section_exists(memory, section_id, &section_size);
    
    if (section == NULL)
    {
        return -1; // Failed to create section
    }

    uint8_t item_type;
    uint16_t item_data_size;
    uint8_t* item_data = (uint8_t*)find_item(section, section_size, item_id, &item_type, &item_data_size);
    
    if (item_data == NULL)
    {
        // Item doesn't exist - create it
        // Ensure item_id has correct type bits
        uint8_t typed_item_id = item_id | CONFIG_ITEM_TYPE_BYTE;
        
        if (append_item_to_section(section, &section_size, typed_item_id, &value, 1) != 0)
        {
            return -1; // Failed to append item
        }
        return 0;
    }

    if (item_type != CONFIG_ITEM_TYPE_BYTE)
    {
        return -1;
    }

    *item_data = value;
    return 0;
}

// Write int configuration item
int spectranet_config_set_int(uint16_t section_id, uint8_t item_id, uint16_t value)
{
    uint8_t* memory = (uint8_t*)spectranet_config_get_memory();
    if (memory == NULL)
    {
        return -1;
    }
    
    uint16_t section_size;
    const uint8_t* section = find_section(memory, section_id, &section_size);
    
    if (section == NULL)
    {
        return -1;
    }

    uint8_t item_type;
    uint16_t item_data_size;
    uint8_t* item_data = (uint8_t*)find_item(section, section_size, item_id, &item_type, &item_data_size);
    
    if (item_data == NULL)
    {
        return -1;
    }

    if (item_type != CONFIG_ITEM_TYPE_INT)
    {
        return -1;
    }

    write_uint16(item_data, 0, value);
    return 0;
}
