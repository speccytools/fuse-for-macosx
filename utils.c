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

#include "fuse.h"
#include "utils.h"

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

/* Make a 'best guess' as to what sort of file this is */
int
utils_identify_file( int *type, const char *filename,
		     const unsigned char *buffer, size_t length )
{
  struct type {

    int type;

    char *extension;

    char *signature; size_t offset, length; int sig_score;
  };

  struct type *ptr,
    types[] = {
      
      { UTILS_TYPE_RECORDING_RZX, "rzx", "RZX!",     0, 4, 3 },

      { UTILS_TYPE_SNAPSHOT_SNA,  "sna", NULL,       0, 0, 0 },
      { UTILS_TYPE_SNAPSHOT_Z80,  "z80", "\0\0",     6, 2, 1 },

      { UTILS_TYPE_TAPE_TAP,      "tap", "\x13\0\0", 0, 3, 1 },
      { UTILS_TYPE_TAPE_TZX,      "tzx", "ZXTape!",  0, 7, 3 },

      { -1, NULL, NULL, 0, 0, 0 }, /* End marker */

    };

  const char *extension;
  int score, best_score, best_guess, duplicate_best;

  /* Get the filename extension, if it exists */
  extension = strrchr( filename, '.' ); if( extension ) extension++;

  best_guess = UTILS_TYPE_UNKNOWN; best_score = 0; duplicate_best = 0;

  /* Compare against known extensions and signatures */
  for( ptr = types; ptr->type != -1; ptr++ ) {

    score = 0;

    if( extension && ptr->extension &&
	!strcasecmp( extension, ptr->extension ) )
      score += 2;

    if( ptr->signature && length >= ptr->offset + ptr->length &&
	!memcmp( &buffer[ ptr->offset ], ptr->signature, ptr->length ) )
      score += ptr->sig_score;

    if( score > best_score ) {
      best_guess = ptr->type; best_score = score; duplicate_best = 0;
    } else if( score == best_score ) {
      duplicate_best = 1;
    }
  }

  /* If two things were equally good, we can't identify this. Otherwise,
     return our best guess */
  if( duplicate_best ) {
    *type = UTILS_TYPE_UNKNOWN;
  } else {
    *type = best_guess;
  }

  return 0;
}
