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

   E-mail: Philip Kendall <pak21-fuse@srcf.ucam.org>
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_ZXATASP_H
#define FUSE_ZXATASP_H

#include <libspectrum.h>
#include "periph.h"

extern const periph_t zxatasp_peripherals[];
extern const size_t zxatasp_peripherals_count;

extern int zxatasp_memenable;

int zxatasp_init( void );
int zxatasp_end( void );
void zxatasp_reset( void );
int zxatasp_insert( const char *filename, libspectrum_ide_unit unit );
int zxatasp_commit( libspectrum_ide_unit unit );
int zxatasp_eject( libspectrum_ide_unit unit );
void zxatasp_mem_setcs( void );

/* We're ignoring all mode bits and only emulating mode 0, basic I/O */
#define MC8255_PORT_C_LOW_IO  0x01
#define MC8255_PORT_B_IO      0x02
#define MC8255_PORT_C_HI_IO   0x08
#define MC8255_PORT_A_IO      0x10
#define MC8255_SETMODE        0x80

#define ZXATASP_IDE_REG       0x07
#define ZXATASP_RAM_BANK      0x1f
#define ZXATASP_IDE_WR        0x08
#define ZXATASP_IDE_RD        0x10
#define ZXATASP_IDE_PRIMARY   0x20
#define ZXATASP_RAM_LATCH     0x40
#define ZXATASP_RAM_DISABLE   0x80
#define ZXATASP_IDE_SECONDARY 0x80

#define ZXATASP_READ_PRIMARY( x )     ( ( x & 0x78 ) == 0x30 )
#define ZXATASP_WRITE_PRIMARY( x )    ( ( x & 0x78 ) == 0x28 )
#define ZXATASP_READ_SECONDARY( x )   ( ( x & 0xd8 ) == 0x90 )
#define ZXATASP_WRITE_SECONDARY( x )  ( ( x & 0xd8 ) == 0x88 )

#endif			/* #ifndef FUSE_ZXATASP_H */
