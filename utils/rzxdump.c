/* rzxdump.c: get info on an RZX file
   Copyright (C) 2002 Philip Kendall

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

#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "types.h"
#include "ui/ui.h"

char *progname;

const char *rzx_signature = "RZX!";

WORD read_word( unsigned char **ptr );
DWORD read_dword( unsigned char **ptr );

static int
do_file( const char *filename );

static int
read_creator_block( unsigned char **ptr, unsigned char *end );
static int
read_snapshot_block( unsigned char **ptr, unsigned char *end );
static int
read_input_block( unsigned char **ptr, unsigned char *end );

WORD read_word( unsigned char **ptr )
{
  WORD result = (*ptr)[0] + (*ptr)[1] * 0x100;
  (*ptr) += 2;
  return result;
}

DWORD read_dword( unsigned char **ptr )
{
  DWORD result = (*ptr)[0]             +
                 (*ptr)[1] *     0x100 +
                 (*ptr)[2] *   0x10000 +
                 (*ptr)[3] * 0x1000000;
  (*ptr) += 4;
  return result;
}


int main( int argc, char **argv )
{
  int i;

  progname = argv[0];

  if( argc < 2 ) {
    fprintf( stderr, "%s: Usage: %s <rzx file(s)>\n", progname, progname );
    return 1;
  }

  for( i=1; i<argc; i++ ) {
    do_file( argv[i] );
  }    

  return 0;
}

static int
do_file( const char *filename )
{
  int fd; struct stat file_info;

  unsigned char *buffer, *ptr, *end;

  printf( "Examining file %s\n", filename );

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

  ptr = buffer; end = buffer + file_info.st_size;

  /* Read the RZX header */

  if( end - ptr < (ptrdiff_t)strlen( rzx_signature ) + 6 ) {
    fprintf( stderr,
	     "%s: Not enough bytes for RZX header (%ld bytes)\n",
	     progname, (unsigned long)strlen( rzx_signature ) + 6 );
    munmap( buffer, file_info.st_size );
    return 1;
  }

  if( memcmp( ptr, rzx_signature, strlen( rzx_signature ) ) ) {
    fprintf( stderr, "%s: Wrong signature: expected `%s'\n", progname,
	     rzx_signature );
    munmap( buffer, file_info.st_size );
    return 1;
  }

  printf( "Found RZX signature\n" );
  ptr += strlen( rzx_signature );

  printf( "  Major version: %d\n", (int)*ptr++ );
  printf( "  Minor version: %d\n", (int)*ptr++ );
  printf( "  Flags: %d\n", read_dword( &ptr ) );

  while( ptr < end ) {

    unsigned char id;
    int error;

    id = *ptr++;

    switch( id ) {

    case 0x10:
      error = read_creator_block( &ptr, end );
      if( error ) { munmap( buffer, file_info.st_size ); return 1; }
      break;

    case 0x30:
      error = read_snapshot_block( &ptr, end );
      if( error ) { munmap( buffer, file_info.st_size ); return 1; }
      break;

    case 0x80:
      error = read_input_block( &ptr, end );
      if( error ) { munmap( buffer, file_info.st_size ); return 1; }
      break;

    default:
      fprintf( stderr, "%s: Unknown block type 0x%02x\n", progname, id );
      munmap( buffer, file_info.st_size );
      return 1;

    }

  }

  if( munmap( buffer, file_info.st_size ) == -1 ) {
    fprintf( stderr, "%s: couldn't munmap `%s': %s\n", progname, filename,
	     strerror( errno ) );
    return 1;
  }

  return 0;
}

static int
read_creator_block( unsigned char **ptr, unsigned char *end )
{
  size_t length;

  if( end - *ptr < 28 ) {
    fprintf( stderr,
	     "%s: Not enough bytes for creator block\n", progname );
    return 1;
  }

  printf( "Found a creator block\n" );

  length = read_dword( ptr );
  printf( "  Length: %ld bytes\n", (unsigned long)length );
  printf( "  Creator: `%s'\n", *ptr ); (*ptr) += 20;
  printf( "  Creator major version: %d\n", read_word( ptr ) );
  printf( "  Creator minor version: %d\n", read_word( ptr ) );
  printf( "  Creator custom data: %ld bytes\n", (unsigned long)length - 29 );
  (*ptr) += length - 29;

  return 0;
}

static int
read_snapshot_block( unsigned char **ptr, unsigned char *end )
{
  size_t block_length;

  if( end - *ptr < 16 ) {
    fprintf( stderr,
	     "%s: Not enough bytes for snapshot block\n", progname );
    return 1;
  }

  printf( "Found a snapshot block\n" );

  block_length = read_dword( ptr );
  printf( "  Length: %ld bytes\n", (unsigned long)block_length );

  printf( "  Flags: %d\n", read_dword( ptr ) );
  printf( "  Snapshot extension: `%s'\n", *ptr ); (*ptr) += 4;
  printf( "  Snap length: %d bytes\n", read_dword( ptr ) );

  (*ptr) += block_length - 17;

  return 0;
}

static int
read_input_block( unsigned char **ptr, unsigned char *end )
{
  size_t i, frames, length; int flags;

  if( end - *ptr < 17 ) {
    fprintf( stderr,
	     "%s: Not enough bytes for input recording block\n", progname );
    return 1;
  }

  printf( "Found an input recording block\n" );
  
  length = read_dword( ptr );
  printf( "  Length: %ld bytes\n", (unsigned long)length );

  frames = read_dword( ptr );
  printf( "  Frame count: %ld\n", (unsigned long)frames );

  printf( "  Frame length (obsolete): %d bytes\n", *(*ptr)++ );
  printf( "  Tstate counter: %d\n", read_dword( ptr ) );

  flags = read_dword( ptr );
  printf( "  Flags: %d\n", flags );

  /* Check there's enough data */
  if( end - *ptr < (off_t)length - 18 ) {
    fprintf( stderr, "%s: not enough frame data\n", progname );
    return 1;
  }

  if( flags & 0x02 ) {		/* Data is compressed */
    printf( "  Skipping compressed data\n" );
    (*ptr) += length - 18;

  } else {			/* Data is not compressed */

    for( i=0; i<frames; i++ ) {

      size_t count;

      printf( "Examining frame %ld\n", (unsigned long)i );
    
      if( end - *ptr < 4 ) {
	fprintf( stderr, "%s: Not enough data for frame %ld\n", progname,
		 (unsigned long)i );
	return 1;
      }

      printf( "  Instruction count: %d\n", read_word( ptr ) );

      count = read_word( ptr );
      if( count == 0xffff ) {
	printf( "  (Repeat last frame's INs)\n" );
	continue;
      }

      printf( "  IN count: %ld\n", (unsigned long)count );

      if( end - *ptr < (ptrdiff_t)count ) {
	fprintf( stderr,
		 "%s: Not enough data for frame %ld (expected %ld bytes)\n",
		 progname, (unsigned long)i, (unsigned long)count );
	return 1;
      }

      (*ptr) += count;
    }

  }

  return 0;
}
