/* utils.c: some useful helper functions
   Copyright (c) 1999-2002 Philip Kendall

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
  unsigned char *buffer; size_t length;
  libspectrum_id_t type;
  int error = 0;

  /* Read the file into a buffer */
  if( utils_read_file( filename, &buffer, &length ) ) return 1;

  /* See if we can work out what it is */
  if( libspectrum_identify_file( &type, filename, buffer, length ) ) {
    munmap( buffer, length );
    return 1;
  }

  switch( type ) {
    
  case LIBSPECTRUM_ID_UNKNOWN:
    ui_error( UI_ERROR_ERROR, "utils_open_file: couldn't identify `%s'",
	      filename );
    munmap( buffer, length );
    return 1;

  case LIBSPECTRUM_ID_RECORDING_RZX:
    error = rzx_start_playback_from_buffer( buffer, length );
    break;

  case LIBSPECTRUM_ID_SNAPSHOT_SNA:
  case LIBSPECTRUM_ID_SNAPSHOT_Z80:
    error = snapshot_read_buffer( buffer, length, type );
    break;

  case LIBSPECTRUM_ID_TAPE_TAP:
  case LIBSPECTRUM_ID_TAPE_TZX:
  case LIBSPECTRUM_ID_TAPE_WARAJEVO:
    error = tape_read_buffer( buffer, length, type, autoload );
    break;

  case LIBSPECTRUM_ID_DISK_DSK:
#ifdef HAVE_765_H
    error = machine_select( LIBSPECTRUM_MACHINE_PLUS3 ); if( error ) break;
    error = specplus3_disk_insert( SPECPLUS3_DRIVE_A, filename );
#else				/* #ifdef HAVE_765_H */
    ui_error( UI_ERROR_INFO, "lib765 not present so can't handle .dsk files" );
#endif				/* #ifdef HAVE_765_H */
    break;

  case LIBSPECTRUM_ID_DISK_SCL:
  case LIBSPECTRUM_ID_DISK_TRD:
    error = machine_select( LIBSPECTRUM_MACHINE_PENT ); if( error ) break;
    error = trdos_disk_insert( TRDOS_DRIVE_A, filename );
    break;

  case LIBSPECTRUM_ID_CARTRIDGE_DCK:
    error = machine_select( LIBSPECTRUM_MACHINE_TC2068 ); if( error ) break;
    error = dck_read( filename );
    break;

  default:
    ui_error( UI_ERROR_ERROR, "utils_open_file: unknown type %d", type );
    fuse_abort();
  }

  if( error ) { munmap( buffer, length ); return error; }

  if( length && munmap( buffer, length ) ) {
    ui_error( UI_ERROR_ERROR, "utils_open_file: couldn't munmap `%s': %s",
	      filename, strerror( errno ) );
    return 1;
  }

  if( type_ptr ) *type_ptr = type;

  return 0;
}

/* Find a ROM called `filename'; look in the current directory, ./roms
   and the defined roms directory; returns a fd for the ROM on success,
   -1 if it couldn't find the ROM */
int utils_find_lib( const char *filename )
{
  int fd;

  char path[ PATHNAME_MAX_LENGTH ];

  snprintf( path, PATHNAME_MAX_LENGTH, "lib/%s", filename );
  fd = open( path, O_RDONLY );
  if( fd != -1 ) return fd;

  snprintf( path, PATHNAME_MAX_LENGTH, "%s/%s", DATADIR, filename );
  fd = open( path, O_RDONLY );
  if( fd != -1 ) return fd;

  return -1;
}

int utils_read_file( const char *filename, unsigned char **buffer,
		     size_t *length )
{
  int fd;

  int error;

  fd = open( filename, O_RDONLY );
  if( fd == -1 ) {
    ui_error( UI_ERROR_ERROR, "couldn't open '%s': %s", filename,
	      strerror( errno ) );
    return 1;
  }

  error = utils_read_fd( fd, filename, buffer, length );
  if( error ) return error;

  return 0;
}

int utils_read_fd( int fd, const char *filename,
		   unsigned char **buffer, size_t *length )
{
  struct stat file_info;

  if( fstat( fd, &file_info) ) {
    ui_error( UI_ERROR_ERROR, "Couldn't stat '%s': %s", filename,
	      strerror( errno ) );
    close(fd);
    return 1;
  }

  *length = file_info.st_size;

  (*buffer) = mmap( 0, *length, PROT_READ, MAP_SHARED, fd, 0 );
  if( *buffer == (void*)-1 ) {
    ui_error( UI_ERROR_ERROR, "Couldn't mmap '%s': %s", filename,
	      strerror( errno ) );
    close(fd);
    return 1;
  }

  if( close(fd) ) {
    ui_error( UI_ERROR_ERROR, "Couldn't close '%s': %s", filename,
	      strerror( errno ) );
    munmap( *buffer, *length );
    return 1;
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
