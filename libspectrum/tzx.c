/* tzx.c: Routines for handling .tzx files
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

#include <stdio.h>
#include <string.h>

#include "tape.h"

/* The .tzx file signature (first 8 bytes) */
static const libspectrum_byte *signature = "ZXTape!\x1a";

/* The types of block available */
typedef enum tzx_block_type {
  TZX_ROM = 0x10,
  TZX_TURBO,

  TZX_ARCHIVE_INFO = 0x32,
} tzx_block_type;

/*** Local function prototypes ***/

static libspectrum_error
tzx_read_rom_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		    const libspectrum_byte *end );
static libspectrum_error
tzx_read_turbo_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		      const libspectrum_byte *end );

libspectrum_error
libspectrum_tzx_create( libspectrum_tape *tape, const libspectrum_byte *buffer,
			const size_t length )
{

  libspectrum_error error;

  const libspectrum_byte *ptr, *end;

  ptr = buffer; end = buffer + length;

  /* Must be at least as many bytes as the signature, and the major/minor
     version numbers */
  if( length < strlen(signature) + 2 ) return LIBSPECTRUM_ERROR_CORRUPT;

  /* Now check the signature */
  if( strncmp( ptr, signature, strlen( signature ) ) )
    return LIBSPECTRUM_ERROR_CORRUPT;
  ptr += strlen( signature );
  
  /* Just skip the version numbers */
  ptr += 2;

  while( ptr < end ) {

    /* Get the ID of the next block */
    tzx_block_type id = *ptr++;

    fprintf( stderr, "Block type 0x%02x\n", id );

    switch( id ) {
    case TZX_ROM:
      error = tzx_read_rom_block( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;
    case TZX_TURBO:
      error = tzx_read_turbo_block( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;
    case TZX_ARCHIVE_INFO:
      error = tzx_read_archive_info( tape, &ptr, end );
      if( error ) { libspectrum_tape_free( tape ); return error; }
      break;
    default:	/* For now, don't handle anything else */
      libspectrum_tape_free( tape );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }
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

static libspectrum_error
tzx_read_rom_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		    const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  libspectrum_tape_rom_block *rom_block;

  /* Check there's enough left in the buffer for the pause and the
     data length */
  if( end - (*ptr) < 4 ) return LIBSPECTRUM_ERROR_CORRUPT;

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) return LIBSPECTRUM_ERROR_MEMORY;

  /* This is a standard ROM loader */
  block->type = LIBSPECTRUM_TAPE_BLOCK_ROM;
  rom_block = &(block->types.rom);

  /* Get the pause length and data length */
  rom_block->pause  = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  rom_block->length = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;

  /* Have we got enough bytes left in buffer? */
  if( ( end - (*ptr) ) < rom_block->length ) {
    free( block );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Allocate memory for the data */
  rom_block->data = (libspectrum_byte*)malloc( rom_block->length *
					       sizeof( libspectrum_byte ) );
  if( rom_block->data == NULL ) {
    free( block );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* Copy the block data across, and move along */
  memcpy( rom_block->data, (*ptr), rom_block->length );
  (*ptr) += rom_block->length;

  /* Finally, put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  /* And return with no error */
  return LIBSPECTRUM_ERROR_NONE;

}

static libspectrum_error
tzx_read_turbo_block( libspectrum_tape *tape, const libspectrum_byte **ptr,
		      const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  libspectrum_tape_turbo_block *turbo_block;

  /* Check there's enough left in the buffer for all the metadata */
  if( end - (*ptr) < 18 ) return LIBSPECTRUM_ERROR_CORRUPT;

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) return LIBSPECTRUM_ERROR_MEMORY;

  /* This is a turbo loader */
  block->type = LIBSPECTRUM_TAPE_BLOCK_TURBO;
  turbo_block = &(block->types.turbo);

  /* Get the metadata */
  turbo_block->pilot_length = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  turbo_block->sync1_length = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  turbo_block->sync2_length = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  turbo_block->bit0_length  = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  turbo_block->bit1_length  = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  turbo_block->pilot_pulses = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;
  turbo_block->bits_in_last_byte = **ptr; (*ptr)++;
  turbo_block->pause        = (*ptr)[0] + (*ptr)[1] * 0x100; (*ptr) += 2;

  turbo_block->length = (*ptr)[0] + (*ptr)[1] * 0x100 + (*ptr)[2] * 0x10000;
  (*ptr) += 3;

  /* Have we got enough bytes left in buffer? */
  if( ( end - (*ptr) ) < turbo_block->length ) {
    free( block );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  /* Allocate memory for the data */
  turbo_block->data = (libspectrum_byte*)malloc( turbo_block->length *
						 sizeof( libspectrum_byte ) );
  if( turbo_block->data == NULL ) {
    free( block );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  /* Copy the block data across, and move along */
  memcpy( turbo_block->data, (*ptr), turbo_block->length );
  (*ptr) += turbo_block->length;

  /* Finally, put the block into the block list */
  tape->blocks = g_slist_append( tape->blocks, (gpointer)block );

  /* And return with no error */
  return LIBSPECTRUM_ERROR_NONE;

}

static libspectrum_error
tzx_read_archive_info( libspectrum_tape *tape, const libspectrum_byte **ptr,
		       const libspectrum_byte *end )
{
  libspectrum_tape_block* block;
  libspectrum_tape_archive_info_block *info_block;

  /* Check there's enough left in the buffer for the length */
  if( end - (*ptr) < 2 ) return LIBSPECTRUM_ERROR_CORRUPT;

  /* Get memory for a new block */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) return LIBSPECTRUM_ERROR_MEMORY;

  /* This is an archive info loader */
  block->type = LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO;
  info_block = &(block->types.archive_info);

  /* Skip the length, as I don't care about it */
  (*ptr) += 2;

  /* Get the number of string */
  info_block->count = **ptr; (*ptr)++;

  /* Allocate memory */
  info_block->ids = (int*)malloc( info_block->count * sizeof( int ) );
  if( info_block->ids == NULL ) {
    free( block );
    return LIBSPECTRUM_ERROR_MEMORY;
  }
  info_block->strings = (char**)malloc( info_block->count * sizeof( char* ) );
  if( info_block->strings == NULL ) {
    free( info_block->ids );
    free( block );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  

  
