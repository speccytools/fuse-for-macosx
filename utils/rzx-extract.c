/* rzx-extract.c: Extract the snapshot from a RZX file
   Copyright (c) 2002 Philip Kendalla

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

#include "../libspectrum/libspectrum.h"
#include "../libspectrum/rzx.h"

int main( int argc, char **argv )
{
  const char *progname, *filename;
  int fd; struct stat file_info;

  unsigned char *buffer; size_t length;

  libspectrum_rzx rzx;
  libspectrum_snap *snap;

  progname = argv[0];

  if( argc != 2 ) {
    fprintf( stderr, "%s: Usage: %s <rzxfile>\n", progname, progname );
    return 1;
  }

  /* Show errors from libspectrum */
  libspectrum_show_errors = 1;

  filename = argv[1];

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
  
  snap = NULL;

  if( libspectrum_rzx_read( &rzx, buffer, file_info.st_size, &snap ) ) {
    munmap( buffer, file_info.st_size );
    return 1;
  }

  if( munmap( buffer, file_info.st_size ) == -1 ) {
    fprintf( stderr, "%s: couldn't munmap `%s': %s\n", progname, filename,
	     strerror( errno ) );
    if( snap ) { libspectrum_snap_destroy( snap ); free( snap ); }
    return 1;
  }

  /* Don't want the RZX data */
  if( libspectrum_rzx_free( &rzx ) ) {
    if( snap ) { libspectrum_snap_destroy( snap ); free( snap ); }
    return 1;
  }

  /* If no snapshot, can't do anything useful */
  if( !snap ) {
    printf( "%s: no snapshot found in `%s'\n", progname, filename );
    return 0;
  }
  
  length = 0;		/* Allocate me some space, please */
  if( libspectrum_z80_write( &buffer, &length, snap ) ) {
    libspectrum_snap_destroy( snap ); free( snap );
    return 1;
  }

  /* Now finished with the snapshot structure */
  if( libspectrum_snap_destroy( snap ) ) { free( buffer ); return 1; }
  free( snap );

  if( fwrite( buffer, 1, length, stdout ) != length ) {
    fprintf( stderr, "%s: error writing output: %s\n", progname,
	     strerror( errno ) );
    free( buffer );
    return 1;
  }

  free( buffer );

  return 0;
}
