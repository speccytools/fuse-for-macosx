/* zxatasp.h: ZXATASP interface routines
   Copyright (c) 2003-2004 Garry Lancaster,
		 2004 Philip Kendall

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

   E-mail: Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#ifndef FUSE_ZXATASP_H
#define FUSE_ZXATASP_H

#include <libspectrum.h>
#include "periph.h"

extern const periph_t zxatasp_peripherals[];
extern const size_t zxatasp_peripherals_count;

int zxatasp_init( void );
int zxatasp_end( void );
void zxatasp_reset( void );
int zxatasp_insert( const char *filename, libspectrum_ide_unit unit );
int zxatasp_commit( libspectrum_ide_unit unit );
int zxatasp_eject( libspectrum_ide_unit unit );
void zxatasp_memory_map( void );

int zxatasp_from_snapshot( libspectrum_snap *snap );
int zxatasp_to_snapshot( libspectrum_snap *snap );

#endif			/* #ifndef FUSE_ZXATASP_H */
