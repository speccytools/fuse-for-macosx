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

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fuse.h"
#include "utils.h"

#define ERROR_MESSAGE_MAX_LENGTH 1024
#define PATHNAME_MAX_LENGTH 1024

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

  int error; char error_message[ ERROR_MESSAGE_MAX_LENGTH ];

  fd = open( filename, O_RDONLY );
  if( fd == -1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: couldn't open `%s'", fuse_progname, filename );
    perror( error_message );
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

  char error_message[ ERROR_MESSAGE_MAX_LENGTH ];

  if( fstat( fd, &file_info) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't stat `%s'", fuse_progname, filename );
    perror( error_message );
    close(fd);
    return 1;
  }

  *length = file_info.st_size;

  (*buffer) = mmap( 0, *length, PROT_READ, MAP_SHARED, fd, 0 );
  if( *buffer == (void*)-1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't mmap `%s'", fuse_progname, filename );
    perror( error_message );
    close(fd);
    return 1;
  }

  if( close(fd) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't close `%s'", fuse_progname, filename );
    perror( error_message );
    munmap( *buffer, *length );
    return 1;
  }

  return 0;
}

int utils_write_file( const char *filename, const unsigned char *buffer,
		      size_t length )
{
  FILE *f;

  char error_message[ ERROR_MESSAGE_MAX_LENGTH ];

  f=fopen( filename, "wb" );
  if(!f) { 
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: error opening `%s'", fuse_progname, filename );
    perror( error_message );
    return 1;
  }
	    
  if( fwrite( buffer, 1, length, f ) != length ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: error writing to `%s'", fuse_progname, filename );
    perror( error_message );
    fclose(f);
    return 1;
  }

  if( fclose( f ) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: error closing `%s'", fuse_progname, filename );
    perror( error_message );
    return 1;
  }

  return 0;
}


