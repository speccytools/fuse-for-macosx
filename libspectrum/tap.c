/* tape.c: Routines for handling .tap files
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

#include <stddef.h>
#include <string.h>

#include "tape.h"

libspectrum_error
libspectrum_tap_create( libspectrum_tape *tape, const libspectrum_byte *buffer,
			const size_t length )
{
  libspectrum_tape_block *block;
  libspectrum_tape_rom_block *rom_block;
  libspectrum_error error;

  const libspectrum_byte *ptr, *end;

  ptr = buffer; end = buffer + length;

  while( ptr < end ) {
    
    /* If we've got less than two bytes for the length, something's
       gone wrong, so gone home */
    if( ( end - ptr ) < 2 ) {
      libspectrum_tape_free( tape );
      libspectrum_print_error(
        "libspectrum_tap_create: not enough data in buffer\n"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    /* Get memory for a new block */
    block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
    if( block == NULL ) {
      libspectrum_print_error( "libspectrum_tap_create: out of memory\n" );
      return LIBSPECTRUM_ERROR_MEMORY;
    }

    /* This is a standard ROM loader */
    block->type = LIBSPECTRUM_TAPE_BLOCK_ROM;
    rom_block = &(block->types.rom);

    /* Get the length, and move along the buffer */
    rom_block->length = ptr[0] + ptr[1] * 0x100; ptr += 2;

    /* Have we got enough bytes left in buffer? */
    if( end - ptr < (ptrdiff_t)rom_block->length ) {
      libspectrum_tape_free( tape );
      free( block );
      libspectrum_print_error(
        "libspectrum_tap_create: not enough data in buffer\n"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    /* Allocate memory for the data */
    rom_block->data = (libspectrum_byte*)malloc( rom_block->length *
						 sizeof( libspectrum_byte ) );
    if( rom_block->data == NULL ) {
      libspectrum_tape_free( tape );
      free( block );
      libspectrum_print_error( "libspectrum_tap_create: out of memory\n" );
      return LIBSPECTRUM_ERROR_MEMORY;
    }

    /* Copy the block data across, and move along */
    memcpy( rom_block->data, ptr, rom_block->length );
    ptr += rom_block->length;

    /* Give a 1s pause after each block */
    rom_block->pause = 1000;

    /* Finally, put the block into the block list */
    tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  }

  /* And we're pointing to the first block */
  tape->current_block = tape->blocks;

  /* Which we should then initialise */
  error = libspectrum_tape_init_block(
            (libspectrum_tape_block*)tape->current_block->data
          );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}
