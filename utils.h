/* utils.h: some useful helper functions
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

#ifndef FUSE_UTILS_H
#define FUSE_UTILS_H

#include <libspectrum.h>

/* The ways we may have allocated memory for a file */
typedef enum utils_file_open_mode {

  UTILS_FILE_OPEN_MMAP,
  UTILS_FILE_OPEN_MALLOC,

} utils_file_open_mode;

typedef struct utils_file {

  unsigned char *buffer;
  size_t length;

  utils_file_open_mode mode;

} utils_file;

int utils_open_file( const char *filename, int autoload,
		     libspectrum_id_t *type );
int utils_find_lib( const char *filename );
int utils_read_file( const char *filename, utils_file *file );
int utils_read_fd( int fd, const char *filename, utils_file *file );
int utils_close_file( utils_file *file );

int utils_write_file( const char *filename, const unsigned char *buffer,
		      size_t length );

#endif			/* #ifndef FUSE_UTILS_H */
