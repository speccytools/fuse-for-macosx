/* tape.c: Routines for handling tape files
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

#include "tape.h"

/* Local function prototypes */
static libspectrum_error
rom_edge( libspectrum_tape_rom_block *block, libspectrum_dword *tstates,
	  int *end_of_block );
static libspectrum_error
rom_next_bit( libspectrum_tape_rom_block *block );

/* The main function: called with a tape object and returns the number of
   t-states until the next edge (or LIBSPECTRUM_TAPE_END if that was the
   last edge) */
libspectrum_error
libspectrum_tape_get_next_edge( libspectrum_tape *tape,
				libspectrum_dword *tstates )
{
  libspectrum_tape_block *block =
    (libspectrum_tape_block*)tape->current_block->data;

  /* Has this edge ended the block? */
  int end_of_block = 0;

  switch( block->type ) {
  case LIBSPECTRUM_TAPE_BLOCK_ROM:
    return rom_edge( &(block->types.rom), tstates, &end_of_block );
  default:
    tstates = 0;
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  /* If that ended the block, move onto the next block
     FIXME: End of tape! */
  if( end_of_block ) {
    tape->current_block = tape->current_block->next;
  }
}

static libspectrum_error
rom_edge( libspectrum_tape_rom_block *block, libspectrum_dword *tstates,
	  int *end_of_block )
{
  int error;

  switch( block->state ) {

  case LIBSPECTRUM_TAPE_STATE_PILOT:
    /* The next edge occurs in one pilot edge timing */
    *tstates = LIBSPECTRUM_TAPE_TIMING_PILOT;
    /* If that was the last pilot edge, change state */
    if( --(block->edge_count) == 0 )
      block->state = LIBSPECTRUM_TAPE_STATE_SYNC1;
    break;

  case LIBSPECTRUM_TAPE_STATE_SYNC1:
    /* The first short sync pulse */
    *tstates = LIBSPECTRUM_TAPE_TIMING_SYNC1;
    /* Followed by the second sync pulse */
    block->state = LIBSPECTRUM_TAPE_STATE_SYNC2;
    break;

  case LIBSPECTRUM_TAPE_STATE_SYNC2:
    /* The second short sync pulse */
    *tstates = LIBSPECTRUM_TAPE_TIMING_SYNC2;
    /* Followed by the first bit of data */
    error = rom_next_bit( block ); if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_STATE_DATA1:
    /* The first edge for a bit of data */
    *tstates = block->bit_tstates;
    /* Followed by the second edge */
    block->state = LIBSPECTRUM_TAPE_STATE_DATA2;
    break;

  case LIBSPECTRUM_TAPE_STATE_DATA2:
    /* The second edge for a bit of data */
    *tstates = block->bit_tstates;
    /* Followed by the next bit of data (or the end of data) */
    error = rom_next_bit( block ); if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_STATE_PAUSE:
    /* The pause at the end of the block */
    *tstates = block->pause * 69888; /* FIXME: should vary with tstates per
					frame */
    *end_of_block = 1;
    break;

  default:
    return LIBSPECTRUM_ERROR_LOGIC;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rom_next_bit( libspectrum_tape_rom_block *block )
{
  int next_bit;

  /* Have we finished the current byte? */
  if( ++(block->bits_through_byte) == 8 ) {

    /* If so, have we finished the entire block? If so, all we've got
       left after this is the pause at the end */
    if( ++(block->bytes_through_block) == block->length ) {
      block->state = LIBSPECTRUM_TAPE_STATE_PAUSE;
      return LIBSPECTRUM_ERROR_NONE;
    }
    
    /* If we've finished the current byte, but not the entire block,
       get the next byte */
    block->current_byte = block->data[ block->bytes_through_block ];
    block->bits_through_byte = 0;
  }

  /* Get the high bit, and shift the byte out leftwards */
  next_bit = block->current_byte & 0x80;
  block->current_byte <<= 1;

  /* And set the timing and state for another data bit */
  block->bit_tstates = ( next_bit ? LIBSPECTRUM_TAPE_TIMING_DATA1
			          : LIBSPECTRUM_TAPE_TIMING_DATA0 );
  block->state = LIBSPECTRUM_TAPE_STATE_DATA1;

  return LIBSPECTRUM_ERROR_NONE;
}
