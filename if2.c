/* if2.c: Interface II cartridge handling routines
   Copyright (c) 2003 Darren Salt, Fredrick Meunier, Philip Kendall
   Copyright (c) 2004 Fredrick Meunier

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

   Darren: linux@youmustbejoking.demon.co.uk
   Fred: fredm@spamcop.net

*/

#include <config.h>

#include <string.h>

#include "if2.h"
#include "machine.h"
#include "memory.h"
#include "periph.h"
#include "settings.h"
#include "ui/ui.h"

/* IF2 cart inserted? */
int if2_active = 0;

int
if2_insert( const char *filename )
{
  int error;

  if ( !periph_interface2_active ) {
    ui_error( UI_ERROR_ERROR,
	      "This machine does not support the Interface II" );
    return 1;
  }

  error = settings_set_string( &settings_current.if2_file, filename );
  if( error ) return error;

  machine_reset();

  return 0;
}

void
if2_eject( void )
{
  if ( !periph_interface2_active ) {
    ui_error( UI_ERROR_ERROR,
	      "This machine does not support the Interface II" );
    return;
  }

  if( settings_current.if2_file ) free( settings_current.if2_file );
  settings_current.if2_file = NULL;

  machine_current->ram.romcs = 0;

  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2_EJECT, 0 );

  machine_reset();
}

int
if2_reset( void )
{
  int error;

  if2_active = 0;

  if( !settings_current.if2_file ) {
    ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2_EJECT, 0 );
    return 0;
  }

  if ( !periph_interface2_active ) {
    return 0;
  }

  error = machine_load_rom_bank( memory_map_romcs, 0, 0,
				 settings_current.if2_file,
				 settings_default.if2_file,
				 2 * MEMORY_PAGE_SIZE );
  if( error ) return error;

  memory_map_romcs[0].source =
    memory_map_romcs[1].source = MEMORY_SOURCE_CARTRIDGE;

  machine_current->ram.romcs = 1;

  if2_active = 1;
  memory_romcs_map();

  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2_EJECT, 1 );

  return 0;
}

void
if2_memory_map( void )
{
  memory_map_read[0] = memory_map_write[0] = memory_map_romcs[0];
  memory_map_read[1] = memory_map_write[1] = memory_map_romcs[1];
}

int
if2_from_snapshot( libspectrum_snap *snap )
{
  if( !libspectrum_snap_interface2_active( snap ) ) return 0;

  if2_active = 1;
  machine_current->ram.romcs = 1;

  if( libspectrum_snap_interface2_rom( snap, 0 ) ) {

    memory_map_romcs[0].offset = 0;
    memory_map_romcs[0].page_num = 0;
    memory_map_romcs[0].page =
      memory_pool_allocate( 2 * MEMORY_PAGE_SIZE *
			    sizeof( libspectrum_byte ) );
    if( !memory_map_romcs[0].page ) {
      ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
      return 1;
    }

    memcpy( memory_map_romcs[0].page,
	    libspectrum_snap_interface2_rom( snap, 0 ), 2 * MEMORY_PAGE_SIZE );

    memory_map_romcs[1].offset = MEMORY_PAGE_SIZE;
    memory_map_romcs[1].page_num = 0;
    memory_map_romcs[1].page = memory_map_romcs[0].page + MEMORY_PAGE_SIZE;
    memory_map_romcs[1].source =
      memory_map_romcs[0].source = MEMORY_SOURCE_CARTRIDGE;
    }

  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2_EJECT, 1 );

  machine_current->memory_map();

  return 0;
}

int
if2_to_snapshot( libspectrum_snap *snap )
{
  libspectrum_byte *buffer;

  if( !if2_active ) return 0;

  libspectrum_snap_set_interface2_active( snap, 1 );

  buffer = malloc( 0x4000 * sizeof( libspectrum_byte ) );
  if( !buffer ) {
    ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  memcpy( buffer, memory_map_romcs[0].page, MEMORY_PAGE_SIZE );
  memcpy( buffer + MEMORY_PAGE_SIZE, memory_map_romcs[1].page,
	  MEMORY_PAGE_SIZE );
  libspectrum_snap_set_interface2_rom( snap, 0, buffer );

  return 0;
}
