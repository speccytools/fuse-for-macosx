/* tzxconf.c: Convert .tzx files to .tap files
   Copyright (c) 2002 Philip Kendall

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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libspectrum.h>

char *progname;

int
main( int argc, char **argv )
{
  int fd; struct stat file_info;
  libspectrum_byte *buffer; size_t length;

  libspectrum_tape tzx;

  progname = argv[0];

  if( argc < 2 ) {
    fprintf( stderr, "%s: no .tzx file given\n", progname );
    return 1;
  }

  /* Don't screw up people's terminals */
  if( isatty( STDOUT_FILENO ) ) {
    fprintf( stderr, "%s: won't output binary data to a terminal\n",
	     progname );
    return 1;
  }

  /* Show errors from libspectrum */
  libspectrum_show_errors = 1;

  if( ( fd = open( argv[1], O_RDONLY ) ) == -1 ) {
    fprintf( stderr, "%s: couldn't open `%s': %s\n", progname, argv[1],
	     strerror( errno ) );
    return 1;
  }

  if( fstat( fd, &file_info) ) {
    fprintf( stderr, "%s: couldn't stat `%s': %s\n", progname, argv[1],
	     strerror( errno ) );
    close(fd);
    return 1;
  }

  length = file_info.st_size;

  buffer = mmap( 0, length, PROT_READ, MAP_SHARED, fd, 0 );
  if( buffer == (void*)-1 ) {
    fprintf( stderr, "%s: couldn't mmap `%s': %s\n", progname, argv[1],
	     strerror( errno ) );
    close(fd);
    return 1;
  }

  if( close(fd) ) {
    fprintf( stderr, "%s: couldn't close `%s': %s\n", progname, argv[1],
	     strerror( errno ) );
    munmap( buffer, length );
    return 1;
  }

  tzx.blocks = NULL;
  if( libspectrum_tzx_create( &tzx, buffer, length ) ) {
    munmap( buffer, length );
    return 1;
  }

  if( munmap( buffer, length ) == -1 ) {
    fprintf( stderr, "%s: couldn't munmap `%s': %s\n", progname, argv[1],
	     strerror( errno ) );
    libspectrum_tape_free( &tzx );
    return 1;
  }

  length = 0;
  if( libspectrum_tap_write( &tzx, &buffer, &length ) ) {
    libspectrum_tape_free( &tzx );
    return 1;
  }

  if( fwrite( buffer, 1, length, stdout ) != length ) {
    free( buffer ); libspectrum_tape_free( &tzx );
    return 1;
  }

  free( buffer ); libspectrum_tape_free( &tzx );

  return 0;
}
