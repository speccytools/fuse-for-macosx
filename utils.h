/* utils.h: some useful helper functions
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

#ifndef FUSE_UTILS_H
#define FUSE_UTILS_H

int utils_find_lib( const char *filename );
int utils_read_file( const char *filename, unsigned char **buffer,
		     size_t *length );
int utils_read_fd( int fd, const char *filename,
		   unsigned char **buffer, size_t *length );

int utils_write_file( const char *filename, const unsigned char *buffer,
		      size_t length );

/* Various types of file we might manage to identify */
typedef enum utils_type_t {

  /* Unidentified file */
  UTILS_TYPE_UNKNOWN = 0,
  
  /* Input recording files */
  UTILS_TYPE_RECORDING_RZX,

  /* Snapshot files */
  UTILS_TYPE_SNAPSHOT_SNA,
  UTILS_TYPE_SNAPSHOT_Z80,

  /* Tape files */
  UTILS_TYPE_TAPE_TAP,
  UTILS_TYPE_TAPE_TZX,

} utils_type_t;

/* Attempt to identify a file from its filename and contents */
int utils_identify_file( int *type, const char *filename,
			 const unsigned char *buffer, size_t length );

#endif			/* #ifndef FUSE_UTILS_H */
