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

#include <stdio.h>
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
rzx_write_input( libspectrum_rzx *rzx, libspectrum_byte **buffer,
		 libspectrum_byte **ptr, size_t *length );
static libspectrum_error
rzx_write_frames( libspectrum_rzx *rzx, libspectrum_byte **buffer,
		  libspectrum_byte **ptr, size_t *length );

/* The signature used to identify .rzx files */
const libspectrum_byte *signature = "RZX2";

libspectrum_error
libspectrum_rzx_frame( libspectrum_rzx *rzx, size_t instructions,
		       size_t count, libspectrum_byte *in_bytes )
{
  libspectrum_rzx_frame_t *frame;

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

  frame = &rzx->frames[ rzx->count ];

  frame->instructions = instructions;
  frame->count        = count;

  frame->in_bytes = (libspectrum_byte*)
    malloc( count * sizeof( libspectrum_byte ) );
  if( frame->in_bytes == NULL ) return LIBSPECTRUM_ERROR_MEMORY;

  memcpy( frame->in_bytes, in_bytes, count * sizeof( libspectrum_byte ) );

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

  /* Frame size is undefined, so just skip it */
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

  /* Fetch the T-state counter */
  rzx->tstates = (*ptr)[0]             +
                 (*ptr)[1] *     0x100 +
                 (*ptr)[2] *   0x10000 +
                 (*ptr)[3] * 0x1000000 ;
  (*ptr) += 4;

  /* Skip flags */
  (*ptr) += 4;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_read_frames( libspectrum_rzx *rzx,
		 const libspectrum_byte **ptr, const libspectrum_byte *end )
{
  size_t i,j;

  /* And read it in */
  for( i=0; i < rzx->count; i++ ) {

    /* Check the two length bytes exist */
    if( end - (*ptr) < 4 ) {
      libspectrum_print_error(
	"rzx_read_frames: not enough data in buffer\n"
      );
      for( j=0; j<i; j++ ) {
	free( rzx->frames[j].in_bytes );
      }
      free( rzx->frames );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    rzx->frames[i].instructions = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
    rzx->frames[i].count        = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;

    if( (*ptr) - end < rzx->frames[i].count ) {
      libspectrum_print_error(
	"rzx_read_frames: not enough data in buffer\n"
      );
      for( j=0; j<i; j++ ) {
	free( rzx->frames[j].in_bytes );
      }
      free( rzx->frames );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    rzx->frames[i].in_bytes =
      (libspectrum_byte*)malloc( rzx->frames[i].count *
				 sizeof( libspectrum_byte ) );
    if( rzx->frames[i].in_bytes == NULL ) {
      libspectrum_print_error(
	"rzx_read_frames: out of memory\n"
      );
      for( j=0; j<i; j++ ) {
	free( rzx->frames[j].in_bytes );
      }
      free( rzx->frames );
      return LIBSPECTRUM_ERROR_MEMORY;
    }

    memcpy( rzx->frames[i].in_bytes, *ptr, rzx->frames[i].count );
    (*ptr) += rzx->frames[i].count;
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

  error = rzx_write_input( rzx, buffer, &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  
  error = rzx_write_frames( rzx, buffer, &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  
  /* *length is the allocated size; we want to return how much is used */
  *length = ptr - *buffer;

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
rzx_write_input( libspectrum_rzx *rzx, libspectrum_byte **buffer,
		 libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error;
  size_t i, size;

  error = libspectrum_make_room( buffer, 18, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    libspectrum_print_error(
      "rzx_write_input: out of memory\n"
    );
    return error;
  }

  /* Block ID */
  *(*ptr)++ = 0x80;

  /* The length bytes: 18 for this block, plus 4 per frame, plus the number
     of bytes in every frame */
  size = 18 + 4 * rzx->count;
  for( i=0; i<rzx->count; i++ ) size += rzx->frames[i].count;
  libspectrum_write_dword( ptr, size );

  /* Each frame has an undefined length, so write a zero */
  *(*ptr)++ = 0;

  /* How many frames? */
  libspectrum_write_dword( ptr, rzx->count );

  /* T-state counter. Zero for now */
  libspectrum_write_dword( ptr, rzx->tstates );

  /* Flags. Also zero */
  libspectrum_write_dword( ptr, 0 );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rzx_write_frames( libspectrum_rzx *rzx, libspectrum_byte **buffer,
		  libspectrum_byte **ptr, size_t *length )
{
  libspectrum_error error; size_t i;
  libspectrum_rzx_frame_t *frame;

  for( i=0; i<rzx->count; i++ ) {

    frame = &rzx->frames[i];

    error = libspectrum_make_room( buffer, 4 + frame->count, ptr, length );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      libspectrum_print_error(
        "rzx_write_frames: out of memory\n"
      );
      return error;
    }

    libspectrum_write_word( ptr, frame->instructions );
    libspectrum_write_word( ptr, frame->count );
    memcpy( *ptr, frame->in_bytes, frame->count ); (*ptr) += frame->count;
  }

  return LIBSPECTRUM_ERROR_NONE;
}
