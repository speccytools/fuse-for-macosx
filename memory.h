/* memory.h: memory access routines
   Copyright (c) 2003 Philip Kendall

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

#ifndef FUSE_MEMORY_H
#define FUSE_MEMORY_H

#include <libspectrum.h>

extern libspectrum_byte *memory_map[8];
extern int memory_writable[8];
extern int memory_contended[8];
extern libspectrum_byte *memory_screen_chunk1, *memory_screen_chunk2;
extern libspectrum_word memory_screen_top;

libspectrum_byte readbyte( libspectrum_word address );

#define readbyte_internal( address ) \
  memory_map[ (address) >> 13 ][ (address) & 0x1fff ]

void writebyte( libspectrum_word address, libspectrum_byte b );
void writebyte_internal( libspectrum_word address, libspectrum_byte b );

#endif				/* #ifndef FUSE_MEMORY_H */
