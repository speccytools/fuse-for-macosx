/* zxcf.c: ZXCF interface routines
   Copyright (c) 2003-2004 Garry Lancaster,
		 2004 Philip Kendall
		 
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

   E-mail: Philip Kendall <pak21-fuse@srcf.ucam.org>
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <compat.h>

#include <libspectrum.h>

#include "machine.h"
#include "memory.h"
#include "periph.h"
#include "settings.h"
#include "ui/ui.h"
#include "zxcf.h"

/*
  TBD: Allow memory size selection (128K/512K/1024K)
  TBD: Add secondary channel
*/


/* Private function prototype */

static void set_zxcf_bank( int bank );
static libspectrum_byte zxcf_memctl_read( libspectrum_word port,
					  int *attached );
static libspectrum_byte zxcf_ide_read( libspectrum_word port, int *attached );
static void zxcf_ide_write( libspectrum_word port, libspectrum_byte data );

/* Data */

const periph_t zxcf_peripherals[] = {
  { 0x10f4, 0x10b4, zxcf_memctl_read, zxcf_memctl_write },
  { 0x10f4, 0x00b4, zxcf_ide_read, zxcf_ide_write },
};

const size_t zxcf_peripherals_count =
  sizeof( zxcf_peripherals ) / sizeof( periph_t );

static int zxcf_writeenable;

static libspectrum_ide_channel *zxcf_idechn;

static libspectrum_byte ZXCFMEM[ 64 ][ 0x4000 ];

static libspectrum_byte last_memctl;

/* Housekeeping functions */

int
zxcf_init( void )
{
  int error;

  last_memctl = 0x80;
                                
  error = libspectrum_ide_alloc( &zxcf_idechn, LIBSPECTRUM_IDE_DATA16 );
  if( error ) return error;

  ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_ZXCF_EJECT, 0 );

  if( settings_current.zxcf_pri_file ) {
    error = libspectrum_ide_insert( zxcf_idechn, LIBSPECTRUM_IDE_MASTER,
				    settings_current.zxcf_pri_file );
    if( error ) return error;
    ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_ZXCF_EJECT, 1 );
  }

  return 0;
}

int
zxcf_end( void )
{
  return libspectrum_ide_free( zxcf_idechn );
}

void
zxcf_reset( void )
{
  set_zxcf_bank( 0 );
  zxcf_writeenable = 0;

  libspectrum_ide_reset( zxcf_idechn );
}

int
zxcf_insert( const char *filename )
{
  int error;

  error = settings_set_string( &settings_current.zxcf_pri_file, filename );
  if( error ) return error;

  error = libspectrum_ide_insert( zxcf_idechn, LIBSPECTRUM_IDE_MASTER,
				  filename );
  if( error ) return error;

  error = ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_ZXCF_EJECT, 1 );
  if( error ) return error;

  return 0;
}

int
zxcf_commit( void )
{
  int error;

  error = libspectrum_ide_commit( zxcf_idechn, LIBSPECTRUM_IDE_MASTER );

  return error;
}

int
zxcf_eject( void )
{
  int error;
  
  free( settings_current.simpleide_master_file );
  settings_current.simpleide_master_file = NULL;
  
  error = libspectrum_ide_eject( zxcf_idechn, LIBSPECTRUM_IDE_MASTER );
  if( error ) return error;

  error = ui_menu_activate( UI_MENU_ITEM_MEDIA_IDE_ZXCF_EJECT, 1 );
  if( error ) return error;

  return 0;
}

static void
set_zxcf_bank( int bank )
{
  memory_page *page;
  size_t i, offset;

  for( i = 0; i < 2; i++ ) {

    page = &memory_map_romcs[i];
    offset = i & 1 ? MEMORY_PAGE_SIZE : 0x0000;
    
    page->page = &ZXCFMEM[ bank ][ offset ];
    page->writable = zxcf_writeenable;
    page->contended = 0;
    
    page->page_num = bank;
    page->offset = offset;
  }
}  

/* Port read/writes */

static libspectrum_byte
zxcf_memctl_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !settings_current.zxcf_active ) return 0xff;

  *attached = 1;

  return 0xff;
}

void
zxcf_memctl_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  if( !settings_current.zxcf_active ) return;

  last_memctl = data;

  /* Bit 7 MEMOFF: 0=mem on, 1 =mem off */
  machine_current->ram.romcs = ( data & 0x80 ) ? 0 : 1;

  /* Bit 6 /MWRPROT: 0=mem protected, 1=mem writable */
  zxcf_writeenable = ( data & 0x40 ) ? 1 : 0;
  
  /* Bits 5-0: MEMBANK */
  set_zxcf_bank( data & 0x3f );

  machine_current->memory_map();
}

libspectrum_byte
zxcf_last_memctl( void )
{
  return last_memctl;
}

static libspectrum_byte
zxcf_ide_read( libspectrum_word port, int *attached )
{
  libspectrum_ide_register idereg = ( port >> 8 ) & 0x07;
  
  if( !settings_current.zxcf_active ) return 0xff;

  *attached = 1;

  return libspectrum_ide_read( zxcf_idechn, idereg ); 
}

static void
zxcf_ide_write( libspectrum_word port, libspectrum_byte data )
{
  libspectrum_ide_register idereg;
  
  if( !settings_current.zxcf_active ) return;

  idereg = ( port >> 8 ) & 0x07;
  libspectrum_ide_write( zxcf_idechn, idereg, data ); 
}

libspectrum_byte*
zxcf_ram( size_t page )
{
  return ZXCFMEM[ page ];
}

void
zxcf_memory_map( void )
{
  if( !settings_current.zxcf_upload ) {
    memory_map_read[0] = memory_map_romcs[0];
    memory_map_read[1] = memory_map_romcs[1];
  }

  memory_map_write[0] = memory_map_romcs[0];
  memory_map_write[1] = memory_map_romcs[1];
}
