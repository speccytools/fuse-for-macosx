/* ula.c: ULA routines
   Copyright (c) 1999-2004 Philip Kendall, Darren Salt

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

#include <config.h>

#include <libspectrum.h>

#include "compat.h"
#include "keyboard.h"
#include "loader.h"
#include "machine.h"
#include "settings.h"
#include "sound.h"
#include "spectrum.h"
#include "tape.h"

static libspectrum_byte last_byte;

libspectrum_byte ula_contention[ 80000 ];

libspectrum_byte
ula_read( libspectrum_word port, int *attached )
{
  *attached = 1;

  loader_detect_loader();

  return ( keyboard_read( port >> 8 ) ^ ( tape_microphone ? 0x40 : 0x00 ) );
}

/* What happens when we write to the ULA? */
void
ula_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  last_byte = b;

  display_set_lores_border( b & 0x07 );
  sound_beeper( 0, b & 0x10 );

  if( machine_current->timex ) {
    keyboard_default_value = 0x5f;
    return;
  }

  if( settings_current.issue2 ) {
    keyboard_default_value = b & 0x18 ? 0xff : 0xbf;
  } else {
    keyboard_default_value = b & 0x10 ? 0xff : 0xbf;
  }
}

libspectrum_byte
ula_last_byte( void )
{
  return last_byte;
}

int
ula_from_snapshot( libspectrum_snap *snap )
{
  ula_write( 0x00fe, libspectrum_snap_out_ula( snap ) );
  tstates = libspectrum_snap_tstates( snap );
  settings_current.issue2 = libspectrum_snap_issue2( snap );

  return 0;
}

int
ula_to_snapshot( libspectrum_snap *snap )
{
  libspectrum_snap_set_out_ula( snap, last_byte );
  libspectrum_snap_set_tstates( snap, tstates );
  libspectrum_snap_set_issue2( snap, settings_current.issue2 );

  return 0;
}  

void
ula_contend_port_early( libspectrum_word port )
{
  if( ( port & 0xc000 ) == 0x4000 ) tstates += ula_contention[ tstates ];
   
  tstates++;
}

void
ula_contend_port_late( libspectrum_word port )
{
  if( machine_current->ram.port_contended( port ) ) {

    tstates += ula_contention[ tstates ]; tstates += 2;

  } else {

    if( ( port & 0xc000 ) == 0x4000 ) {
      tstates += ula_contention[ tstates ]; tstates++;
      tstates += ula_contention[ tstates ]; tstates++;
      tstates += ula_contention[ tstates ];
    } else {
      tstates += 2;
    }

  }
}
