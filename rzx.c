/* rzx.c: .rzx files
   Copyright (c) 2002 Philip Kendall

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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>

#include "fuse.h"
#include "keyboard.h"
#include "libspectrum/rzx.h"
#include "rzx.h"
#include "types.h"
#include "utils.h"
#include "z80/z80_macros.h"

/* The count of instruction fetches needed for .rzx files */
size_t rzx_instructions;

/* Are we currently recording a .rzx file? */
int rzx_recording;

libspectrum_rzx rzx;

int rzx_init( void )
{
  rzx_recording = 0;

  return 0;
}

int rzx_start_recording( void )
{
  libspectrum_error error;

  size_t i; libspectrum_byte keyboard[8];

  /* Note that we're recording */
  rzx_recording = 1;

  for( i=0; i<8; i++ )
    keyboard[i] = keyboard_default_value & keyboard_return_values[i];

  error = libspectrum_rzx_frame( &rzx, 0, keyboard );
  if( error ) return error;

  /* Start the count of instruction fetches here */
  rzx_instructions = 0;

  return 0;
}

int rzx_stop_recording( void )
{
  libspectrum_byte *buffer; size_t length;
  libspectrum_error libspec_error; int error;

  /* Stop recording data */
  rzx_recording = 0;

  length = 0;
  libspec_error = libspectrum_rzx_write( &rzx, &buffer, &length );
  if( libspec_error != LIBSPECTRUM_ERROR_NONE ) {
    fprintf(stderr, "%s: error during libspectrum_rzx_write: %s\n",
	    fuse_progname, libspectrum_error_message( libspec_error ) );
    libspectrum_rzx_free( &rzx );
    return libspec_error;
  }

  error = utils_write_file( "test.rzx", buffer, length );
  if( error ) { free( buffer ); libspectrum_rzx_free( &rzx ); return error; }

  free( buffer );

  libspec_error = libspectrum_rzx_free( &rzx );
  if( libspec_error != LIBSPECTRUM_ERROR_NONE ) {
    fprintf(stderr, "%s: error during libspectrum_rzx_free: %s\n",
	    fuse_progname, libspectrum_error_message( libspec_error ) );
    return libspec_error;
  }

  return 0;
}

int rzx_frame( void )
{
  libspectrum_error error;

  size_t i; libspectrum_byte keyboard[8];

  if( ! rzx_recording ) return 0;

  for( i=0; i<8; i++ )
    keyboard[i] = keyboard_default_value & keyboard_return_values[i];

  error = libspectrum_rzx_frame( &rzx, rzx_instructions, keyboard );
  if( error ) return error;

  /* Reset the instruction counter */
  rzx_instructions = 0;

  return 0;
}
