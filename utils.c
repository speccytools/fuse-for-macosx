/* utils.c: some useful helper functions
   Copyright (c) 1999-2003 Philip Kendall

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
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <ui/ui.h>
#include <unistd.h>

#include <libspectrum.h>

#include "dck.h"
#include "fuse.h"
#include "rzx.h"
#include "snapshot.h"
#include "specplus3.h"
#include "tape.h"
#include "trdos.h"
#include "utils.h"

#define PATHNAME_MAX_LENGTH 1024

/* Open `filename' and do something sensible with it; autoload tapes
   if `autoload' is true and return the type of file found in `type' */
int
utils_open_file( const char *filename, int autoload,
		 libspectrum_id_t *type_ptr)
{
  utils_file file;
  libspectrum_id_t type;
  libspectrum_class_t class;
  int error;

  /* Read the file into a buffer */
  if( utils_read_file( filename, &file ) ) return 1;

  /* See if we can work out what it is */
  if( libspectrum_identify_file_with_class( &type, &class, filename,
					    file.buffer, file.length ) ) {
    utils_close_file( &file );
    return 1;
  }

  error = 0;

  switch( class ) {
    
  case LIBSPECTRUM_CLASS_UNKNOWN:
    ui_error( UI_ERROR_ERROR, "utils_open_file: couldn't identify `%s'",
	      filename );
    utils_close_file( &file );
    return 1;

  case LIBSPECTRUM_CLASS_RECORDING:
    error = rzx_start_playback_from_buffer( file.buffer, file.length );
    break;

  case LIBSPECTRUM_CLASS_SNAPSHOT:
    error = snapshot_read_buffer( file.buffer, file.length, type );
    break;

  case LIBSPECTRUM_CLASS_TAPE:
    error = tape_read_buffer( file.buffer, file.length, type, autoload );
    break;

  case LIBSPECTRUM_CLASS_DISK_PLUS3:
#ifdef HAVE_765_H
    error = machine_select( LIBSPECTRUM_MACHINE_PLUS3 ); if( error ) break;
    error = specplus3_disk_insert( SPECPLUS3_DRIVE_A, filename );
    if( autoload ) {

      int fd; utils_file snap;

      fd = utils_find_lib( "disk_plus3.z80" );
      if( fd == -1 ) {
	ui_error( UI_ERROR_ERROR, "Couldn't find +3 disk autoload snap" );
	error = 1;
	goto escape;
      }

      error = utils_read_fd( fd, "disk_plus3.z80", &snap );
      if( error ) { utils_close_file( &snap ); goto escape; }

      error = snapshot_read_buffer( snap.buffer, snap.length,
				    LIBSPECTRUM_ID_SNAPSHOT_Z80 );
      if( error ) { utils_close_file( &snap ); goto escape; }

      if( utils_close_file( &snap ) ) {
	ui_error( UI_ERROR_ERROR, "Couldn't munmap 'disk_plus3.z80': %s",
		  strerror( errno ) );
	error = 1;
	goto escape;
      }
    }
  escape:
    break;

#else				/* #ifdef HAVE_765_H */
    ui_error( UI_ERROR_INFO, "lib765 not present so can't handle .dsk files" );
#endif				/* #ifdef HAVE_765_H */
    break;

  case LIBSPECTRUM_CLASS_DISK_TRDOS:
    error = machine_select( LIBSPECTRUM_MACHINE_PENT ); if( error ) break;
    error = trdos_disk_insert( TRDOS_DRIVE_A, filename ); if( error ) break;
    if( autoload ) {
      machine_current->ram.current_rom = 1;
      trdos_active = 1;
    }
    break;

  case LIBSPECTRUM_CLASS_CARTRIDGE_TIMEX:
    error = machine_select( LIBSPECTRUM_MACHINE_TC2068 ); if( error ) break;
    error = dck_read( filename );
    break;

  default:
    ui_error( UI_ERROR_ERROR, "utils_open_file: unknown class %d", type );
    error = 1;
    break;
  }

  if( error ) { utils_close_file( &file ); return error; }

  if( utils_close_file( &file ) ) return 1;

  if( type_ptr ) *type_ptr = type;

  return 0;
}

/* Find the auxillary file called `filename'; look in the lib
   directory below where the Fuse executable is and the defined
   package data directory; returns a fd for the file on success, -1 if
   it couldn't find the file */
int utils_find_lib( const char *filename )
{
  int fd;

  char fuse_path[ PATHNAME_MAX_LENGTH ], path[ PATHNAME_MAX_LENGTH ];
  char *fuse_dir;

  if( fuse_progname[0] == '/' ) {
    strncpy( fuse_path, fuse_progname, PATHNAME_MAX_LENGTH );
  } else {
    strncpy( fuse_path, fuse_directory, PATHNAME_MAX_LENGTH );
    strncat( fuse_path, fuse_progname, PATHNAME_MAX_LENGTH );
  }
  fuse_dir = dirname( fuse_path );

  snprintf( path, PATHNAME_MAX_LENGTH, "%s/lib/%s", fuse_dir, filename );
  fd = open( path, O_RDONLY | O_BINARY );
  if( fd != -1 ) return fd;

  snprintf( path, PATHNAME_MAX_LENGTH, "%s/%s", FUSEDATADIR, filename );
  fd = open( path, O_RDONLY | O_BINARY );
  if( fd != -1 ) return fd;

  return -1;
}

int
utils_read_file( const char *filename, utils_file *file )
{
  int fd;

  int error;

  fd = open( filename, O_RDONLY | O_BINARY );
  if( fd == -1 ) {
    ui_error( UI_ERROR_ERROR, "couldn't open '%s': %s", filename,
	      strerror( errno ) );
    return 1;
  }

  error = utils_read_fd( fd, filename, file );
  if( error ) return error;

  return 0;
}

int
utils_read_fd( int fd, const char *filename, utils_file *file )
{
  struct stat file_info;

  if( fstat( fd, &file_info) ) {
    ui_error( UI_ERROR_ERROR, "Couldn't stat '%s': %s", filename,
	      strerror( errno ) );
    close(fd);
    return 1;
  }

  file->length = file_info.st_size;

#ifdef HAVE_MMAP

  file->buffer = mmap( 0, file->length, PROT_READ, MAP_SHARED, fd, 0 );

  if( file->buffer != (void*)-1 ) {

    file->mode = UTILS_FILE_OPEN_MMAP;

    if( close(fd) ) {
      ui_error( UI_ERROR_ERROR, "Couldn't close '%s': %s", filename,
		strerror( errno ) );
      munmap( file->buffer, file->length );
      return 1;
    }

    return 0;
  }

#endif			/* #ifdef HAVE_MMAP */

  /* Either mmap() isn't available, or it failed for some reason so try
     using normal IO */
  file->mode = UTILS_FILE_OPEN_MALLOC;

  file->buffer = malloc( file->length );
  if( !file->buffer ) {
    ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d\n", __FILE__, __LINE__ );
    return 1;
  }

  if( read( fd, file->buffer, file->length ) != file->length ) {
    ui_error( UI_ERROR_ERROR, "Error reading from '%s': %s", filename,
	      strerror( errno ) );
    free( file->buffer );
    close( fd );
    return 1;
  }

  if( close(fd) ) {
    ui_error( UI_ERROR_ERROR, "Couldn't close '%s': %s", filename,
	      strerror( errno ) );
    free( file->buffer );
    return 1;
  }

  return 0;
}

int
utils_close_file( utils_file *file )
{
  switch( file->mode ) {

  case UTILS_FILE_OPEN_MMAP:
#ifdef HAVE_MMAP
    if( file->length ) {
      if( munmap( file->buffer, file->length ) ) {
	ui_error( UI_ERROR_ERROR, "Couldn't munmap: %s\n", strerror( errno ) );
	return 1;
      }
    }
#else				/* #ifdef HAVE_MMAP */
    ui_error( UI_ERROR_ERROR, "utils_close_file: file->mode == UTILS_FILE_OPEN_MMAP, but mmap not available?!" );
    fuse_abort();
#endif
    break;

  case UTILS_FILE_OPEN_MALLOC:
    free( file->buffer );
    break;

  default:
    ui_error( UI_ERROR_ERROR, "Unknown file open mode %d\n", file->mode );
    fuse_abort();

  }

  return 0;
}

int utils_write_file( const char *filename, const unsigned char *buffer,
		      size_t length )
{
  FILE *f;

  f=fopen( filename, "wb" );
  if(!f) { 
    ui_error( UI_ERROR_ERROR, "error opening '%s': %s", filename,
	      strerror( errno ) );
    return 1;
  }
	    
  if( fwrite( buffer, 1, length, f ) != length ) {
    ui_error( UI_ERROR_ERROR, "error writing to '%s': %s", filename,
	      strerror( errno ) );
    fclose(f);
    return 1;
  }

  if( fclose( f ) ) {
    ui_error( UI_ERROR_ERROR, "error closing '%s': %s", filename,
	      strerror( errno ) );
    return 1;
  }

  return 0;
}

/* Get a path where we can reasonably create temporary files */
const char*
utils_get_temp_path( void )
{
  const char *dir;

#ifdef WIN32

  /* Something close to this algorithm specified at
     http://msdn.microsoft.com/library/default.asp?url=/library/en-us/fileio/base/gettemppath.asp
  */
  dir = getenv( "TMP" ); if( dir ) return dir;
  dir = getenv( "TEMP" ); if( dir ) return dir;
  return ".";

#else				/* #ifdef WIN32 */

  /* Unix-ish. Use TMPDIR if specified, if not /tmp */
  dir = getenv( "TMPDIR" ); if( dir ) return dir;
  return "/tmp";

#endif				/* #ifdef WIN32 */
  
}

/* Return the path where we will store our configuration information etc */
const char*
utils_get_home_path( void )
{
  const char *dir;

#ifdef WIN32

  dir = getenv( "USERPROFILE" ); if( dir ) return dir;
  dir = getenv( "WINDIR" ); if( dir ) return dir;

#else				/* #ifdef WIN32 */

  dir = getenv( "HOME" ); if( dir ) return dir;

#endif				/* #ifdef WIN32 */

  ui_error( UI_ERROR_ERROR, "couldn't find a plausible home directory" );
  return NULL;
}
