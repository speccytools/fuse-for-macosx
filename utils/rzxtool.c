/* rzxtool.c: (Un)compress RZX data and add, remove or extract embedded snaps
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

#include "../libspectrum/libspectrum.h"
#include "../libspectrum/rzx.h"

struct options {

  char *rzxfile;		/* The RZX file we'll operate on */

  char *add;			/* The snapshot to add */
  int remove;			/* Shall we remove the existing snap? */

  int uncompress;		/* Shall we uncompress the data? */

  int extract;			/* Shall we extract the snapshot? */

};

char *progname;			/* argv[0] */

void init_options( struct options *options );
int parse_options( int argc, char **argv, struct options *options );
int mmap_file( const char *filename, unsigned char **buffer, size_t *length );
int write_snapshot( libspectrum_snap *snap );
int make_snapshot( unsigned char **buffer, size_t *length,
		   libspectrum_snap *snap );

int
main( int argc, char **argv )
{
  unsigned char *buffer; size_t length;
  unsigned char *snap_buffer = NULL; size_t snap_length;

  libspectrum_rzx rzx;
  libspectrum_snap *snap = NULL;

  struct options options;

  progname = argv[0];

  init_options( &options );
  if( parse_options( argc, argv, &options ) ) return 1;

  /* Don't screw up people's terminals */
  if( isatty( STDOUT_FILENO ) ) {
    fprintf( stderr, "%s: won't output binary data to a terminal\n",
	     progname );
    return 1;
  }

  /* Show errors from libspectrum */
  libspectrum_show_errors = 1;

  if( mmap_file( options.rzxfile, &buffer, &length ) ) return 1;

  if( libspectrum_rzx_read( &rzx, buffer, length, &snap ) ) {
    munmap( buffer, length );
    return 1;
  }

  if( munmap( buffer, length ) == -1 ) {
    fprintf( stderr, "%s: couldn't munmap `%s': %s\n", progname,
	     options.rzxfile, strerror( errno ) );
    if( snap ) { libspectrum_snap_destroy( snap ); free( snap ); }
    return 1;
  }

  if( options.extract ) {

    if( !snap ) {
      fprintf( stderr, "%s: no snapshot in `%s' to extract\n", progname,
	       options.rzxfile );
      libspectrum_rzx_free( &rzx );
      return 1;
    }
    if( write_snapshot( snap ) ) {
      libspectrum_rzx_free( &rzx );
      libspectrum_snap_destroy( snap ); free( snap );
      return 1;
    }

  } else {

    if( options.remove ) {
      if( !snap ) {
	fprintf( stderr, "%s: warning: no snapshot to remove\n", progname );
      } else {
	libspectrum_snap_destroy( snap ); free( snap ); snap = NULL;
      }
    }

    if( options.add ) {
      
      /* Don't want the old snap anymore */
      if( snap ) {
	libspectrum_snap_destroy( snap ); free( snap );
	snap = NULL;
      }

      /* Get the new snap */
      if( mmap_file( options.add, &snap_buffer, &snap_length ) ) {
	libspectrum_rzx_free( &rzx );
	return 1;
      }
      
      /* If the file has a ".sna" extension, assume it's an .sna file. If not,
	 assume it's a .z80 file */
      if(    strlen( options.add ) < 4
	  || strncasecmp( &options.add[ strlen(options.add) - 4 ], ".sna", 4 )
	) {
	
	if( libspectrum_z80_read( snap_buffer, snap_length, snap ) ) {
	  munmap( snap_buffer, snap_length );
	  libspectrum_rzx_free( &rzx );
	  return 1;
	}
	
      } else {

	if( libspectrum_sna_read( snap_buffer, snap_length, snap ) ) {
	  munmap( snap_buffer, snap_length );
	  libspectrum_rzx_free( &rzx );
	  return 1;
	}

      }

      /* Now done with this buffer */
      if( munmap( snap_buffer, snap_length ) ) {
	libspectrum_snap_destroy( snap ); free( snap );
	libspectrum_rzx_free( &rzx );
	return 1;
      }

    }      
    
    /* Reserialise the snapshot */
    if( snap ) {
      snap_length = 0;
      if( libspectrum_z80_write( &snap_buffer, &snap_length, snap ) ) {
	libspectrum_snap_destroy( snap ); free( snap );
	libspectrum_rzx_free( &rzx );
	return 1;
      }
    }

    /* Serialise the RZX file */
    length = 0;
    if( libspectrum_rzx_write( &rzx, &buffer, &length, snap_buffer,
			       snap_length, "rzxtool", 0, 1,
			       !options.uncompress ) ) {
      free( snap_buffer );
      libspectrum_rzx_free( &rzx );
      return 1;
    }

    free( snap_buffer );

    /* And (finally!) write it */
    if( fwrite( buffer, 1, length, stdout ) != length ) {
      free( buffer );
      libspectrum_rzx_free( &rzx );
      return 1;
    }

    free( buffer );

  }

  /* Tidy up */
  libspectrum_rzx_free( &rzx );
  if( snap ) { libspectrum_snap_destroy( snap ); free( snap ); }

  return 0;
}

void
init_options( struct options *options )
{
  options->rzxfile = NULL;

  options->add = NULL;
  options->remove = 0;
  options->uncompress = 0;
  options->extract = 0;
}

int
parse_options( int argc, char **argv, struct options *options )
{
  /* Defined by getopt */
  extern char *optarg;
  extern int optind;

  int c;

  int unknown = 0, output_rzx = 0, output_snapshot = 0;

  while( ( c = getopt( argc, argv, "a:ceru" ) ) != EOF ) 
    switch( c ) {
    case 'a': options->add = optarg;   break;
    case 'e': options->extract = 1;    break;
    case 'r': options->remove = 1;     break;
    case 'u': options->uncompress = 1; break;
    case '?': unknown = c;	       break;
    }

  if( unknown ) {
    fprintf( stderr, "%s: unknown option `%c'\n", progname, (char)c );
    return 1;
  }

  if( options->add || options->remove || options->uncompress ) output_rzx = 1;
  if( options->extract ) output_snapshot = 1;

  if( output_rzx && output_snapshot ) {
    fprintf( stderr, "%s: can't output both a snapshot and a RZX file\n",
	     progname );
    return 1;
  }

  if( argv[optind] == NULL ) {
    fprintf( stderr, "%s: no RZX file given\n", progname );
    return 1;
  }

  options->rzxfile = argv[optind];

  return 0;
}

int
mmap_file( const char *filename, unsigned char **buffer, size_t *length )
{
  int fd; struct stat file_info;
  
  if( ( fd = open( filename, O_RDONLY ) ) == -1 ) {
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

  (*length) = file_info.st_size;

  (*buffer) = mmap( 0, *length, PROT_READ, MAP_SHARED, fd, 0 );
  if( buffer == (void*)-1 ) {
    fprintf( stderr, "%s: couldn't mmap `%s': %s\n", progname, filename,
	     strerror( errno ) );
    close(fd);
    return 1;
  }

  if( close(fd) ) {
    fprintf( stderr, "%s: couldn't close `%s': %s\n", progname, filename,
	     strerror( errno ) );
    munmap( *buffer, *length );
    return 1;
  }
  
  return 0;
}

int
write_snapshot( libspectrum_snap *snap )
{
  unsigned char *buffer; size_t length = 0;

  if( libspectrum_z80_write( &buffer, &length, snap ) ) return 1;
  
  if( fwrite( buffer, 1, length, stdout ) != length ) {
    fprintf( stderr, "%s: error writing output: %s\n", progname,
	     strerror( errno ) );
    free( buffer );
    return 1;
  }

  free( buffer );

  return 0;
}
