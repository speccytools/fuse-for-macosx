/* rzx.h: routines for dealing with .rzx files
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

#ifndef LIBSPECTRUM_RZX_H
#define LIBSPECTRUM_RZX_H

#ifndef LIBSPECTRUM_LIBSPECTRUM_H
#include "libspectrum.h"
#endif			/* #ifndef LIBSPECTRUM_LIBSPECTRUM_H */

typedef enum libspectrum_rzx_block_t {

  LIBSPECTRUM_RZX_CREATOR_BLOCK = 0x10,
  LIBSPECTRUM_RZX_SNAPSHOT_BLOCK = 0x30,
  LIBSPECTRUM_RZX_INPUT_BLOCK = 0x80,

} libspectrum_rzx_block_t;

typedef struct libspectrum_rzx_frame_t {

  size_t instructions;

  size_t count;
  libspectrum_byte* in_bytes;

  int repeat_last;			/* Set if we should use the last
					   frame's value */

} libspectrum_rzx_frame_t;

typedef struct libspectrum_rzx {

  libspectrum_rzx_frame_t *frames;
  size_t count;
  size_t allocated;

  size_t tstates;

} libspectrum_rzx;

libspectrum_error libspectrum_rzx_frame( libspectrum_rzx *rzx,
					 size_t instructions, 
					 size_t count,
					 libspectrum_byte *in_bytes );

libspectrum_error libspectrum_rzx_free( libspectrum_rzx *rzx );

libspectrum_error
libspectrum_rzx_read( libspectrum_rzx *rzx, const libspectrum_byte *buffer,
		      const size_t length, libspectrum_snap **snap );

libspectrum_error
libspectrum_rzx_write( libspectrum_rzx *rzx,
		       libspectrum_byte **buffer, size_t *length,
		       libspectrum_byte *snap, size_t snap_length,
		       const char *program, libspectrum_word major,
		       libspectrum_word minor, int compress );

#endif			/* #ifndef LIBSPECTRUM_RZX_H */
