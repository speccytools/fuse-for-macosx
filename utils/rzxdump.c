/* rzxdump.c: get info on an RZX2 file */
/* $Id$ */

#include <config.h>

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "types.h"
#include "ui/ui.h"

char *progname;

const char *rzx_signature = "RZX!";

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

  if( end - ptr < strlen( rzx_signature ) + 6 ) {
    fprintf( stderr,
	     "%s: Not enough bytes for RZX header (%d bytes)\n",
	     progname, strlen( rzx_signature ) + 6 );
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

  length = read_dword( ptr ); printf( "  Length: %d bytes\n", length );
  printf( "  Creator: `%s'\n", *ptr ); (*ptr) += 20;
  printf( "  Creator major version: %d\n", read_word( ptr ) );
  printf( "  Creator minor version: %d\n", read_word( ptr ) );
  printf( "  Creator custom data: %d bytes\n", length - 29 );
  (*ptr) += length - 29;

  return 0;
}

static int
read_snapshot_block( unsigned char **ptr, unsigned char *end )
{
  size_t snap_length;

  if( end - *ptr < 16 ) {
    fprintf( stderr,
	     "%s: Not enough bytes for snapshot block\n", progname );
    return 1;
  }

  printf( "Found a snapshot block\n" );
  printf( "  Length: %d bytes\n", read_dword( ptr ) );
  printf( "  Flags: %d\n", read_dword( ptr ) );
  printf( "  Snapshot extension: `%s'\n", *ptr ); (*ptr) += 4;

  snap_length = read_dword( ptr );
  printf( "  Snap length: %d bytes\n", snap_length );

  if( end - *ptr < snap_length ) {
    fprintf( stderr,
	     "%s: Not enough bytes for snapshot\n", progname );
    return 1;
  }
  (*ptr) += snap_length;

  return 0;
}

static int
read_input_block( unsigned char **ptr, unsigned char *end )
{
  size_t i, frames;

  if( end - *ptr < 17 ) {
    fprintf( stderr,
	     "%s: Not enough bytes for input recording block\n", progname );
    return 1;
  }

  printf( "Found an input recording block\n" );
  printf( "  Length: %d bytes\n", read_dword( ptr ) );

  frames = read_dword( ptr );
  printf( "  Frame count: %d\n", frames );

  printf( "  Frame length (obsolete): %d bytes\n", *(*ptr)++ );
  printf( "  Tstate counter: %d\n", read_dword( ptr ) );
  printf( "  Flags: %d\n", read_dword( ptr ) );

  for( i=0; i<frames; i++ ) {

    size_t count;

    printf( "Examining frame %d\n", i );
    
    if( end - *ptr < 8 ) {
      fprintf( stderr, "%s: Not enough data for frame %d\n", progname, i );
      return 1;
    }

    printf( "  Instruction count: %d\n", read_word( ptr ) );

    count = read_word( ptr );
    printf( "  IN count: %d\n", count );

    if( end - *ptr < count ) {
      fprintf( stderr,
	       "%s: Not enough data for frame %d (expected %d bytes)\n",
	       progname, i, count );
      return 1;
    }

    (*ptr) += count;
  }

  return 0;
}
