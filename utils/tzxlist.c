/* tzxlist.c: Produce a listing of the blocks in a .tzx file
   Copyright (c) 2001 Philip Kendall, Darren Salt

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
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libspectrum/tape.h>

#define DESCRIPTION_LENGTH 80

static const char *progname;

static const char*
hardware_desc( int type, int id )
{
  switch( type ) {
  case 0:
    switch( id ) {
    case 0: return "16K Spectrum";
    case 1: return "48K Spectrum/Spectrum +";
    case 2: return "48K Spectrum (Issue 1)";
    case 3: return "128K Spectrum";
    case 4: return "Spectrum +2";
    case 5: return "Spectrum +2A/+3";
    default: return "Unknown machine";
    }
  case 3:
    switch( id ) {
    case 0: return "AY-3-8192";
    default: return "Unknown sound device";
    }
  case 4:
    switch( id ) {
    case 0: return "Kempston joystick";
    case 1: return "Cursor/Protek/AGF joystick";
    case 2: return "Sinclair joystick (Left)";
    case 3: return "Sinclair joystick (Right)";
    case 4: return "Fuller joystick";
    default: return "Unknown joystick";
    }
  default: return "Unknown type";
  }
}

static int
process_tzx( char *filename )
{
  int fd;
  struct stat file_info;

  int error;

  unsigned char *buffer;
  libspectrum_tape tape;

  GSList *ptr;

  size_t i;

  tape.blocks = NULL;

  fd = open( filename, O_RDONLY );
  if( fd == -1 ) {
    fprintf( stderr, "%s: couldn't open `%s': %s\n", progname, filename,
	     strerror( errno ) );
    return 1;
  }

  if( fstat( fd, &file_info) ) {
    fprintf( stderr, "%s: couldn't stat `%s': %s\n", progname, filename,
	     strerror( errno ) );
    close(fd);
    return 1;
  }

  buffer = mmap( 0, file_info.st_size, PROT_READ, MAP_SHARED, fd, 0 );
  if( buffer == (void*)-1 ) {
    fprintf( stderr, "%s: couldn't mmap `%s': %s\n", progname, filename,
	     strerror( errno ) );
    close(fd);
    return 1;
  }

  if( close(fd) ) {
    fprintf( stderr, "%s: couldn't close `%s': %s\n", progname, filename,
	     strerror( errno ) );
    munmap( buffer, file_info.st_size );
    return 1;
  }

  error = libspectrum_tzx_create( &tape, buffer, file_info.st_size );
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    munmap( buffer, file_info.st_size );
    return error;
  }

  if( munmap( buffer, file_info.st_size ) == -1 ) {
    fprintf( stderr, "%s: couldn't munmap `%s': %s\n", progname, filename,
	     strerror( errno ) );
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
    libspectrum_tape_raw_data_block *raw_block;
    libspectrum_tape_select_block *select_block;
    libspectrum_tape_archive_info_block *info_block;
    libspectrum_tape_hardware_block *hardware_block;

    error = libspectrum_tape_block_description(
      block, description, DESCRIPTION_LENGTH
    );
    if( error ) return 1;
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

    case LIBSPECTRUM_TAPE_BLOCK_RAW_DATA:
      raw_block = &(block->types.raw_data);
      printf("  Length: %d bytes\n", raw_block->length );
      printf("  Bits in last byte: %d\n", raw_block->bits_in_last_byte );
      printf("  Each bit is %d tstates\n", raw_block->bit_length );
      printf("  Pause length: %d ms\n", raw_block->pause );
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

    case LIBSPECTRUM_TAPE_BLOCK_JUMP:
      printf("  Offset: %d\n", block->types.jump.offset );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
      printf("  Count: %d\n", block->types.loop_start.count );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
      /* Do nothing */
      break;

    case LIBSPECTRUM_TAPE_BLOCK_SELECT:
      select_block = &(block->types.select);
      for( i=0; i<select_block->count; i++ ) {
	printf("  Choice %2d: Offset %d: %s\n", i, select_block->offsets[i],
	       select_block->descriptions[i] );
      }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_STOP48:
      /* Do nothing */
      break;

    case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
      printf("  Comment: %s\n", block->types.comment.text );
      break;

    case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
      printf("  Display for %d seconds\n", block->types.message.time );
      printf("  Comment: %s\n", block->types.message.text );
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
	case   6: printf("     Price:"); break;
	case   7: printf("    Loader:"); break;
	case   8: printf("    Origin:"); break;
	case 255: printf("   Comment:"); break;
	 default: printf("(Unknown string): "); break;
	}
	printf(" %s\n", info_block->strings[i] );
      }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
      hardware_block = &(block->types.hardware);
      for( i=0; i<hardware_block->count; i++ ) {
	printf( "  %s: ",
	        hardware_desc( hardware_block->types[i],
			       hardware_block->ids[i]
			     )
	      );
	switch( hardware_block->values[i] ) {
	case 0: printf("runs"); break;
	case 1: printf("runs, using hardware"); break;
	case 2: printf("runs, does not use hardware"); break;
	case 3: printf("does not run"); break;
	}
	printf("\n");
      }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
      printf( "  Description: %s\n", block->types.custom.description );
      printf( "       Length: %d bytes\n", block->types.custom.length );
      break;

    default:
      printf("  (Sorry -- %s can't handle that kind of block. Skipping it)\n",
	     progname );
      break;
    }

    ptr = ptr->next;
  }

  error = libspectrum_tape_free( &tape );
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    munmap( buffer, file_info.st_size );
    return error;
  }

  return 0;
}

int
main( int argc, char **argv )
{
  int ret = 0;
  int arg = 0;

  progname = argv[0];

  /* Show libspectrum error messages */
  libspectrum_show_errors = 1;

  if( argc < 2 ) {
    fprintf( stderr, "%s: usage: %s <tzx files>...\n", progname, progname );
    return 1;
  }

  while( ++arg < argc )
    ret |= process_tzx( argv[arg] );

  return ret;
}
