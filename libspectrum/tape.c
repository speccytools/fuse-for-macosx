/* tape.c: Routines for handling tape files
   Copyright (c) 2001, 2002 Philip Kendall, Darren Salt

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

/*** Local function prototypes ***/

/* Free the memory used by a block */
static void
block_free( gpointer data, gpointer user_data );

/* Functions to initialise block types */

static libspectrum_error
rom_init( libspectrum_tape_rom_block *block );
static libspectrum_error
turbo_init( libspectrum_tape_turbo_block *block );
static libspectrum_error
pure_data_init( libspectrum_tape_pure_data_block *block );
static libspectrum_error
raw_data_init( libspectrum_tape_raw_data_block *block );

/* Functions to get the next edge */

static libspectrum_error
rom_edge( libspectrum_tape_rom_block *block, libspectrum_dword *tstates,
	  int *end_of_block );
static libspectrum_error
rom_next_bit( libspectrum_tape_rom_block *block );

static libspectrum_error
turbo_edge( libspectrum_tape_turbo_block *block, libspectrum_dword *tstates,
	    int *end_of_block );
static libspectrum_error
turbo_next_bit( libspectrum_tape_turbo_block *block );

static libspectrum_error
tone_edge( libspectrum_tape_pure_tone_block *block, libspectrum_dword *tstates,
	   int *end_of_block );

static libspectrum_error
pulses_edge( libspectrum_tape_pulses_block *block, libspectrum_dword *tstates,
	     int *end_of_block );

static libspectrum_error
pure_data_edge( libspectrum_tape_pure_data_block *block,
		libspectrum_dword *tstates, int *end_of_block );
static libspectrum_error
pure_data_next_bit( libspectrum_tape_pure_data_block *block );

static libspectrum_error
raw_data_edge( libspectrum_tape_raw_data_block *block,
	       libspectrum_dword *tstates, int *end_of_block );
static libspectrum_error
raw_data_next_bit( libspectrum_tape_raw_data_block *block );

static libspectrum_error
jump_blocks( libspectrum_tape *tape, int offset );

/*** Function definitions ****/

/* Free up a list of blocks */
libspectrum_error
libspectrum_tape_free( libspectrum_tape *tape )
{
  g_slist_foreach( tape->blocks, block_free, NULL );
  g_slist_free( tape->blocks );
  tape->blocks = tape->current_block = NULL;
  
  return LIBSPECTRUM_ERROR_NONE;
}

/* Free the memory used by one block */
static void
block_free( gpointer data, gpointer user_data GCC_UNUSED )
{
  libspectrum_tape_block *block = (libspectrum_tape_block*)data;
  size_t i;

  switch( block->type ) {

  case LIBSPECTRUM_TAPE_BLOCK_ROM:
    free( block->types.rom.data );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_TURBO:
    free( block->types.turbo.data );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PULSES:
    free( block->types.pulses.lengths );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
    free( block->types.pure_data.data );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    free( block->types.raw_data.data );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
    break;
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
    free( block->types.group_start.name );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
    break;
  case LIBSPECTRUM_TAPE_BLOCK_JUMP:
    break;
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
    break;
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
    break;

  case LIBSPECTRUM_TAPE_BLOCK_SELECT:
    for( i=0; i<block->types.select.count; i++ ) {
      free( block->types.select.descriptions[i] );
    }
    free( block->types.select.descriptions );
    free( block->types.select.offsets );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_STOP48:
    break;

  case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
    free( block->types.comment.text );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
    free( block->types.message.text );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
    for( i=0; i<block->types.archive_info.count; i++ ) {
      free( block->types.archive_info.strings[i] );
    }
    free( block->types.archive_info.ids );
    free( block->types.archive_info.strings );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
    free( block->types.hardware.types  );
    free( block->types.hardware.ids    );
    free( block->types.hardware.values );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
    free( block->types.custom.data );
    break;

  /* This should never actually occur */
  case LIBSPECTRUM_TAPE_BLOCK_CONCAT:
    break;
  }
}

/* Called when a new block is started to initialise its internal state */
libspectrum_error
libspectrum_tape_init_block( libspectrum_tape_block *block )
{
  switch( block->type ) {

  case LIBSPECTRUM_TAPE_BLOCK_ROM:
    return rom_init( &(block->types.rom) );
  case LIBSPECTRUM_TAPE_BLOCK_TURBO:
    return turbo_init( &(block->types.turbo) );
  case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    block->types.pure_tone.edge_count = block->types.pure_tone.pulses;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PULSES:
    block->types.pulses.edge_count = 0;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
    return pure_data_init( &(block->types.pure_data) );
  case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    return raw_data_init( &(block->types.raw_data) );

  /* These blocks need no initialisation */
  case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
  case LIBSPECTRUM_TAPE_BLOCK_JUMP:
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
  case LIBSPECTRUM_TAPE_BLOCK_SELECT:
  case LIBSPECTRUM_TAPE_BLOCK_STOP48:
  case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
  case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
  case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
  case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
  case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
    return LIBSPECTRUM_ERROR_NONE;

  default:
    libspectrum_print_error(
      "libspectrum_tape_init_block: unknown block type 0x%02x\n",
      block->type
    );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
rom_init( libspectrum_tape_rom_block *block )
{
  /* Initialise the number of pilot pulses */
  block->edge_count = block->data[0] & 0x80         ?
                      LIBSPECTRUM_TAPE_PILOTS_DATA  :
                      LIBSPECTRUM_TAPE_PILOTS_HEADER;

  /* And that we're just before the start of the data */
  block->bytes_through_block = -1; block->bits_through_byte = 7;
  block->state = LIBSPECTRUM_TAPE_STATE_PILOT;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
turbo_init( libspectrum_tape_turbo_block *block )
{
  /* Initialise the number of pilot pulses */
  block->edge_count = block->pilot_pulses;

  /* And that we're just before the start of the data */
  block->bytes_through_block = -1; block->bits_through_byte = 7;
  block->state = LIBSPECTRUM_TAPE_STATE_PILOT;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
pure_data_init( libspectrum_tape_pure_data_block *block )
{
  libspectrum_error error;

  /* We're just before the start of the data */
  block->bytes_through_block = -1; block->bits_through_byte = 7;
  /* Set up the next bit */
  error = pure_data_next_bit( block ); if( error ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
raw_data_init( libspectrum_tape_raw_data_block *block )
{
  libspectrum_error error;

  /* We're just before the start of the data */
  block->state = LIBSPECTRUM_TAPE_STATE_DATA1;
  block->bytes_through_block = -1; block->bits_through_byte = 7;
  block->last_bit = 0x80 & block->data[0];
  /* Set up the next bit */
  error = raw_data_next_bit( block ); if( error ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

/* Some flags which may be set after calling libspectrum_tape_get_next_edge */
const int LIBSPECTRUM_TAPE_FLAGS_BLOCK  = 1 << 0; /* End of block */
const int LIBSPECTRUM_TAPE_FLAGS_STOP   = 1 << 1; /* Stop tape */
const int LIBSPECTRUM_TAPE_FLAGS_STOP48 = 1 << 2; /* Stop tape if in
						     48K mode */

/* The main function: called with a tape object and returns the number of
   t-states until the next edge, and a marker if this was the last edge
   on the tape */
libspectrum_error
libspectrum_tape_get_next_edge( libspectrum_tape *tape,
				libspectrum_dword *tstates, int *flags )
{
  int error;

  libspectrum_tape_block *block =
    (libspectrum_tape_block*)tape->current_block->data;

  /* Has this edge ended the block? */
  int end_of_block = 0;

  /* After getting a new block, do we want to advance to the next one? */
  int no_advance = 0;

  /* Assume no special flags by default */
  *flags = 0;

  switch( block->type ) {
  case LIBSPECTRUM_TAPE_BLOCK_ROM:
    error = rom_edge( &(block->types.rom), tstates, &end_of_block );
    if( error ) return error;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_TURBO:
    error = turbo_edge( &(block->types.turbo), tstates, &end_of_block );
    if( error ) return error;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    error = tone_edge( &(block->types.pure_tone), tstates, &end_of_block );
    if( error ) return error;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PULSES:
    error = pulses_edge( &(block->types.pulses), tstates, &end_of_block );
    if( error ) return error;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
    error = pure_data_edge( &(block->types.pure_data), tstates, &end_of_block);
    if( error ) return error;
    break;
  case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    error = raw_data_edge( &(block->types.raw_data), tstates, &end_of_block );
    if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
    *tstates = ( block->types.pause.length * 69888 ) / 20; end_of_block = 1;
    /* 0 ms pause => stop tape */
    if( *tstates == 0 ) { *flags |= LIBSPECTRUM_TAPE_FLAGS_STOP; }
    break;

  case LIBSPECTRUM_TAPE_BLOCK_JUMP:
    error = jump_blocks( tape, block->types.jump.offset );
    if( error ) return error;
    *tstates = 0; end_of_block = 1; no_advance = 1;
    break;

  case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
    tape->loop_block = tape->current_block->next;
    tape->loop_count = block->types.loop_start.count;
    *tstates = 0; end_of_block = 1;
    break;

  case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
    if( --tape->loop_count ) {
      tape->current_block = tape->loop_block;
      no_advance = 1;
    }
    *tstates = 0; end_of_block = 1;
    break;

  case LIBSPECTRUM_TAPE_BLOCK_STOP48:
    *tstates = 0; *flags |= LIBSPECTRUM_TAPE_FLAGS_STOP48; end_of_block = 1;
    break;

  /* For blocks which contain no Spectrum-readable data, return zero
     tstates and set end of block set so we instantly get the next block */
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_START: 
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
  case LIBSPECTRUM_TAPE_BLOCK_SELECT:
  case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
  case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
  case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
  case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
  case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
    *tstates = 0; end_of_block = 1;
    break;

  default:
    *tstates = 0;
    libspectrum_print_error(
      "libspectrum_tape_get_next_edge: unknown block type 0x%02x\n",
      block->type
    );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  /* If that ended the block, move onto the next block */
  if( end_of_block ) {

    *flags |= LIBSPECTRUM_TAPE_FLAGS_BLOCK;

    /* Advance to the next block, unless we've been told not to */
    if( !no_advance ) {

      tape->current_block = tape->current_block->next;

      /* If we've just hit the end of the tape, stop the tape (and
	 then `rewind' to the start) */
      if( tape->current_block == NULL ) {
	*flags |= LIBSPECTRUM_TAPE_FLAGS_STOP;
	tape->current_block = tape->blocks;
      }
    }

    /* Initialise the new block */
    block = (libspectrum_tape_block*)tape->current_block->data;
    error = libspectrum_tape_init_block( block );
    if( error ) return error;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

/* The timings for the standard ROM loader */
const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_PILOT = 2168; /* Pilot */
const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_SYNC1 =  667; /* Sync 1 */
const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_SYNC2 =  735; /* Sync 2 */
const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_DATA0 =  855; /* Reset */
const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_DATA1 = 1710; /* Set */

/* The number of pilot pulses for the standard ROM loader
   NB: These disagree with the .tzx specification (they're one less), but
       are correct. Entering the loop at #04D8 in the 48K ROM with HL == #0001
       will produce the first sync pulse, not a pilot pulse.
*/
const size_t LIBSPECTRUM_TAPE_PILOTS_HEADER = 0x1f7f;
const size_t LIBSPECTRUM_TAPE_PILOTS_DATA   = 0x0c97;

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
    *tstates = (block->pause * 69888)/20; /* FIXME: should vary with tstates
					     per frame */
    *end_of_block = 1;
    break;

  default:
    libspectrum_print_error( "rom_edge: unknown state %d\n", block->state );
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

static libspectrum_error
turbo_edge( libspectrum_tape_turbo_block *block, libspectrum_dword *tstates,
	    int *end_of_block )
{
  int error;

  switch( block->state ) {

  case LIBSPECTRUM_TAPE_STATE_PILOT:
    /* The next edge occurs in one pilot edge timing */
    *tstates = block->pilot_length;
    /* If that was the last pilot edge, change state */
    if( --(block->edge_count) == 0 )
      block->state = LIBSPECTRUM_TAPE_STATE_SYNC1;
    break;

  case LIBSPECTRUM_TAPE_STATE_SYNC1:
    /* The first short sync pulse */
    *tstates = block->sync1_length;
    /* Followed by the second sync pulse */
    block->state = LIBSPECTRUM_TAPE_STATE_SYNC2;
    break;

  case LIBSPECTRUM_TAPE_STATE_SYNC2:
    /* The second short sync pulse */
    *tstates = block->sync2_length;
    /* Followed by the first bit of data */
    error = turbo_next_bit( block ); if( error ) return error;
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
    error = turbo_next_bit( block ); if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_STATE_PAUSE:
    /* The pause at the end of the block */
    *tstates = (block->pause * 69888)/20; /* FIXME: should vary with tstates
					     per frame */
    *end_of_block = 1;
    break;

  default:
    libspectrum_print_error( "turbo_edge: unknown state %d\n", block->state );
    return LIBSPECTRUM_ERROR_LOGIC;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
turbo_next_bit( libspectrum_tape_turbo_block *block )
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

    /* If we're looking at the last byte, take account of the fact it
       may have less than 8 bits in it */
    if( block->bytes_through_block == block->length-1 ) {
      block->bits_through_byte = 8 - block->bits_in_last_byte;
    } else {
      block->bits_through_byte = 0;
    }
  }

  /* Get the high bit, and shift the byte out leftwards */
  next_bit = block->current_byte & 0x80;
  block->current_byte <<= 1;

  /* And set the timing and state for another data bit */
  block->bit_tstates = ( next_bit ? block->bit1_length : block->bit0_length );
  block->state = LIBSPECTRUM_TAPE_STATE_DATA1;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tone_edge( libspectrum_tape_pure_tone_block *block, libspectrum_dword *tstates,
	   int *end_of_block )
{
  /* The next edge occurs in one pilot edge timing */
  *tstates = block->length;
  /* If that was the last edge, the block is finished */
  if( --(block->edge_count) == 0 ) (*end_of_block) = 1;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
pulses_edge( libspectrum_tape_pulses_block *block, libspectrum_dword *tstates,
	     int *end_of_block )
{
  /* Get the length of this edge */
  *tstates = block->lengths[ block->edge_count ];
  /* Was that the last edge? */
  if( ++(block->edge_count) == block->count ) (*end_of_block) = 1;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
pure_data_edge( libspectrum_tape_pure_data_block *block,
		libspectrum_dword *tstates, int *end_of_block )
{
  int error;

  switch( block->state ) {

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
    error = pure_data_next_bit( block ); if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_STATE_PAUSE:
    /* The pause at the end of the block */
    *tstates = (block->pause * 69888)/20; /* FIXME: should vary with tstates
					     per frame */
    *end_of_block = 1;
    break;

  default:
    libspectrum_print_error( "pure_data_edge: unknown state %d\n",
			     block->state );
    return LIBSPECTRUM_ERROR_LOGIC;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
pure_data_next_bit( libspectrum_tape_pure_data_block *block )
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

    /* If we're looking at the last byte, take account of the fact it
       may have less than 8 bits in it */
    if( block->bytes_through_block == block->length-1 ) {
      block->bits_through_byte = 8 - block->bits_in_last_byte;
    } else {
      block->bits_through_byte = 0;
    }
  }

  /* Get the high bit, and shift the byte out leftwards */
  next_bit = block->current_byte & 0x80;
  block->current_byte <<= 1;

  /* And set the timing and state for another data bit */
  block->bit_tstates = ( next_bit ? block->bit1_length : block->bit0_length );
  block->state = LIBSPECTRUM_TAPE_STATE_DATA1;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
raw_data_edge( libspectrum_tape_raw_data_block *block,
	       libspectrum_dword *tstates, int *end_of_block )
{
  int error;

  switch (block->state) {
  case LIBSPECTRUM_TAPE_STATE_DATA1:
    *tstates = block->bit_tstates;
    error = raw_data_next_bit( block ); if( error ) return error;
    break;

  case LIBSPECTRUM_TAPE_STATE_PAUSE:
    /* The pause at the end of the block */
    *tstates = ( block->pause * 69888 )/20; /* FIXME: should vary with tstates
					       per frame */
    *end_of_block = 1;
    break;

  default:
    libspectrum_print_error( "raw_edge: unknown state %d\n", block->state );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
raw_data_next_bit( libspectrum_tape_raw_data_block *block )
{
  int length = 0;

  if( block->bytes_through_block == block->length ) {
    block->state = LIBSPECTRUM_TAPE_STATE_PAUSE;
    return LIBSPECTRUM_ERROR_NONE;
  }

  block->state = LIBSPECTRUM_TAPE_STATE_DATA1;

  /* Step through the data until we find an edge */
  do {
    length++;
    if( ++block->bits_through_byte == 8 ) {
      if( ++block->bytes_through_block == block->length - 1 ) {
	block->bits_through_byte = 8 - block->bits_in_last_byte;
      } else {
	block->bits_through_byte = 0;
      }
      if( block->bytes_through_block == block->length )
	break;
    }
  } while( (block->data[block->bytes_through_block] << block->bits_through_byte
            & 0x80 ) != block->last_bit) ;

  block->bit_tstates = length * block->bit_length;
  block->last_bit ^= 0x80;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
jump_blocks( libspectrum_tape *tape, int offset )
{
  gint current_position; GSList *new_block;

  current_position = g_slist_position( tape->blocks, tape->current_block );
  if( current_position == -1 ) return LIBSPECTRUM_ERROR_LOGIC;

  new_block = g_slist_nth( tape->blocks, current_position + offset );
  if( new_block == NULL ) return LIBSPECTRUM_ERROR_CORRUPT;

  tape->current_block = new_block;

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_tape_block_description( libspectrum_tape_block *block,
				    char *buffer, size_t length )
{
  switch( block->type ) {
  case LIBSPECTRUM_TAPE_BLOCK_ROM:
    strncpy( buffer, "Standard Speed Data Block", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_TURBO:
    strncpy( buffer, "Turbo Speed Data Block", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
    strncpy( buffer, "Pure Tone Block", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PULSES:
    strncpy( buffer, "List of Pulses", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
    strncpy( buffer, "Pure Data Block", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
    strncpy( buffer, "Raw Data Block", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
    strncpy( buffer, "Pause Block", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
    strncpy( buffer, "Group Start Block", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
    strncpy( buffer, "Group End Block", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_JUMP:
    strncpy( buffer, "Jump Block", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
    strncpy( buffer, "Loop Start Block", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
    strncpy( buffer, "Loop End Block", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_SELECT:
    strncpy( buffer, "Select Block", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_STOP48:
    strncpy( buffer, "Stop Tape If In 48K Mode Block", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
    strncpy( buffer, "Comment Block", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
    strncpy( buffer, "Message Block", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
    strncpy( buffer, "Archive Info Block", length );
    break;
  case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
    strncpy( buffer, "Hardware Information Block", length );
    break;

  case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
    strncpy( buffer, "Custom Info Block", length );
    break;

  default:
    libspectrum_print_error(
      "libspectrum_tape_block_description: unknown block type 0x%02x\n",
      block->type
    );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  buffer[ length-1 ] = '\0';
  return LIBSPECTRUM_ERROR_NONE;
}
