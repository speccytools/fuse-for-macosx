/* utils.c: some useful helper functions
   Copyright (c) 1999-2005 Philip Kendall

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif				/* #ifdef HAVE_LIBGEN_H */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ui/ui.h>
#include <unistd.h>

#include <libspectrum.h>

#include "dck.h"
#include "disk/beta.h"
#include "divide.h"
#include "fuse.h"
#include "if1.h"
#include "if2.h"
#include "machines/specplus3.h"
#include "memory.h"
#include "rzx.h"
#include "settings.h"
#include "simpleide.h"
#include "snapshot.h"
#include "tape.h"
#include "utils.h"
#include "zxatasp.h"
#include "zxcf.h"

typedef struct path_context {

  int state;

  utils_aux_type type;
  char path[ PATH_MAX ];

} path_context;

static void init_path_context( path_context *ctx, utils_aux_type type );
static int get_next_path( path_context *ctx );

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
    error = tape_read_buffer( file.buffer, file.length, type, filename,
			      autoload );
    break;

  case LIBSPECTRUM_CLASS_DISK_PLUS3:
#ifdef HAVE_765_H

    if( !( machine_current->capabilities &
	   LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_DISK ) ) {
      error = machine_select( LIBSPECTRUM_MACHINE_PLUS3 ); if( error ) break;
    }

    error = specplus3_disk_insert( SPECPLUS3_DRIVE_A, filename, autoload );
    break;

#else				/* #ifdef HAVE_765_H */
    ui_error( UI_ERROR_WARNING,
	      "lib765 not present so can't handle .dsk files" );
#endif				/* #ifdef HAVE_765_H */
    break;

  case LIBSPECTRUM_CLASS_DISK_PLUSD:

    error = plusd_disk_insert( PLUSD_DRIVE_1, filename, autoload );
    break;

  case LIBSPECTRUM_CLASS_DISK_TRDOS:

    if( !( machine_current->capabilities &
	   LIBSPECTRUM_MACHINE_CAPABILITY_TRDOS_DISK ) &&
        !periph_beta128_active ) {
      error = machine_select( LIBSPECTRUM_MACHINE_PENT ); if( error ) break;
    }

    error = beta_disk_insert( BETA_DRIVE_A, filename, autoload );
    break;

  case LIBSPECTRUM_CLASS_CARTRIDGE_IF2:
    error = if2_insert( filename );
    break;

  case LIBSPECTRUM_CLASS_MICRODRIVE:
    error = if1_mdr_insert( -1, filename );
    break;

  case LIBSPECTRUM_CLASS_CARTRIDGE_TIMEX:
    error = machine_select( LIBSPECTRUM_MACHINE_TC2068 ); if( error ) break;
    error = dck_insert( filename );
    break;

  case LIBSPECTRUM_CLASS_HARDDISK:
    if( !settings_current.simpleide_active &&
	!settings_current.zxatasp_active   &&
	!settings_current.divide_enabled   &&
	!settings_current.zxcf_active         ) {
      settings_current.zxcf_active = 1;
      periph_update();
    }

    if( settings_current.zxcf_active ) {
      error = zxcf_insert( filename );
    } else if( settings_current.zxatasp_active ) {
      error = zxatasp_insert( filename, LIBSPECTRUM_IDE_MASTER );
    } else if( settings_current.simpleide_active ) {
      error = simpleide_insert( filename, LIBSPECTRUM_IDE_MASTER );
    } else {
      error = divide_insert( filename, LIBSPECTRUM_IDE_MASTER );
    }
    if( error ) return error;
    
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

/* Find the auxiliary file called `filename'; returns a fd for the
   file on success, -1 if it couldn't find the file */
int
utils_find_auxiliary_file( const char *filename, utils_aux_type type )
{
  int fd;

  char path[ PATH_MAX ];
  path_context ctx;

  /* If given an absolute path, just look there */
  if( compat_is_absolute_path( filename ) )
    return open( filename, O_RDONLY | O_BINARY );

  /* Otherwise look in some likely locations */
  init_path_context( &ctx, type );

  while( get_next_path( &ctx ) ) {
#ifdef AMIGA
    snprintf( path, PATH_MAX, "%s%s", ctx.path, filename );
#else
    snprintf( path, PATH_MAX, "%s" FUSE_DIR_SEP_STR "%s", ctx.path, filename );
#endif
    fd = open( path, O_RDONLY | O_BINARY );
    if( fd != -1 ) return fd;

  }

  /* Give up. Couldn't find this file */
  return -1;
}


int
utils_find_file_path( const char *filename, char *ret_path,
		      utils_aux_type type )
{
  path_context ctx;
  struct stat stat_info;

  /* If given an absolute path, just look there */
  if( compat_is_absolute_path( filename ) ) {
    strncpy( ret_path, filename, PATH_MAX );
    return 0;
  }

  /* Otherwise look in some likely locations */
  init_path_context( &ctx, type );

  while( get_next_path( &ctx ) ) {

#ifdef AMIGA
    snprintf( ret_path, PATH_MAX, "%s%s", ctx.path, filename );
#else
    snprintf( ret_path, PATH_MAX, "%s" FUSE_DIR_SEP_STR "%s", ctx.path,
              filename );
#endif
    if( !stat( ret_path, &stat_info ) ) return 0;

  }

  return 1;
}

/* Something similar to that described at
   http://www.chiark.greenend.org.uk/~sgtatham/coroutines.html was
   _very_ tempting here...
*/
static void
init_path_context( path_context *ctx, utils_aux_type type )
{
  ctx->state = 0;
  ctx->type = type;
}

static int
get_next_path( path_context *ctx )
{
  char buffer[ PATH_MAX ], *path_segment, *path2;

  switch( (ctx->state)++ ) {

    /* First look relative to the current directory */
  case 0:
#ifdef AMIGA
    strncpy( ctx->path, "PROGDIR:", PATH_MAX );
#else
    strncpy( ctx->path, ".", PATH_MAX );
#endif
    return 1;

    /* Then relative to the Fuse executable */
  case 1:

    switch( ctx->type ) {
#ifdef AMIGA
    case UTILS_AUXILIARY_LIB: strncpy( ctx->path, "PROGDIR:lib/", PATH_MAX); return 1;
    case UTILS_AUXILIARY_ROM: strncpy( ctx->path, "PROGDIR:roms/", PATH_MAX); return 1;
    case UTILS_AUXILIARY_WIDGET: strncpy( ctx->path, "PROGDIR:widget/", PATH_MAX); return 1;
#else
    case UTILS_AUXILIARY_LIB: path_segment = "lib"; break;
    case UTILS_AUXILIARY_ROM: path_segment = "roms"; break;
    case UTILS_AUXILIARY_WIDGET: path_segment = "widget"; break;
#endif
    default:
      ui_error( UI_ERROR_ERROR, "unknown auxiliary file type %d", ctx->type );
      return 0;
    }

    if( compat_is_absolute_path( fuse_progname ) ) {
      strncpy( buffer, fuse_progname, PATH_MAX );
      buffer[ PATH_MAX - 1 ] = '\0';
    } else {
      snprintf( buffer, PATH_MAX, "%s%s", fuse_directory, fuse_progname );
    }
    path2 = dirname( buffer );
    snprintf( ctx->path, PATH_MAX, "%s" FUSE_DIR_SEP_STR "%s", path2,
              path_segment );
    return 1;

    /* Then where we may have installed the data files */
  case 2:

#ifndef ROMSDIR
    path2 = FUSEDATADIR;
#else				/* #ifndef ROMSDIR */
    path2 = ctx->type == UTILS_AUXILIARY_ROM ? ROMSDIR : FUSEDATADIR;
#endif				/* #ifndef ROMSDIR */
    strncpy( ctx->path, path2, PATH_MAX ); buffer[ PATH_MAX - 1 ] = '\0';
    return 1;

  case 3: return 0;
  }
  ui_error( UI_ERROR_ERROR, "unknown path_context state %d", ctx->state );
  fuse_abort();
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

  file->buffer = malloc( file->length );
  if( !file->buffer ) {
    ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
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
  free( file->buffer );

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

/* Make a copy of a file in a temporary file */
int
utils_make_temp_file( int *fd, char *tempfilename, const char *filename,
		      const char *template )
{
  int error;
  utils_file file;
  ssize_t bytes_written;

#if defined AMIGA || defined __MORPHOS__
  snprintf( tempfilename, PATH_MAX, "%s%s", compat_get_temp_path(), template );
#else
  snprintf( tempfilename, PATH_MAX, "%s" FUSE_DIR_SEP_STR "%s",
            compat_get_temp_path(), template );
#endif

  *fd = mkstemp( tempfilename );
  if( *fd == -1 ) {
    ui_error( UI_ERROR_ERROR, "couldn't create temporary file: %s",
	      strerror( errno ) );
  }

  error = utils_read_file( filename, &file );
  if( error ) { close( *fd ); unlink( tempfilename ); return error; }

  bytes_written = write( *fd, file.buffer, file.length );
  if( bytes_written != file.length ) {
    if( bytes_written == -1 ) {
      ui_error( UI_ERROR_ERROR, "error writing to temporary file '%s': %s",
		tempfilename, strerror( errno ) );
    } else {
      ui_error( UI_ERROR_ERROR,
		"could write only %lu of %lu bytes to temporary file '%s'",
		(unsigned long)bytes_written, (unsigned long)file.length,
		tempfilename );
    }
    utils_close_file( &file ); close( *fd ); unlink( tempfilename );
    return 1;
  }

  error = utils_close_file( &file );
  if( error ) { close( *fd ); unlink( tempfilename ); return error; }

  return 0;
}
