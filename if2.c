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

  if( !settings_current.if2_file ) {
    ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2_EJECT, 0 );
    return 0;
  }

  if ( !periph_interface2_active ) {
    return 0;
  }

  error = machine_load_rom_bank( memory_map_romcs, 0,
				 settings_current.if2_file,
				 2 * MEMORY_PAGE_SIZE );
  if( error ) return error;

  machine_current->ram.romcs = 1;

  memory_romcs_map();

  ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2_EJECT, 1 );

  return 0;
}
