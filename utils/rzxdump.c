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

const char *rzx2_signature = "RZX2";

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

int do_file( const char *filename )
{
  int fd; struct stat file_info;

  unsigned char *buffer, *ptr, *end;

  size_t i, frames;

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

  if( end - ptr < strlen( rzx2_signature ) + 6 ) {
    fprintf( stderr,
	     "%s: Not enough bytes for RZX2 header (%d bytes)\n",
	     progname, strlen( rzx2_signature ) + 6 );
    munmap( buffer, file_info.st_size );
    return 1;
  }

  if( memcmp( ptr, rzx2_signature, strlen( rzx2_signature ) ) ) {
    fprintf( stderr, "%s: Wrong signature: expected `%s'\n", progname,
	     rzx2_signature );
    munmap( buffer, file_info.st_size );
    return 1;
  }

  printf( "Found RZX2 signature\n" );
  ptr += strlen( rzx2_signature );

  printf( "  Major version: %d\n", (int)*ptr++ );
  printf( "  Minor version: %d\n", (int)*ptr++ );
  printf( "  Flags: %d\n", read_dword( &ptr ) );

  /* Read the input recording block */

  if( end - ptr < 18 ) {
    fprintf( stderr,
	     "%s: Not enough bytes for input recording block (%d bytes)\n",
	     progname, 18 );
    munmap( buffer, file_info.st_size );
    return 1;
  }

  if( *ptr != 0x80 ) {
    fprintf( stderr,
	     "%s: Got ID 0x%02x, not input recording block (0x80)\n",
	     progname, *ptr );
    munmap( buffer, file_info.st_size );
    return 1;
  }
  ptr++;

  printf( "Found an input recording block\n" );
  printf( "  Length: %d bytes\n", read_dword( &ptr ) );
  printf( "  Frame length (obsolete): %d bytes\n", *ptr++ );

  frames = read_dword( &ptr );
  printf( "  Frame count: %d\n", frames );
  printf( "  Tstate counter: %d\n", read_dword( &ptr ) );
  printf( "  Flags: %d\n", read_dword( &ptr ) );

  for( i=0; i<frames; i++ ) {

    size_t count;

    printf( "Examining frame %d\n", i );
    
    if( end - ptr < 8 ) {
      fprintf( stderr, "%s: Not enough data for frame %d\n", progname, i );
      munmap( buffer, file_info.st_size );
      return 1;
    }

    printf( "  Instruction count: %d\n", read_word( &ptr ) );

    count = read_word( &ptr );
    printf( "  IN count: %d\n", count );

    if( end - ptr < count ) {
      fprintf( stderr,
	       "%s: Not enough data for frame %d (expected %d bytes)\n",
	       progname, i, count );
      munmap( buffer, file_info.st_size );
      return 1;
    }

    ptr += count;
  }

  if( munmap( buffer, file_info.st_size ) == -1 ) {
    fprintf( stderr, "%s: couldn't munmap `%s': %s\n", progname, filename,
	     strerror( errno ) );
    return 1;
  }

  return 0;
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

