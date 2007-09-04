/* divide.c: DivIDE interface routines
   Copyright (c) 2005 Matthew Westcott

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

   E-mail: Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#include <config.h>

#include <libspectrum.h>

#include "ide.h"
#include "machine.h"
#include "module.h"
#include "periph.h"
#include "settings.h"
#include "ui/ui.h"
#include "divide.h"

/* Private function prototypes */

static libspectrum_byte divide_ide_read( libspectrum_word port, int *attached );
static void divide_ide_write( libspectrum_word port, libspectrum_byte data );
static void divide_control_write( libspectrum_word port, libspectrum_byte data );
static void divide_control_write_internal( libspectrum_byte data );
static void divide_page( void );
static void divide_unpage( void );
static libspectrum_ide_register port_to_ide_register( libspectrum_byte port );

/* Data */

const periph_t divide_peripherals[] = {
  { 0x00e3, 0x00a3, divide_ide_read, divide_ide_write },
  { 0x00ff, 0x00e3, NULL, divide_control_write },
};

const size_t divide_peripherals_count =
  sizeof( divide_peripherals ) / sizeof( periph_t );

static const libspectrum_byte DIVIDE_CONTROL_CONMEM = 0x80;
static const libspectrum_byte DIVIDE_CONTROL_MAPRAM = 0x40;

int divide_automapping_enabled = 0;
int divide_active = 0;
static libspectrum_byte divide_control;

/* divide_automap tracks opcode fetches to entry and exit points to determine
   whether DivIDE memory *would* be paged in at this moment if mapram / wp
   flags allowed it */
static int divide_automap = 0;

static libspectrum_ide_channel *divide_idechn0;
static libspectrum_ide_channel *divide_idechn1;

#define DIVIDE_PAGES 4
#define DIVIDE_PAGE_LENGTH 0x2000
static libspectrum_byte divide_ram[ DIVIDE_PAGES ][ DIVIDE_PAGE_LENGTH ];
static libspectrum_byte divide_eprom[ DIVIDE_PAGE_LENGTH ];

static void divide_reset( int hard_reset );
static void divide_memory_map( void );

static module_info_t divide_module_info = {

  divide_reset,
  divide_memory_map,
  NULL,
  NULL,

};

/* Housekeeping functions */

int
divide_init( void )
{
  int error;

  error = libspectrum_ide_alloc( &divide_idechn0, LIBSPECTRUM_IDE_DATA16 );
  if( error ) return error;
  error = libspectrum_ide_alloc( &divide_idechn1, LIBSPECTRUM_IDE_DATA16 );
  if( error ) return error;
  
  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_DIVIDE_MASTER_EJECT, 0 );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_DIVIDE_SLAVE_EJECT, 0 );

  if( settings_current.divide_master_file ) {
    error = libspectrum_ide_insert( divide_idechn0, LIBSPECTRUM_IDE_MASTER,
				    settings_current.divide_master_file );
    if( error ) return error;
    ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_DIVIDE_MASTER_EJECT, 1 );
  }

  if( settings_current.divide_slave_file ) {
    error = libspectrum_ide_insert( divide_idechn0, LIBSPECTRUM_IDE_SLAVE,
				    settings_current.divide_slave_file );
    if( error ) return error;
    ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_DIVIDE_SLAVE_EJECT, 1 );
  }

  module_register( &divide_module_info );

  return error;
}

int
divide_end( void )
{
  int error;
  
  error = libspectrum_ide_free( divide_idechn0 );
  error = libspectrum_ide_free( divide_idechn1 ) || error;

  return error;
}

/* DivIDE does not page in immediately on a reset condition (we do that by
   trapping PC instead); however, it needs to perform housekeeping tasks upon
   reset */
static void
divide_reset( int hard_reset )
{
  divide_active = 0;

  if( !settings_current.divide_enabled ) return;
  
  if( hard_reset ) {
    divide_control = 0;
  } else {
    divide_control &= DIVIDE_CONTROL_MAPRAM;
  }
  divide_automap = 0;
  divide_refresh_page_state();

  libspectrum_ide_reset( divide_idechn0 );
  libspectrum_ide_reset( divide_idechn1 );
}

int
divide_insert( const char *filename, libspectrum_ide_unit unit )
{
  char **setting;
  ui_menu_item item;

  switch( unit ) {
  case LIBSPECTRUM_IDE_MASTER:
    setting = &settings_current.divide_master_file;
    item = UI_MENU_ITEM_MEDIA_IDE_DIVIDE_MASTER_EJECT;
    break;
    
  case LIBSPECTRUM_IDE_SLAVE:
    setting = &settings_current.divide_slave_file;
    item = UI_MENU_ITEM_MEDIA_IDE_DIVIDE_SLAVE_EJECT;
    break;
    
  default: return 1;
  }

  return ide_insert( filename, divide_idechn0, unit, divide_commit, setting,
		     item );
}

int
divide_commit( libspectrum_ide_unit unit )
{
  int error;

  error = libspectrum_ide_commit( divide_idechn0, unit );

  return error;
}

int
divide_eject( libspectrum_ide_unit unit )
{
  char **setting;
  ui_menu_item item;

  switch( unit ) {
  case LIBSPECTRUM_IDE_MASTER:
    setting = &settings_current.divide_master_file;
    item = UI_MENU_ITEM_MEDIA_IDE_DIVIDE_MASTER_EJECT;
    break;

  case LIBSPECTRUM_IDE_SLAVE:
    setting = &settings_current.divide_slave_file;
    item = UI_MENU_ITEM_MEDIA_IDE_DIVIDE_SLAVE_EJECT;
    break;
    
  default: return 1;
  }

  return ide_eject( divide_idechn0, unit, divide_commit, setting, item );
}

/* Port read/writes */

static libspectrum_ide_register
port_to_ide_register( libspectrum_byte port )
{
  switch( port & 0xff ) {
    case 0xa3:
      return LIBSPECTRUM_IDE_REGISTER_DATA;
    case 0xa7:
      return LIBSPECTRUM_IDE_REGISTER_ERROR_FEATURE;
    case 0xab:
      return LIBSPECTRUM_IDE_REGISTER_SECTOR_COUNT;
    case 0xaf:
      return LIBSPECTRUM_IDE_REGISTER_SECTOR;
    case 0xb3:
      return LIBSPECTRUM_IDE_REGISTER_CYLINDER_LOW;
    case 0xb7:
      return LIBSPECTRUM_IDE_REGISTER_CYLINDER_HIGH;
    case 0xbb:
      return LIBSPECTRUM_IDE_REGISTER_HEAD_DRIVE;
    default: /* 0xbf */
      return LIBSPECTRUM_IDE_REGISTER_COMMAND_STATUS;
  }
}

libspectrum_byte
divide_ide_read( libspectrum_word port, int *attached )
{
  int ide_register;
  if( !settings_current.divide_enabled ) return 0xff;

  *attached = 1;
  ide_register = port_to_ide_register( port );

  return libspectrum_ide_read( divide_idechn0, ide_register );
}

static void
divide_ide_write( libspectrum_word port, libspectrum_byte data )
{
  int ide_register;
  if( !settings_current.divide_enabled ) return;
  
  ide_register = port_to_ide_register( port );
  
  libspectrum_ide_write( divide_idechn0, ide_register, data );
}

static void
divide_control_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  int old_mapram;

  if( !settings_current.divide_enabled ) return;
  
  /* MAPRAM bit cannot be reset, only set */
  old_mapram = divide_control & DIVIDE_CONTROL_MAPRAM;
  divide_control_write_internal( data | old_mapram );
}

static void
divide_control_write_internal( libspectrum_byte data )
{
  divide_control = data;
  divide_refresh_page_state();
}

void
divide_set_automap( int state )
{
  divide_automap = state;
  divide_refresh_page_state();
}

void
divide_refresh_page_state( void )
{
  if( divide_control & DIVIDE_CONTROL_CONMEM ) {
    /* always paged in if conmem enabled */
    divide_page();
  } else if( settings_current.divide_wp
    || ( divide_control & DIVIDE_CONTROL_MAPRAM ) ) {
    /* automap in effect */
    if( divide_automap ) {
      divide_page();
    } else {
      divide_unpage();
    }
  } else {
    divide_unpage();
  }
}

static void
divide_page( void )
{
  divide_active = 1;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();
}

static void
divide_unpage( void )
{
  divide_active = 0;
  machine_current->ram.romcs = 0;
  machine_current->memory_map();
}

void
divide_memory_map( void )
{
  int upper_ram_page;

  if( !divide_active ) return;

  /* low bits of divide_control register give page number to use in upper
     bank; only lowest two bits on original 32K model */
  upper_ram_page = divide_control & (DIVIDE_PAGES - 1);
  
  if( divide_control & DIVIDE_CONTROL_CONMEM ) {
    memory_map_romcs[0].page = divide_eprom;
    memory_map_romcs[0].writable = !settings_current.divide_wp;
    memory_map_romcs[1].page = divide_ram[ upper_ram_page ];
    memory_map_romcs[1].writable = 1;
  } else {
    if( divide_control & DIVIDE_CONTROL_MAPRAM ) {
      memory_map_romcs[0].page = divide_ram[3];
      memory_map_romcs[0].writable = 0;
      memory_map_romcs[1].page = divide_ram[ upper_ram_page ];
      memory_map_romcs[1].writable = ( upper_ram_page != 3 );
    } else {
      memory_map_romcs[0].page = divide_eprom;
      memory_map_romcs[0].writable = 0;
      memory_map_romcs[1].page = divide_ram[ upper_ram_page ];
      memory_map_romcs[1].writable = 1;
    }
  }

  memory_map_read[0] = memory_map_write[0] = memory_map_romcs[0];
  memory_map_read[1] = memory_map_write[1] = memory_map_romcs[1];
}
