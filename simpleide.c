/* simpleide.c: Simple 8-bit IDE interface routines
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

#include <config.h>

#include <libspectrum.h>

#include "periph.h"
#include "settings.h"
#include "simpleide.h"

/* Private function prototypes */

static libspectrum_byte simpleide_read( libspectrum_word port, int *attached );
static void simpleide_write( libspectrum_word port, libspectrum_byte data );

/* Data */

const periph_t simpleide_peripherals[] = {
  { 0x0010, 0x0000, simpleide_read, simpleide_write },
};

const size_t simpleide_peripherals_count =
  sizeof( simpleide_peripherals ) / sizeof( periph_t );

static libspectrum_ide_channel *simpleide_idechn;

/* Housekeeping functions */

int
simpleide_init( void )
{
  int error;

  error = libspectrum_ide_alloc( &simpleide_idechn, LIBSPECTRUM_IDE_DATA8 );
  if( error ) return error;

  error = libspectrum_ide_insert( simpleide_idechn, LIBSPECTRUM_IDE_MASTER,
                                  settings_current.simpleide_master_file );
  if( error ) return error;

  error = libspectrum_ide_insert( simpleide_idechn, LIBSPECTRUM_IDE_SLAVE,
                                  settings_current.simpleide_slave_file );
  if( error ) return error;

  return 0;
}

int
simpleide_end( void )
{
  return libspectrum_ide_free( simpleide_idechn );
}

void
simpleide_reset( void )
{
  libspectrum_ide_reset( simpleide_idechn );
}

int
simpleide_insert( const char *filename, libspectrum_ide_unit unit )
{
  int error;

  switch( unit ) {

    case LIBSPECTRUM_IDE_MASTER:
      error = settings_set_string( &settings_current.simpleide_master_file,
				   filename );
    break;
    
    case LIBSPECTRUM_IDE_SLAVE:
      error = settings_set_string( &settings_current.simpleide_slave_file,
				   filename );
    break;
    
    default:
      error = 1;
  }
  
  if( error ) return error;

  error = libspectrum_ide_insert( simpleide_idechn, unit, filename );

  return error;
}

int
simpleide_commit( libspectrum_ide_unit unit )
{
  int error;

  error = libspectrum_ide_commit( simpleide_idechn, unit );

  return error;
}

int
simpleide_eject( libspectrum_ide_unit unit )
{
  int error = 0;
  
  switch( unit ) {

    case LIBSPECTRUM_IDE_MASTER:
      if( settings_current.simpleide_master_file )
        free( settings_current.simpleide_master_file );
      settings_current.simpleide_master_file = NULL;
    break;
    
    case LIBSPECTRUM_IDE_SLAVE:
      if( settings_current.simpleide_slave_file )
        free( settings_current.simpleide_slave_file );
      settings_current.simpleide_slave_file = NULL;
    break;
    
    default:
      error = 1;
  }
  
  if( error ) return error;
  
  error = libspectrum_ide_eject( simpleide_idechn, unit );
  
  return error;
}

/* Port read/writes */

static libspectrum_byte
simpleide_read( libspectrum_word port, int *attached )
{
  libspectrum_ide_register idereg;
  
  if( !settings_current.simpleide_active ) return 0xff;
  
  *attached = 1;
  
  idereg = ( ( port >> 8 ) & 0x01 ) | ( ( port >> 11 ) & 0x06 );
  
  return libspectrum_ide_read( simpleide_idechn, idereg ); 
}  

static void
simpleide_write( libspectrum_word port, libspectrum_byte data )
{
  libspectrum_ide_register idereg;
  
  if( !settings_current.simpleide_active ) return;
   
  idereg = ( ( port >> 8 ) & 0x01 ) | ( ( port >> 11 ) & 0x06 );
  
  libspectrum_ide_write( simpleide_idechn, idereg, data ); 
}
