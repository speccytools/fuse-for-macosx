/* tape.c: tape handling routines
   Copyright (c) 1999-2001 Philip Kendall

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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_LIB_GLIB		/* If we're using glib */
#include <glib.h>
#else				/* #ifdef HAVE_LIB_GLIB */
#include "myglib.h"		/* If not, use the local replacement */
#endif

#include "event.h"
#include "fuse.h"
#include "settings.h"
#include "spectrum.h"
#include "tape.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

/* The list of tape blocks */
static GSList* tape_block_list;

/* The current tape block */
static GSList* tape_block_pointer;

/* Is the emulated tape deck playing? */
int tape_playing;

/* Is there a high input to the EAR socket? */
int tape_microphone;

/* Some states for the edge generator */
typedef enum tape_edge_state_t {
  TAPE_EDGE_STATE_PILOT,
  TAPE_EDGE_STATE_SYNC1,
  TAPE_EDGE_STATE_SYNC2,
  TAPE_EDGE_STATE_DATA1,
  TAPE_EDGE_STATE_DATA2,
} tape_edge_state_t;
/* And the current state of the generator */
static tape_edge_state_t tape_edge_state;

/* How many pilot pulses are still to be generated? */
static int tape_edge_count;

/* How far through the current block are we? */
static int tape_bytes_through_block;
static int tape_bits_through_byte;

/* The current byte we're loading (gets shifted out rightwards) */
static int tape_current_byte;

/* The length of the current data bit (in T-states) */
static int tape_next_bit_timing;

/* The lengths of various pulses */
static const int tape_timings_pilot = 2168;
static const int tape_timings_sync1 =  667;
static const int tape_timings_sync2 =  735;
static const int tape_timings_data0 =  855;
static const int tape_timings_data1 = 1710;
static const int tape_timings_pause = 69888 * 50; /* FIXME: should vary
						     with T-states per frame */

#define ERROR_MESSAGE_MAX_LENGTH 1024

static int tape_buffer_to_block_list( GSList **block_list_ptr,
				      const unsigned char *buffer,
				      const size_t length );
static int tape_block_list_clear( GSList **block_list_ptr );
static void tape_free_entry( gpointer data, gpointer user_data );

static int tape_start_block( void );
static int tape_get_next_bit( void );

int tape_init( void )
{
  tape_block_list = NULL;
  tape_playing = 0;
  tape_microphone = 0;
  return 0;
}

int tape_open( const char *filename )
{
  struct stat file_info; int fd; unsigned char *buffer;

  int error; char error_message[ ERROR_MESSAGE_MAX_LENGTH ];

  /* If we already have a tape file open, close it */
  if( tape_block_list ) {
    error = tape_close();
    if( error ) return error;
  }

  fd = open( filename, O_RDONLY );
  if( fd == -1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: couldn't open `%s'", fuse_progname, filename );
    perror( error_message );
    return 1;
  }

  if( fstat( fd, &file_info) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't stat `%s'", fuse_progname, filename );
    perror( error_message );
    close(fd);
    return 1;
  }

  buffer = mmap( 0, file_info.st_size, PROT_READ, MAP_SHARED, fd, 0 );
  if( buffer == (void*)-1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't mmap `%s'", fuse_progname, filename );
    perror( error_message );
    close(fd);
    return 1;
  }

  if( close(fd) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't close `%s'", fuse_progname, filename );
    perror( error_message );
    munmap( buffer, file_info.st_size );
    return 1;
  }

  error = tape_buffer_to_block_list( &tape_block_list, buffer,
				     file_info.st_size );
  if( error ) {
    fprintf( stderr, "%s: error from tape_buffer_to_block_list: %d\n",
	     fuse_progname, error );
    munmap( buffer, file_info.st_size );
    return error;
  }

  if( munmap( buffer, file_info.st_size ) == -1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't munmap `%s'", fuse_progname, filename );
    perror( error_message );
    return 1;
  }

  /* Current block is the first block on the tape */
  tape_block_pointer = tape_block_list;

  return 0;

}

static int tape_buffer_to_block_list( GSList **block_list_ptr,
				      const unsigned char *buffer,
				      const size_t length )
{
  const unsigned char *ptr, *end;

  tape_block_t *block_ptr;

  ptr = buffer; end = buffer + length;

  while( ptr < end ) {

    /* If we've got less than two bytes for the length, something's
       gone wrong, so gone home */
    if( ( end - ptr ) < 2 ) {
      tape_block_list_clear( block_list_ptr );
      return 1;
    }

    /* Get memory for a new block */
    block_ptr = (tape_block_t*)malloc( sizeof( tape_block_t ) );
    if( block_ptr == NULL ) return 1;

    /* Get the length of the next block, and move along */
    block_ptr->length = ptr[0] + ptr[1] * 0x100; ptr += 2;

    /* Have we got enough bytes left in buffer? */
    if( ( end - ptr ) < block_ptr->length ) {
      tape_block_list_clear( block_list_ptr );
      free( block_ptr );
      return 1;
    }

    /* Allocate memory for the data */
    block_ptr->data = (unsigned char*)malloc( block_ptr->length *
					      sizeof( unsigned char ) );
    if( block_ptr->data == NULL ) {
      tape_block_list_clear( block_list_ptr );
      return 1;
    }

    /* Copy the block data across, and move along */
    memcpy( block_ptr->data, ptr, block_ptr->length );
    ptr += block_ptr->length;

    /* And put it into the block list */
    *(block_list_ptr) = g_slist_append( (*block_list_ptr),
					(gpointer)block_ptr );

  }

  return 0;

}

/* Close the active tape file */
int tape_close( void )
{
  int error;

  error = tape_block_list_clear( &tape_block_list );
  if( error ) return error;

  return 0;
}

/* Free all the memory used by a block list */
static int tape_block_list_clear( GSList **block_list_ptr )
{
  g_slist_foreach( *(block_list_ptr), tape_free_entry, NULL );
  g_slist_free( *(block_list_ptr) );

  *(block_list_ptr) = NULL;
  
  return 0;
}

/* Free the memory used by a specific entry */
static void tape_free_entry( gpointer data, gpointer user_data )
{
  tape_block_t *ptr = (tape_block_t*)data;
  free(ptr);
}

/* Load the next tape block into memory; returns 0 if a block was
   loaded (even if it had an tape loading error or equivalent) or
   non-zero if there was an error at the emulator level, or tape traps
   are not active */
int tape_trap( void )
{
  tape_block_t *current_block;

  int loading;			/* Load (true) or verify (false) */
  BYTE parity, *ptr;

  int i;

  /* Do nothing if tape traps aren't active */
  if( ! settings_current.tape_traps ) return 2;

  /* Return with error if no tape file loaded */
  if( tape_block_list == NULL ) return 1;

  current_block = (tape_block_t*)(tape_block_pointer->data);

  /* We've done this block; move onto the next one. If we hit the end
     of the tape, loop back to the start */
  tape_block_pointer = tape_block_pointer->next;
  if( tape_block_pointer == NULL ) tape_block_pointer = tape_block_list;

  /* All returns made via the RET at #05E2 */
  PC = 0x05e2;

  /* If the block's too short, give up and go home (with carry reset
     to indicate error */
  if( current_block->length < DE ) { 
    F = ( F & ~FLAG_C );
    return 0;
  }

  ptr = current_block->data;
  parity = *ptr;

  /* If the flag byte (stored in A') does not match, reset carry and return */
  if( *ptr++ != A_ ) {
    F = ( F & ~FLAG_C );
    return 0;
  }

  /* Loading or verifying determined by the carry flag of F' */
  loading = ( F_ & FLAG_C );

  if( loading ) {
    for( i=0; i<DE; i++ ) {
      writebyte( IX+i, *ptr );
      parity ^= *ptr++;
    }
  } else {		/* verifying */
    for( i=0; i<DE; i++) {
      parity ^= *ptr;
      if( *ptr++ != readbyte(IX+i) ) {
	F = ( F & ~FLAG_C );
	return 0;
      }
    }
  }

  /* If the parity byte does not match, reset carry and return */
  if( *ptr++ != parity ) {
    F = ( F & ~FLAG_C );
    return 0;
  }

  /* Else return with carry set */
  F |= FLAG_C;

  return 0;

}

int tape_play( void )
{
  int error;

  if( tape_block_list == NULL ) return 1;
  
  tape_playing = 1;

  error = tape_start_block(); if( error ) return error;
  error = tape_next_edge(); if( error ) return error;

  return 0;
}

int tape_stop( void )
{
  tape_playing = 0;
  tape_microphone = 0;
  return 0;
}

int tape_next_edge( void )
{
  int error;

  /* If the tape's not playing, return with an error */
  if( ! tape_playing ) return 2;

  /* Invert the microphone state */
  tape_microphone = !tape_microphone;

  switch( tape_edge_state ) {

  case TAPE_EDGE_STATE_PILOT:
    /* The next edge occurs after one pilot pulse */
    error = event_add( tstates + tape_timings_pilot, EVENT_TYPE_EDGE );
    if( error ) return 1;

    /* If that was the last pilot pulse, the next one is the first sync
       pulse. If not, it's another pilot pulse. */
    if( --tape_edge_count == 0 ) tape_edge_state = TAPE_EDGE_STATE_SYNC1;

    break;

  case TAPE_EDGE_STATE_SYNC1:
    /* The first short sync pulse, followed by the second sync pulse */
    error = event_add( tstates + tape_timings_sync1, EVENT_TYPE_EDGE );
    if( error ) return 1;
    tape_edge_state = TAPE_EDGE_STATE_SYNC2;
    break;

  case TAPE_EDGE_STATE_SYNC2:
    /* The second sync pulse, followed by the first pulse for one
       bit of data */
    error = event_add( tstates + tape_timings_sync2, EVENT_TYPE_EDGE );
    if( error ) return 1;
    error = tape_get_next_bit(); if( error ) return error;
    break;

  case TAPE_EDGE_STATE_DATA1:
    error = event_add( tstates + tape_next_bit_timing, EVENT_TYPE_EDGE );
    if( error ) return error;
    tape_edge_state = TAPE_EDGE_STATE_DATA2;
    break;

  case TAPE_EDGE_STATE_DATA2:
    error = event_add( tstates + tape_next_bit_timing, EVENT_TYPE_EDGE );
    if( error ) return error;
    error = tape_get_next_bit(); if( error ) return error;
    break;
  }

  return 0;
}

static int tape_start_block( void )
{
  tape_block_t *ptr;

  ptr = (tape_block_t*)(tape_block_pointer->data);

  tape_microphone = 0;

  if( ptr->data[0] & 0x80 ) {	/* program or data */
    tape_edge_count = 0x0c97;
  } else {			/* header */
    tape_edge_count = 0x1f79;
  }

  tape_bytes_through_block = -1; tape_bits_through_byte = 7;
  tape_edge_state = TAPE_EDGE_STATE_PILOT;

  return 0;
}

static int tape_get_next_bit( void )
{
  tape_block_t *ptr;
  int next_bit;

  int error;

  ptr = (tape_block_t*)(tape_block_pointer->data);

  if( ++tape_bits_through_byte == 8 ) {
    if( ++tape_bytes_through_block == ptr->length ) {
      tape_block_pointer = tape_block_pointer->next;
      if( tape_block_pointer == NULL ) {
	tape_stop();
	tape_block_pointer = tape_block_list;
	return 0;
      }
      error = event_add( tstates + tape_timings_pause, EVENT_TYPE_EDGE );
      if( error ) return error;
      error = tape_start_block(); if( error ) return error;
      return 0;
    }
    tape_current_byte = ptr->data[ tape_bytes_through_block ];
    tape_bits_through_byte = 0;
  }

  next_bit = tape_current_byte & 0x01;
  tape_current_byte >>= 1;

  tape_next_bit_timing = ( next_bit ? tape_timings_data1
			            : tape_timings_data0 );
  tape_edge_state = TAPE_EDGE_STATE_DATA1;

  return 0;
}
