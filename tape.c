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
#include "libspectrum/tape.h"
#include "settings.h"
#include "spectrum.h"
#include "tape.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

/* The current tape */
libspectrum_tape tape;

/* Is the emulated tape deck playing? */
int tape_playing;

/* Is there a high input to the EAR socket? */
int tape_microphone;

#define ERROR_MESSAGE_MAX_LENGTH 1024

int tape_init( void )
{
  tape.blocks = NULL;
  tape_playing = 0;
  tape_microphone = 0;
  return 0;
}

int tape_open( const char *filename )
{
  struct stat file_info; int fd; unsigned char *buffer;

  int error; char error_message[ ERROR_MESSAGE_MAX_LENGTH ];

  /* If we already have a tape file open, close it */
  if( tape.blocks ) {
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

  if(    strlen( filename ) < 4
      || strncmp( &filename[ strlen(filename)-4 ], ".tap", 4 ) ) {
    
    error = libspectrum_tzx_create( &tape, buffer, file_info.st_size );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      fprintf( stderr,
	       "%s: error from libspectrum_tzx_create whilst reading `%s': %s\n",
	       fuse_progname, filename, libspectrum_error_message(error) );
      munmap( buffer, file_info.st_size );
      return error;
    }

  } else {

    error = libspectrum_tap_create( &tape, buffer, file_info.st_size );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      fprintf( stderr,
	       "%s: error from libspectrum_tap_create whilst reading `%s': %s\n",
	       fuse_progname, filename, libspectrum_error_message(error) );
      munmap( buffer, file_info.st_size );
      return error;
    }

  }

  if( munmap( buffer, file_info.st_size ) == -1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't munmap `%s'", fuse_progname, filename );
    perror( error_message );
    return 1;
  }

  /* And the tape is stopped */
  if( tape_playing ) tape_stop();

  return 0;

}

/* Close the active tape file */
int tape_close( void )
{
  int error;

  error = libspectrum_tape_free( &tape );
  if( error ) return error;

  return 0;
}

/* Load the next tape block into memory; returns 0 if a block was
   loaded (even if it had an tape loading error or equivalent) or
   non-zero if there was an error at the emulator level, or tape traps
   are not active */
int tape_trap( void )
{
  libspectrum_tape_block *current_block;
  libspectrum_tape_rom_block *rom_block;

  int loading;			/* Load (true) or verify (false) */
  BYTE parity, *ptr;

  int i;

  /* Do nothing if tape traps aren't active */
  if( ! settings_current.tape_traps ) return 2;

  /* Return with error if no tape file loaded */
  if( tape.blocks == NULL ) return 1;

  current_block = (libspectrum_tape_block*)(tape.current_block->data);

  /* We've done this block; move onto the next one. If we hit the end
     of the tape, loop back to the start */
  tape.current_block = tape.current_block->next;
  if( tape.current_block == NULL ) tape.current_block = tape.blocks;

  /* If this block isn't a ROM loader, return with error */
  if( current_block->type != LIBSPECTRUM_TAPE_BLOCK_ROM ) return 3;

  rom_block = &(current_block->types.rom);

  /* All returns made via the RET at #05E2 */
  PC = 0x05e2;

  /* If the block's too short, give up and go home (with carry reset
     to indicate error */
  if( rom_block->length < DE ) { 
    F = ( F & ~FLAG_C );
    return 0;
  }

  ptr = rom_block->data;
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

  if( tape.blocks == NULL ) return 1;
  
  tape_playing = 1;
  tape_microphone = 0;

  error = tape_next_edge(); if( error ) return error;

  return 0;
}

int tape_stop( void )
{
  tape_playing = 0;
  return 0;
}

int tape_next_edge( void )
{
  int error; libspectrum_error libspec_error;

  libspectrum_dword edge_tstates;
  int end_of_tape;

  /* If the tape's not playing, just return */
  if( ! tape_playing ) return 0;

  /* Invert the microphone state */
  tape_microphone = !tape_microphone;

  /* Get the time until the next edge */
  libspec_error = libspectrum_tape_get_next_edge( &tape, &edge_tstates,
						  &end_of_tape );
  if( libspec_error != LIBSPECTRUM_ERROR_NONE ) return libspec_error;

  /* And put this into the event queue */
  error = event_add( tstates + edge_tstates, EVENT_TYPE_EDGE );
  if( error ) return error;

  /* If this was the end of the tape, stop playing */
  if( end_of_tape) {
    error = tape_stop();
    if( error ) return error;
  }

  return 0;
}
