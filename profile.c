/* profile.c: Z80 profiler
   Copyright (c) 2005 Philip Kendall
		 
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

#include <stdlib.h>

#include <libspectrum.h>

#include "ui/ui.h"

int profile_active = 0;

static char executed[ 0x10000 ];

void
profile_start( void )
{
  memset( executed, 0, sizeof( executed ) );

  profile_active = 1;
}

void
profile_map( libspectrum_word pc )
{
  executed[ pc ] = 1;
}

void
profile_finish( const char *filename )
{
  FILE *f;
  char data[ 0x2000 ];
  size_t i, j, bytes;

  f = fopen( filename, "wb" );
  if( !f ) {
    ui_error( UI_ERROR_ERROR, "unable to open profile map '%s' for writing",
	      filename );
    return;
  }

  for( i = 0; i < 8192; i++ ) {

    libspectrum_byte b = 0;

    for( j = 0; j < 8; j++ ) {
      b <<= 1;
      b |= executed[ 8 * i + j ];
    }

    data[ i ] = b;
  }

  bytes = fwrite( data, 1, 0x2000, f );
  if( bytes != 0x2000 ) {
    ui_error( UI_ERROR_ERROR,
	      "could write only %d bytes of %d to profile map '%s'",
	      bytes, 0x2000, filename );
    return;
  }

  profile_active = 0;
}
