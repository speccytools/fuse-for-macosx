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

#include <string.h>

#include "rzx.h"

static libspectrum_error
rzx_read_header( const libspectrum_byte **ptr, const libspectrum_byte *end );
static libspectrum_error
rzx_read_input( libspectrum_rzx *rzx,
		const libspectrum_byte **ptr, const libspectrum_byte *end );
static libspectrum_error
rzx_read_frames( libspectrum_rzx *rzx,
		 const libspectrum_byte **ptr, const libspectrum_byte *end );

static libspectrum_error
rzx_write_header( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length );
static libspectrum_error
rzx_write_input( size_t frames, libspectrum_byte **buffer,
		 libspectrum_byte **ptr, size_t *length );
static libspectrum_error
rzx_write_frames( libspectrum_rzx *rzx, libspectrum_byte **buffer,
		  libspectrum_byte **ptr, size_t *length );

/* The signature used to identify .rzx files */
const libspectrum_byte *signature = "RZX!";

libspectrum_error
libspectrum_rzx_frame( libspectrum_rzx *rzx, size_t instructions,
		       libspectrum_byte *keyboard )
{
  /* Get more space if we need it; allocate twice as much as we currently
     have, with a minimum of 50 */
  if( rzx->count == rzx->allocated ) {

    libspectrum_rzx_frame_t *ptr; size_t new_allocated;

    new_allocated = rzx->allocated >= 25 ? 2 * rzx->allocated : 50;
    ptr =
      (libspectrum_rzx_frame_t*)realloc(
        rzx->frames, new_allocated * sizeof(libspectrum_rzx_frame_t)
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
  free( rzx->frames ); rzx->frames = NULL;
  rzx->count = rzx->allocated = 0;
  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_rzx_read( libspectrum_rzx *rzx, const libspectrum_byte *buffer,
		      const size_t length )
{
  libspectrum_error error;
  const libspectrum_byte *ptr, *end;

  ptr = buffer; end = buffer + length;

  error = rzx_read_header( &ptr, end );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = rzx_read_input( rzx, &ptr, end );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = rzx_read_frames( rzx, &ptr, end );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_read_header( const libspectrum_byte **ptr, const libspectrum_byte *end )
{
  /* Check the header exists */
  if( end - (*ptr) < 10 ) {
    libspectrum_print_error(
      "rzx_read_header: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Check the RZX signature exists */
  if( memcmp( *ptr, signature, strlen( signature ) ) ) {
    libspectrum_print_error(
      "rzx_read_header: RZX signature not found\n"
    );
    return LIBSPECTRUM_ERROR_SIGNATURE;
  }

  (*ptr) += 10;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_read_input( libspectrum_rzx *rzx,
		const libspectrum_byte **ptr, const libspectrum_byte *end )
{
  /* Check we've got enough data for the block */
  if( end - (*ptr) < 18 ) {
    libspectrum_print_error(
      "rzx_read_input: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Check the block ID */
  if( **ptr != 0x80 ) {
    libspectrum_print_error(
      "rzx_read_input: wrong block ID 0x%02x\n", (int)**ptr
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Skip some stuff */
  (*ptr) += 5;

  /* Check the frame size is what we expect */
  if( **ptr != 11 ) {
    libspectrum_print_error(
      "rzx_read_input: unknown frame size %d\n", (int)**ptr
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }
  (*ptr)++;

  /* Get the number of frames */
  rzx->count = (*ptr)[0]             +
               (*ptr)[1] *     0x100 +
               (*ptr)[2] *   0x10000 +
               (*ptr)[3] * 0x1000000 ;
  (*ptr) += 4;

  /* Allocate memory for the frames */
  rzx->frames =
    (libspectrum_rzx_frame_t*)malloc( rzx->count *
				      sizeof( libspectrum_rzx_frame_t) );
  if( rzx->frames == NULL ) {
    libspectrum_print_error(
      "rzx_read_input: out of memory\n"
    );
    return LIBSPECTRUM_ERROR_MEMORY;
  }
  rzx->allocated = rzx->count;

  /* Skip more stuff */
  (*ptr) += 8;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_read_frames( libspectrum_rzx *rzx,
		 const libspectrum_byte **ptr, const libspectrum_byte *end )
{
  size_t i;

  /* Check there's enough data left */
  if( (*ptr) - end < 11 * rzx->count ) {
    libspectrum_print_error(
      "rzx_read_frames: not enough data in buffer\n"
    );
    free( rzx->frames );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* And read it in */
  for( i=0; i < rzx->count; i++ ) {
    rzx->frames[i].instructions = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
    memcpy( rzx->frames[i].keyboard, *ptr, 8 ); (*ptr) += 8;
    (*ptr)++;		/* Skip the Kempston byte */
  }

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

  error = rzx_write_input( rzx->count, buffer, &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  
  error = rzx_write_frames( rzx, buffer, &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_write_header( libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length )
{
  libspectrum_error error;

  error = libspectrum_make_room( buffer, strlen(signature) + 6, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    libspectrum_print_error(
      "rzx_write_header: out of memory\n"
    );
    return error;
  }

  strcpy( *ptr, signature ); (*ptr) += strlen( signature );
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
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    libspectrum_print_error(
      "rzx_write_input: out of memory\n"
    );
    return error;
  }

  /* Block ID */
  *(*ptr)++ = 0x80;

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

static libspectrum_error
rzx_write_frames( libspectrum_rzx *rzx, libspectrum_byte **buffer,
		  libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error; int i,j;

  error = libspectrum_make_room( buffer, 11 * rzx->count, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    libspectrum_print_error(
      "rzx_write_frames: out of memory\n"
    );
    return error;
  }

  for( i=0; i<rzx->count; i++ ) {
    libspectrum_write_word(
      ptr, i == rzx->count ? 0 : rzx->frames[i+1].instructions
    );
    for( j=0; j<8; j++ ) *(*ptr)++ = rzx->frames[i].keyboard[j];
    *(*ptr)++ = 0;		/* Kempston joystick input */
  }

  return LIBSPECTRUM_ERROR_NONE;
}

     
