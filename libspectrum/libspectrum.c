/* libspectrum.c: Some general routines
   Copyright (c) 2001 Philip Kendall

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

#include <stdlib.h>
#include <string.h>

#include "libspectrum.h"

/* Initialise a libspectrum_snap structure (constructor!) */
int libspectrum_snap_initalise( libspectrum_snap *snap )
{
  int i;

  for( i=0; i<8; i++ ) snap->pages[i]=NULL;

  return LIBSPECTRUM_ERROR_NONE;
}

/* Free all memory used by a libspectrum_snap structure (destructor...) */
int libspectrum_snap_destroy( libspectrum_snap *snap )
{
  int i;

  for( i=0; i<8; i++ ) if( snap->pages[i] ) free( snap->pages[i] );

  return LIBSPECTRUM_ERROR_NONE;
}

char* libspectrum_error_message( libspectrum_error error )
{
  switch( error ) {
  case LIBSPECTRUM_ERROR_NONE:
    return "no error";
  case LIBSPECTRUM_ERROR_MEMORY:
    return "out of memory";
  case LIBSPECTRUM_ERROR_UNKNOWN:
    return "unknown data";
  case LIBSPECTRUM_ERROR_CORRUPT:
    return "corrupt data";
  case LIBSPECTRUM_ERROR_LOGIC:
    return "internal logic error";
  default:
    return "unknown error";
  }
}

/* Given a 48K memory dump `data', place it into the
   appropriate bits of `snap' for a 48K machine */
int libspectrum_split_to_48k_pages( libspectrum_snap *snap,
				    const uchar* data )
{
  /* If any of the three pages are already occupied, barf */
  if( snap->pages[5] || snap->pages[2] || snap->pages[0] )
    return LIBSPECTRUM_ERROR_LOGIC;

  /* Allocate memory for the three pages */
  snap->pages[5] = (uchar*)malloc( 0x4000 * sizeof(uchar) );
  if( snap->pages[5] == NULL ) {
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  snap->pages[2] = (uchar*)malloc( 0x4000 * sizeof(uchar) );
  if( snap->pages[2] == NULL ) {
    free( snap->pages[5] ); snap->pages[5] = NULL;
    return LIBSPECTRUM_ERROR_MEMORY;
  }
    
  snap->pages[0] = (uchar*)malloc( 0x4000 * sizeof(uchar) );
  if( snap->pages[0] == NULL ) {
    free( snap->pages[5] ); snap->pages[5] = NULL;
    free( snap->pages[2] ); snap->pages[2] = NULL;
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* Finally, do the copies... */
  memcpy( snap->pages[5], &data[0x0000], 0x4000 );
  memcpy( snap->pages[2], &data[0x4000], 0x4000 );
  memcpy( snap->pages[0], &data[0x8000], 0x4000 );

  return LIBSPECTRUM_ERROR_NONE;
    
}

/* Ensure there is room for `requested' characters after the current
   position `ptr' in `buffer'. If not, realloc() and update the
   pointers as necessary */
int libspectrum_make_room( uchar **dest, size_t requested, uchar **ptr,
			   size_t *allocated )
{
  size_t current_length;

  current_length = *ptr - *dest;

  if( *allocated == 0 ) {

    (*allocated) = requested;

    *dest = (uchar*)malloc( requested * sizeof(uchar) );
    if( *dest == NULL ) return 1;

  } else {

    /* If there's already enough room here, just return */
    if( current_length + requested <= (*allocated) ) return 0;

    /* Make the new size the maximum of the new needed size and the
     old allocated size * 2 */
    (*allocated) =
      current_length + requested > 2 * (*allocated) ?
      current_length + requested :
      2 * (*allocated);

    *dest = (uchar*)realloc( *dest, (*allocated) * sizeof(uchar) );
    if( *dest == NULL ) return 1;

  }

  /* Update the secondary pointer to the block */
  *ptr = *dest + current_length;

  return 0;

}

/* Write an (LSB) word to buffer */
int libspectrum_write_word( uchar *buffer, libspectrum_word w )
{
  (*buffer++) = w & 0xff;
  (*buffer  ) = w >> 8;
  return LIBSPECTRUM_ERROR_NONE;
}

  
