/* file.c: File-related compatibility routines
   Copyright (c) 2008 Philip Kendall

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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "compat.h"
#include "ui/ui.h"

/* Certain brain damaged operating systems (DOS/Windows) treat text
   and binary files different in open(2) and need to be given the
   O_BINARY flag to tell them it's a binary file */
#ifndef O_BINARY
#define O_BINARY 0
#endif				/* #ifndef O_BINARY */

const compat_fd COMPAT_FILE_OPEN_FAILED = -1;

compat_fd
compat_file_open( const char *path, int write )
{
  int flags = write ? O_WRONLY | O_CREAT : O_RDONLY;
  return open( path, flags );
}

off_t
compat_file_get_length( compat_fd fd )
{
  struct stat file_info;

  if( fstat( fd, &file_info ) ) {
    ui_error( UI_ERROR_ERROR, "couldn't stat file: %s", strerror( errno ) );
    return -1;
  }

  return file_info.st_size;
}

int
compat_file_read( compat_fd fd, utils_file *file )
{
  ssize_t bytes = read( fd, file->buffer, file->length );
  if( bytes != file->length ) {
    if( bytes == -1 ) {
      ui_error( UI_ERROR_ERROR, "error reading file: %s", strerror( errno ) );
    } else {
      ui_error( UI_ERROR_ERROR,
                "error reading file: expected %d bytes, but read only %d",
                file->length, bytes );
    }
    return 1;
  }

  return 0;
}

int
compat_file_write( compat_fd fd, const unsigned char *buffer, size_t length )
{
  ssize_t bytes = write( fd, buffer, length );
  if( bytes != length ) {
    if( bytes == -1 ) {
      ui_error( UI_ERROR_ERROR, "error writing file: %s", strerror( errno ) );
    } else {
      ui_error( UI_ERROR_ERROR,
                "error writing file: expected %d bytes, but wrote only %d",
                length, bytes );
    }
    return 1;
  }

  return 0;
}

int
compat_file_close( compat_fd fd )
{
  return close( fd );
}
