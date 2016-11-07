/* FuseMenus.m: Functions which provide an interface for C code to call the
                FuseController's functions. Inspiration and code fragments from
                the Macintosh OS X SDL port of Atari800

   Copyright (c) 2002-2003 Fredrick Meunier, Mark Grebe <atarimac@cox.net>

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

*/

#import "Emulator.h"
#import "FuseController.h"
#import "FuseMenus.h"

#include "divide.h"
#include "if1.h"
#include "menu.h"
#include "opus.h"
#include "settings.h"
#include "simpleide.h"
#include "tape.h"
#include "uimedia.h"
#include "zxatasp.h"
#include "zxcf.h"

void SetEmulationHz( float hz )
{
  [[Emulator instance] setEmulationHz:hz];
}

int
menu_check_media_changed( void )
{
  int confirm, i;

  confirm = tape_close(); if( confirm ) return 1;

  confirm = ui_media_drive_eject_all();
  if( confirm ) return 1;
  
  for( i = 0; i < 8; i++ ) {
    confirm = if1_mdr_eject( i );
    if( confirm ) return 1;
  }

  if( settings_current.simpleide_master_file ) {
    confirm = simpleide_eject( LIBSPECTRUM_IDE_MASTER );
    if( confirm ) return 1;
  }

  if( settings_current.simpleide_slave_file ) {
    confirm = simpleide_eject( LIBSPECTRUM_IDE_SLAVE );
    if( confirm ) return 1;
  }

  if( settings_current.zxatasp_master_file ) {
    confirm = zxatasp_eject( LIBSPECTRUM_IDE_MASTER );
    if( confirm ) return 1;
  }

  if( settings_current.zxatasp_slave_file ) {
    confirm = zxatasp_eject( LIBSPECTRUM_IDE_SLAVE );
    if( confirm ) return 1;
  }

  if( settings_current.zxcf_pri_file ) {
    confirm = zxcf_eject(); if( confirm ) return 1;
  }

  if( settings_current.divide_master_file ) {
    confirm = divide_eject( LIBSPECTRUM_IDE_MASTER );
    if( confirm ) return 1;
  }

  if( settings_current.divide_slave_file ) {
    confirm = divide_eject( LIBSPECTRUM_IDE_SLAVE );
    if( confirm ) return 1;
  }

  return 0;
}
