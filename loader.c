/* loader.c: loader detection
   Copyright (c) 2006 Philip Kendall

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

#include "loader.h"
#include "settings.h"
#include "spectrum.h"
#include "tape.h"
#include "z80/z80.h"

static int successive_reads = 0;
static libspectrum_signed_dword last_tstates_read = -100000;
static libspectrum_byte last_b_read = 0x00;

void
loader_frame( libspectrum_dword frame_length )
{
  if( last_tstates_read > -100000 ) {
    last_tstates_read -= frame_length;
  }
}

void
loader_tape_play( void )
{
  successive_reads = 0;
}

void
loader_tape_stop( void )
{
  successive_reads = 0;
}

void
loader_detect_loader( void )
{
  libspectrum_dword tstates_diff = tstates - last_tstates_read;
  libspectrum_byte b_diff = z80.bc.b.h - last_b_read;

  last_tstates_read = tstates;
  last_b_read = z80.bc.b.h;

  if( settings_current.detect_loader ) {

    if( tape_is_playing() ) {
      if( tstates_diff > 1000 || ( b_diff != 1 && b_diff != 0xff ) ) {
	successive_reads++;
	if( successive_reads >= 2 ) {
	  tape_stop();
	}
      } else {
	successive_reads = 0;
      }
    } else {
      if( tstates_diff <= 500 && ( b_diff == 1 || b_diff == 0xff ) ) {
	successive_reads++;
	if( successive_reads >= 10 ) {
	  tape_do_play( 1 );
	}
      } else {
	successive_reads = 0;
      }
    }

  } else {

    successive_reads = 0;

  }

}
