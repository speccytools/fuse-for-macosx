/* fuller.c: Routines for handling the Fuller Box
   Copyright (c) 2007-2009 Stuart Brady, Fredrick Meunier

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

   Philip: philip-fuse@shadowmagic.org.uk

   Stuart: sdbrady@ntlworld.com

*/

#include <config.h>

#include <libspectrum.h>

#include "ay.h"
#include "compat.h"
#include "fuller.h"
#include "joystick.h"
#include "machine.h"
#include "memory.h"
#include "module.h"
#include "periph.h"
#include "settings.h"

static void fuller_enabled_snapshot( libspectrum_snap *snap );
static void fuller_from_snapshot( libspectrum_snap *snap );
static void fuller_to_snapshot( libspectrum_snap *snap );

static module_info_t fuller_module_info = {

  NULL,
  NULL,
  fuller_enabled_snapshot,
  fuller_from_snapshot,
  fuller_to_snapshot,

};

const periph_t fuller_peripherals[] = {
  { 0x00ff, 0x003f, ay_registerport_read, ay_registerport_write },
  { 0x00ff, 0x005f, NULL, ay_dataport_write },
  { 0x00ff, 0x007f, joystick_fuller_read, NULL },
};

const size_t fuller_peripherals_count =
  sizeof( fuller_peripherals ) / sizeof( periph_t );

static void
fuller_enabled_snapshot( libspectrum_snap *snap )
{
  if( libspectrum_snap_fuller_box_active( snap ) )
    settings_current.fuller = 1;
}

static void
fuller_from_snapshot( libspectrum_snap *snap )
{
  if( periph_fuller_active ) {
    ay_state_from_snapshot( snap );
  }
}

static void
fuller_to_snapshot( libspectrum_snap *snap )
{
  libspectrum_snap_set_fuller_box_active( snap, periph_fuller_active );
}

int
fuller_init( void )
{
  module_register( &fuller_module_info );

  return 0;
}
