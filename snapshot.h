#/* snapshot.h: snapshot handling routines
   Copyright (c) 1999,2001-2002 Philip Kendall

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

#ifndef FUSE_SNAPSHOT_H
#define FUSE_SNAPSHOT_H

#ifndef LIBSPECTRUM_LIBSPECTRUM_H
#include <libspectrum.h>
#endif				/* #ifndef LIBSPECTRUM_LIBSPECTRUM_H */

int snapshot_read( const char *filename );
int snapshot_open_sna_buffer( const unsigned char *buffer, size_t length );
int snapshot_open_z80_buffer( const unsigned char *buffer, size_t length );

int snapshot_copy_from( libspectrum_snap *snap );

int snapshot_write( const char *filename );
int snapshot_copy_to( libspectrum_snap *snap );

void snapshot_flush_slt( void );

#endif
