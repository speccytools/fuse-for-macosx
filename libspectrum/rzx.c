/* rzx.c: routines for dealing with .rzx files
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

#include "rzx.h"

static libspectrum_error
rzx_write_header( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length );
static libspectrum_error
rzx_write_input( size_t frames, libspectrum_byte **buffer,
		 libspectrum_byte **ptr, size_t *length );

/* The signature used to identify .rzx files */
const libspectrum_byte *signature = "RZX!";

libspectrum_error
libspectrum_rzx_frame( libspectrum_rzx *rzx, size_t instructions,
		       libspectrum_byte *keyboard )
{
  int i;

  /* Get more space if we need it; allocate twice as much as we currently
     have, with a minimum of 50 */
  if( rzx->count == rzx->allocated ) {

    libspectrum_rzx_frame_t *ptr; size_t new_allocated;

    new_allocated = rzx_frame_allocated >= 50 ? 2 * rzx_frame_allocated : 50;
    ptr =
      (libspectrum_rzx_frame_t*)realloc( rzx_frames,
					 new_allocated * sizeof(rzx_frame_t)
				       );
    if( ptr == NULL ) return LIBSPECTRUM_ERROR_MEMORY;

    rzx->frames = ptr;
    rzx->allocated = new_allocated;
  }

  rzx->frames[ rzx->count ].instructions = instructions;
  memcpy( rzx->frames[ rzx->count ].keyboard, keyboard,
	  8 * sizeof( libspectrum_byte ) );

  /* Move along to the next frame */
  rzx->count++;

  return 0;
}

libspectrum_error
libspectrum_rzx_free( libspectrum_rzx *rzx )
{
  free( rzx->frames );
  rzx->count = rzx->allocated = 0;
  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_rzx_write( libspectrum_rzx *rzx,
		       libspectrum_byte **buffer, size_t *length )
{
  libspectrum_error error;
  libspectrum_byte *ptr = *buffer;

  error = rzx_write_header( buffer, &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = rzx_write_input( rzx->frames, buffer, &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_write_header( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length )
{
  libspectrum_error error;

  error = libspectrum_make_room( buffer, strlen(signature) + 6, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  strcpy( buffer, signature ); ptr += strlen( signature );
  *(*ptr)++ = 0x00;		/* Minor version number */
  *(*ptr)++ = 0x01;		/* Major version number */

  /* 'Reserved' flags */
  *(*ptr)++ = '\0'; *(*ptr)++ = '\0'; *(*ptr)++ = '\0'; *(*ptr)++ = '\0';

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_write_input( size_t frames, libspectrum_byte **buffer,
		 libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;

  error = libspectrum_make_room( buffer, 18, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* The length bytes: 18 for this block, plus 11 per frame */
  libspectrum_write_dword( ptr, 18 + 11 * frames );

  /* Each frame is 11 bytes long */
  *(*ptr)++ = 11;

  /* How many frames? */
  libspectrum_write_dword( ptr, frames );

  /* T-state counter. Zero for now */
  libspectrum_write_dword( ptr, 0 );

  /* Flags. Also zero */
  libspectrum_write_dword( ptr, 0 );

  return LIBSPECTRUM_ERROR_NONE;
}
