/* tape.c: tape handling routines
   Copyright (c) 1999-2002 Philip Kendall, Darren Salt

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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include <libspectrum.h>

#include "event.h"
#include "fuse.h"
#include "machine.h"
#include "settings.h"
#include "sound.h"
#include "spectrum.h"
#include "settings.h"
#include "snapshot.h"
#include "tape.h"
#include "ui/ui.h"
#include "utils.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

/* The current tape */
libspectrum_tape *tape;

/* Is the emulated tape deck playing? */
int tape_playing;

/* Is there a high input to the EAR socket? */
int tape_microphone;

/* Function prototypes */

static int tape_autoload( libspectrum_machine hardware );
static int trap_load_block( libspectrum_tape_rom_block *block );
int trap_check_rom( void );

/* Function defintions */

int tape_init( void )
{
  libspectrum_error error;

  error = libspectrum_tape_alloc( &tape );
  if( !tape ) return error;

  tape_playing = 0;
  tape_microphone = 0;
  if( settings_current.sound_load ) sound_beeper( 1, tape_microphone );
  return 0;
}

int tape_open( const char *filename )
{
  unsigned char *buffer; size_t length;

  int error; libspectrum_error_function_t old_error_fn;

  /* Get the file's data */
  error = utils_read_file( filename, &buffer, &length );
  if( error ) return error;

  /* If we already have a tape file open, close it */
  if( libspectrum_tape_present( tape ) ) {
    error = tape_close();
    if( error ) { munmap( buffer, length ); return error; }
  }

  /* First, try opening the file as a .tzx file; if we get back an
     error saying it didn't have the .tzx signature, then try opening
     it as a .tap file */

  /* Temporarily ignore signature errors */
  old_error_fn = libspectrum_error_function;
  libspectrum_error_function = ui_libspectrum_error_ignore_sig;

  error = libspectrum_tzx_read( tape, buffer, length );
  if( error == LIBSPECTRUM_ERROR_SIGNATURE ) {
    error = libspectrum_tap_read( tape, buffer, length );
  }

  libspectrum_error_function = old_error_fn;

  if( error != LIBSPECTRUM_ERROR_NONE ) {
    munmap( buffer, length );
    return error;
  }

  if( munmap( buffer, length ) == -1 ) {
    tape_close();
    ui_error( UI_ERROR_ERROR, "Couldn't munmap '%s': %s", filename,
	      strerror( errno ) );
    return 1;
  }

  if( settings_current.auto_load ) {
    error = tape_autoload( machine_current->machine );
    if( error ) return error;
  }

  return 0;

}

/* Use an already open .tap file as the current tape */
int
tape_open_tap_buffer( unsigned char *buffer, size_t length )
{
  int error;

  if( libspectrum_tape_present( tape ) ) {
    error = tape_close(); if( error ) return error;
  }

  return libspectrum_tap_read( tape, buffer, length );
}

/* Use an already open .tzx file as the current tape */
int
tape_open_tzx_buffer( unsigned char *buffer, size_t length )
{
  int error;

  if( libspectrum_tape_present( tape ) ) {
    error = tape_close(); if( error ) return error;
  }

  return libspectrum_tzx_read( tape, buffer, length );
}

/* Load a snap to start the current tape autoloading */
static int
tape_autoload( libspectrum_machine hardware )
{
  int error; const char *id; int fd;
  char filename[80];
  unsigned char *snap; size_t length;

  /* If no hardware specified, get a best guess from the tape */
  if( hardware == LIBSPECTRUM_MACHINE_UNKNOWN ) {
    error = libspectrum_tape_guess_hardware( &hardware, tape );
    if( error ) return error;

    /* If we still don't know, default to 48K */
    if( hardware == LIBSPECTRUM_MACHINE_UNKNOWN )
      hardware = LIBSPECTRUM_MACHINE_48;
  }

  id = machine_get_id( hardware );
  if( !id ) {
    ui_error( UI_ERROR_ERROR, "Unknown machine type %d!", hardware );
    return 1;
  }

  snprintf( filename, 80, "tape_%s.z80", id );
  fd = utils_find_lib( filename );
  if( fd == -1 ) {
    ui_error( UI_ERROR_ERROR,
	      "Couldn't find autoload snap for machine type '%s'", id );
    return 1;
  }

  error = utils_read_fd( fd, filename, &snap, &length );
  if( error ) return error;

  error = snapshot_open_z80_buffer( snap, length );
  if( error ) { munmap( snap, length ); return error; }

  if( munmap( snap, length ) ) {
    ui_error( UI_ERROR_ERROR, "Couldn't munmap '%s': %s", filename,
	      strerror( errno ) );
    return 1;
  }
    
  return 0;
}

/* Close the active tape file */
int tape_close( void )
{
  int error;

  /* Stop the tape if it's currently playing */
  if( tape_playing ) {
    error = tape_stop();
    if( error ) return error;
  }

  /* And then remove it from memory */
  error = libspectrum_tape_clear( tape );
  if( error ) return error;

  return 0;
}

/* Select the nth block on the tape; 0 => 1st block */
int
tape_select_block( size_t n )
{
  return libspectrum_tape_nth_block( tape, n );
}

/* Which block is current? */
int
tape_get_current_block( void )
{
  int n;
  libspectrum_error error;

  if( !libspectrum_tape_present( tape ) ) return -1;

  error = libspectrum_tape_position( &n, tape );
  if( error ) return -1;

  return n;
}

/* Write the current in-memory tape file out to disk */
int tape_write( const char* filename )
{
  libspectrum_byte *buffer; size_t length;

  int error;

  length = 0;
  error = libspectrum_tzx_write( &buffer, &length, tape );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = utils_write_file( filename, buffer, length );
  if( error ) { free( buffer ); return error; }

  return 0;

}

/* Load the next tape block into memory; returns 0 if a block was
   loaded (even if it had an tape loading error or equivalent) or
   non-zero if there was an error at the emulator level, or tape traps
   are not active */
int tape_load_trap( void )
{
  libspectrum_tape_block *current_block;
  libspectrum_tape_rom_block *rom_block;

  GSList *block_ptr; libspectrum_tape_block *next_block;

  int error;

  /* Do nothing if tape traps aren't active, or the tape is already playing */
  if( ! settings_current.tape_traps || tape_playing ) return 2;

  /* Do nothing if we're not in the correct ROM */
  if( ! trap_check_rom() ) return 3;

  /* Return with error if no tape file loaded */
  if( !libspectrum_tape_present( tape ) ) return 1;

  current_block = (libspectrum_tape_block*)(tape->current_block->data);

  block_ptr = tape->current_block->next;
  /* Loop if we hit the end of the tape */
  if( block_ptr == NULL ) { block_ptr = tape->blocks; }

  next_block = (libspectrum_tape_block*)(block_ptr->data);
  libspectrum_tape_init_block( next_block );

  /* If this block isn't a ROM loader, start it playing
     Then return with `error' so that we actually do whichever instruction
     it was that caused the trap to hit */
  if( current_block->type != LIBSPECTRUM_TAPE_BLOCK_ROM ) {

    error = tape_play();
    if( error ) return 3;

    return -1;
  }

  rom_block = &(current_block->types.rom);

  /* All returns made via the RET at #05E2 */
  PC = 0x05e2;

  error = trap_load_block( rom_block );
  if( error ) return error;

  /* Peek at the next block. If it's a ROM block, just move along and
     return */
  if( next_block->type == LIBSPECTRUM_TAPE_BLOCK_ROM ) {
    tape->current_block = block_ptr;
    return 0;
  }

  /* If the next block isn't a ROM block, set ourselves up such that the
     next thing to occur is the pause at the end of the current block */
  current_block->types.rom.state = LIBSPECTRUM_TAPE_STATE_PAUSE;

  /* And start the tape playing */
  error = tape_play();
  /* On error, still return without error as we did sucessfully do
     the tape trap, and so don't want to do the trigger instruction */
  if( error ) return 0;

  return 0;

}

static int trap_load_block( libspectrum_tape_rom_block *block )
{
  libspectrum_byte parity, *data;
  int loading, i;

  /* If the block's too short, give up and go home (with carry reset
     to indicate error */
  if( block->length < DE ) { 
    F = ( F & ~FLAG_C );
    return 0;
  }

  data = block->data;
  parity = *data;

  /* If the flag byte (stored in A') does not match, reset carry and return */
  if( *data++ != A_ ) {
    F = ( F & ~FLAG_C );
    return 0;
  }

  /* Loading or verifying determined by the carry flag of F' */
  loading = ( F_ & FLAG_C );

  if( loading ) {
    for( i=0; i<DE; i++ ) {
      writebyte( IX+i, *data );
      parity ^= *data++;
    }
  } else {		/* verifying */
    for( i=0; i<DE; i++) {
      parity ^= *data;
      if( *data++ != readbyte(IX+i) ) {
	F = ( F & ~FLAG_C );
	return 0;
      }
    }
  }

  /* If the parity byte does not match, reset carry and return */
  if( *data++ != parity ) {
    F = ( F & ~FLAG_C );
    return 0;
  }

  /* Else return with carry set */
  F |= FLAG_C;

  return 0;
}

/* Append to the current tape file in memory; returns 0 if a block was
   saved or non-zero if there was an error at the emulator level, or tape
   traps are not active */
int tape_save_trap( void )
{
  libspectrum_tape_block *block;
  libspectrum_tape_rom_block *rom_block;

  libspectrum_byte parity;

  int i; libspectrum_error error;

  /* Do nothing if tape traps aren't active */
  if( ! settings_current.tape_traps ) return 2;

  /* Check we're in the right ROM */
  if( ! trap_check_rom() ) return 3;

  /* Get a new block to store this data in */
  block = (libspectrum_tape_block*)malloc( sizeof( libspectrum_tape_block ));
  if( block == NULL ) return 1;

  /* This is a ROM block */
  block->type = LIBSPECTRUM_TAPE_BLOCK_ROM;
  rom_block = &(block->types.rom);

  /* The +2 here is for the flag and parity bytes */
  rom_block->length = DE + 2;

  rom_block->data =
    (libspectrum_byte*)malloc( rom_block->length * sizeof(libspectrum_byte) );
  if( rom_block->data == NULL ) {
    free( block );
    return 1;
  }

  /* First, store the flag byte (and initialise the parity counter) */
  rom_block->data[0] = parity = A;

  /* then the main body of the data, counting parity along the way */
  for( i=0; i<DE; i++) {
    libspectrum_byte b = readbyte( IX+i );
    parity ^= b;
    rom_block->data[i+1] = b;
  }

  /* And finally the parity byte */
  rom_block->data[ DE+1 ] = parity;

  /* Give a 1 second pause after this block */
  rom_block->pause = 1000;

  /* Add the block to the current tape file */
  error = libspectrum_tape_append_block( tape, block );
  if( error ) return error;

  /* And then return via the RET at #053E */
  PC = 0x053e;

  return 0;

}

/* Check whether we're actually in the right ROM when a tape trap hit */
int trap_check_rom( void )
{
  switch( machine_current->machine ) {
  case LIBSPECTRUM_MACHINE_48:
  case LIBSPECTRUM_MACHINE_TC2048:
    return 1;		/* Always OK here */

  case LIBSPECTRUM_MACHINE_128:
  case LIBSPECTRUM_MACHINE_PLUS2:
    /* OK if we're in ROM 1 */
    return( machine_current->ram.current_rom == 1 );

  case LIBSPECTRUM_MACHINE_PLUS2A:
  case LIBSPECTRUM_MACHINE_PLUS3:
    /* OK if we're not in a 64Kb RAM configuration and we're in
       ROM 3 */
    return( ! machine_current->ram.special &&
	    machine_current->ram.current_rom == 3 );

    return 1;		/* Always OK here */
  }

  ui_error( UI_ERROR_ERROR, "Impossible machine type %d",
	    machine_current->machine );
  fuse_abort();
  return 0; /* Keep gcc happy */
}

int tape_play( void )
{
  libspectrum_tape_block* block;

  int error;

  if( !libspectrum_tape_present( tape ) ) return 1;
  
  block = (libspectrum_tape_block*)(tape->current_block->data);

  /* If tape traps are active and the current block is a ROM block, do
     nothing, _unless_ the ROM block has already reached the pause at
     its end which (hopefully) means we're in the magic state involving
     starting slow loading whilst tape traps are active */
  if( settings_current.tape_traps &&
      block->type == LIBSPECTRUM_TAPE_BLOCK_ROM &&
      block->types.rom.state != LIBSPECTRUM_TAPE_STATE_PAUSE )
    return 0;

  /* Otherwise, start the tape going */
  tape_playing = 1;
  tape_microphone = 0;

  /* Timex machines have no loading noise */
  if( ( !( machine_current->timex ) ) && settings_current.sound_load )
    sound_beeper( 1, tape_microphone );

  error = tape_next_edge( tstates ); if( error ) return error;

  return 0;
}

int tape_toggle_play( void )
{
  if( tape_playing ) {
    return tape_stop();
  } else {
    return tape_play();
  }
}

int tape_stop( void )
{
  tape_playing = 0;
  return 0;
}

int tape_next_edge( DWORD last_tstates )
{
  int error; libspectrum_error libspec_error;

  libspectrum_dword edge_tstates;
  int flags;

  /* If the tape's not playing, just return */
  if( ! tape_playing ) return 0;

  /* Get the time until the next edge */
  libspec_error = libspectrum_tape_get_next_edge( &edge_tstates, &flags,
						  tape );
  if( libspec_error != LIBSPECTRUM_ERROR_NONE ) return libspec_error;

  /* Invert the microphone state */
  if( edge_tstates || ( flags & LIBSPECTRUM_TAPE_FLAGS_STOP ) ) {
    tape_microphone = !tape_microphone;
    if( settings_current.sound_load ) sound_beeper( 1, tape_microphone );
  }

  /* If we've been requested to stop the tape, do so and then
     return without stacking another edge */
  /* FIXME: what should we do if we're emulating the TC2048 here?
     Yet more brokeness in the TZX format... */
  if( ( flags & LIBSPECTRUM_TAPE_FLAGS_STOP ) ||
      ( ( flags & LIBSPECTRUM_TAPE_FLAGS_STOP48 ) && 
	machine_current->machine == LIBSPECTRUM_MACHINE_48 )
    ) {
    error = tape_stop(); if( error ) return error;
    return 0;
  }

  /* If that was the end of a block, tape traps are active _and_ the
     new block is a ROM loader, return without putting another event
     into the queue */
  if( ( flags & LIBSPECTRUM_TAPE_FLAGS_BLOCK ) &&
      settings_current.tape_traps &&
      ((libspectrum_tape_block*)(tape->current_block->data))->type ==
        LIBSPECTRUM_TAPE_BLOCK_ROM
    ) {
    tape_playing = 0;
    return 0;
  }

  /* Otherwise, put this into the event queue; remember that this edge
     should occur 'edge_tstates' after the last edge, not after the
     current time (these will be slightly different as we only process
     events between instructions). */
  error = event_add( last_tstates + edge_tstates, EVENT_TYPE_EDGE );
  if( error ) return error;

  return 0;
}

int
tape_get_block_list( char ****list, size_t *n )
{
  GSList *block_list = tape->blocks;
  size_t allocated;

  *list = NULL; *n = 0; allocated = 0;

  while( block_list ) {

    libspectrum_tape_block *block =
      (libspectrum_tape_block*)(block_list->data);
    
    /* Get more space if we need it */
    if( *n == allocated ) {
      size_t new_size = allocated ? 2 * allocated : 1;
      char ***new_list = (char***)realloc( (*list),
					   new_size * sizeof( char** ) );
      if( !new_list ) {
	tape_free_block_list( *list, *n );
	ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d",
		  __FILE__, __LINE__ );
	return 1;
      }

      (*list) = new_list; allocated = new_size;
    }

    (*list)[*n] = (char**)malloc( 2 * sizeof( char* ) );
    if( !(*list)[*n] ) {
      tape_free_block_list( *list, *n );
      ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
      return 1;
    }

    (*list)[*n][0] = malloc( 80 * sizeof( char ) );
    if( !(*list)[*n][0] ) {
      free( (*list)[*n] ); tape_free_block_list( *list, *n );
      ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
      return 1;
    }

    (*list)[*n][1] = malloc( 80 * sizeof( char ) );
    if( !(*list)[*n][1] ) {
      free( (*list)[*n][0] ); free( (*list)[*n] );
      tape_free_block_list( *list, *n );
      ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
      return 1;
    }

    libspectrum_tape_block_description( (*list)[*n][0], 80, block );

    switch( block->type ) {

    case LIBSPECTRUM_TAPE_BLOCK_ROM:
      snprintf( (*list)[*n][1], 80, "%lu bytes",
		(unsigned long)block->types.rom.length );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_TURBO:
      snprintf( (*list)[*n][1], 80, "%lu bytes",
		(unsigned long)block->types.turbo.length );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
      snprintf( (*list)[*n][1], 80, "%lu tstates",
		(unsigned long)block->types.pure_tone.length );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PULSES:
      snprintf( (*list)[*n][1], 80, "%lu pulses",
		(unsigned long)block->types.pulses.count );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
      snprintf( (*list)[*n][1], 80, "%lu bytes",
		(unsigned long)block->types.pure_data.length );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
      snprintf( (*list)[*n][1], 80, "%lu bytes",
		(unsigned long)block->types.raw_data.length );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
      snprintf( (*list)[*n][1], 80, "%lu ms",
		(unsigned long)block->types.pause.length );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
      snprintf( (*list)[*n][1], 80, "%s", block->types.group_start.name );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_JUMP:
      if( block->types.jump.offset > 0 ) {
	snprintf( (*list)[*n][1], 80, "Forward %d blocks",
		  block->types.jump.offset );
      } else {
	snprintf( (*list)[*n][1], 80, "Backward %d blocks",
		  -block->types.jump.offset );
      }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
      snprintf( (*list)[*n][1], 80, "%d iterations",
		block->types.loop_start.count );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_SELECT:
      snprintf( (*list)[*n][1], 80, "%lu options",
		(unsigned long)block->types.select.count );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
      snprintf( (*list)[*n][1], 80, "%s", block->types.comment.text );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
      snprintf( (*list)[*n][1], 80, "%s", block->types.message.text );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
      snprintf( (*list)[*n][1], 80, "%s", block->types.custom.description );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
    case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
    case LIBSPECTRUM_TAPE_BLOCK_STOP48:
    case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
    case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
    default:
      (*list)[*n][1][0] = '\0';
      break;

    }

    (*n)++; block_list = block_list->next;
  }

  return 0;
}

int
tape_free_block_list( char ***list, size_t n )
{
  size_t i;

  for( i=0; i<n; i++ ) {
    free( list[i][0] ); free( list[i][1] );
    free( list[i] );
  }

  free( list );

  return 0;
}
