/* tzxlist.c: Produce a listing of the blocks in a .tzx file
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

#include <config.h>

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libspectrum/tape.h>

#define ERROR_MESSAGE_MAX_LENGTH 1024
#define DESCRIPTION_LENGTH 80

int
main( int argc, char **argv )
{
  char *progname = argv[0];

  char *filename; int fd;
  struct stat file_info;

  int error;
  char error_message[ ERROR_MESSAGE_MAX_LENGTH ];

  unsigned char *buffer;
  libspectrum_tape tape;

  GSList *ptr;

  int i;

  /* Show libspectrum error messages */
  libspectrum_show_errors = 1;

  tape.blocks = NULL;

  if( argc < 2 ) {
    fprintf( stderr, "%s: usage: %s <tzx file>\n", progname, progname );
    return 1;
  }

  filename = argv[1];

  fd = open( filename, O_RDONLY );
  if( fd == -1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: couldn't open `%s'", progname, filename );
    perror( error_message );
    return 1;
  }

  if( fstat( fd, &file_info) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't stat `%s'", progname, filename );
    perror( error_message );
    close(fd);
    return 1;
  }

  buffer = mmap( 0, file_info.st_size, PROT_READ, MAP_SHARED, fd, 0 );
  if( buffer == (void*)-1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't mmap `%s'", progname, filename );
    perror( error_message );
    close(fd);
    return 1;
  }

  if( close(fd) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't close `%s'", progname, filename );
    perror( error_message );
    munmap( buffer, file_info.st_size );
    return 1;
  }

  error = libspectrum_tzx_create( &tape, buffer, file_info.st_size );
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    fprintf( stderr,
	     "%s: error from libspectrum_tzx_create whilst reading `%s': %s\n",
	     progname, filename, libspectrum_error_message(error) );
    munmap( buffer, file_info.st_size );
    return error;
  }

  if( munmap( buffer, file_info.st_size ) == -1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't munmap `%s'", progname, filename );
    perror( error_message );
    return 1;
  }

  printf("Listing of `%s':\n\n", filename );

  ptr = tape.blocks;

  while( ptr ) {
    libspectrum_tape_block *block = (libspectrum_tape_block*)ptr->data;
    char description[ DESCRIPTION_LENGTH ];

    libspectrum_tape_rom_block *rom_block;
    libspectrum_tape_turbo_block *turbo_block;
    libspectrum_tape_pure_tone_block *tone_block;
    libspectrum_tape_pulses_block *pulses_block;
    libspectrum_tape_pure_data_block *data_block;
    libspectrum_tape_archive_info_block *info_block;

    error = libspectrum_tape_block_description(
      block, description, DESCRIPTION_LENGTH
    );
    if( error ) {
      fprintf( stderr, "%s: error from libspectrum_tape_block_description: %s\n",
	       progname, libspectrum_error_message( error ) );
      return 1;
    }
    printf( "Block type 0x%02x (%s)\n", block->type, description );

    switch( block->type ) {
    case LIBSPECTRUM_TAPE_BLOCK_ROM:
      rom_block = &(block->types.rom);
      printf("  Data length: %d bytes\n", rom_block->length );
      printf("  Pause length: %d ms\n", rom_block->pause );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_TURBO:
      turbo_block = &(block->types.turbo);
      printf("  %d pilot pulses of %d tstates\n",
	     turbo_block->pilot_pulses, turbo_block->pilot_length );
      printf("  Sync pulses of %d and %d tstates\n",
	     turbo_block->sync1_length, turbo_block->sync2_length );
      printf("  Data bits are %d (reset) and %d (set) tstates\n",
	     turbo_block->bit0_length, turbo_block->bit1_length );
      printf("  Data length: %d bytes (%d bits in last byte used)\n",
	     turbo_block->length, turbo_block->bits_in_last_byte );
      printf("  Pause length: %d ms\n", turbo_block->pause );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
      tone_block = &(block->types.pure_tone);
      printf("  %d pulses of %d tstates\n",
	     tone_block->pulses, tone_block->length );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PULSES:
      pulses_block = &(block->types.pulses);
      for( i=0; i<pulses_block->count; i++ )
	printf("  Pulse %3d: length %d tstates\n",
	       i, pulses_block->lengths[i] );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
      data_block = &(block->types.pure_data);
      printf("  Data bits are %d (reset) and %d (set) tstates\n",
	     data_block->bit0_length, data_block->bit1_length );
      printf("  Data length: %d bytes (%d bits in last byte used)\n",
	     data_block->length, data_block->bits_in_last_byte );
      printf("  Pause length: %d ms\n", data_block->pause );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
      printf("  Length: %d ms\n", block->types.pause.length );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
      printf("  Name: %s\n", block->types.group_start.name );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
      /* Do nothing */
      break;

    case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
      printf("  Comment: %s\n", block->types.comment.text );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
      info_block = &(block->types.archive_info);
      for( i=0; i<info_block->count; i++ ) {
	printf("  ");
	switch( info_block->ids[i] ) {
	case   0: printf("Full Title:"); break;
	case   1: printf(" Publisher:"); break;
	case   2: printf("    Author:"); break;
	case   3: printf("      Year:"); break;
	case   4: printf(" Langugage:"); break;
	case   5: printf("  Category:"); break;
	case   6: printf("    Budget:"); break;
	case   7: printf("    Loader:"); break;
	case   8: printf("    Origin:"); break;
	case 255: printf("   Comment:"); break;
	 default: printf("(Unknown string): "); break;
	}
	printf(" %s\n", info_block->strings[i] );
      }
      break;

    default:
      printf("  (Sorry -- %s can't handle that kind of block. Skipping it)\n",
	     progname );
      break;
    }

    ptr = ptr->next;
  }

  return 0;
}
