/* tc2048.h: Timex TC2048 specific routines
   Copyright (c) 1999-2002 Philip Kendall
   Copyright (c) 2002 Fredrick Meunier

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

#ifndef FUSE_TC2048_H
#define FUSE_TC2048_H

#ifndef FUSE_TYPES_H
#include "types.h"
#endif			/* #ifndef FUSE_TYPES_H */

BYTE tc2048_readbyte(WORD address);
BYTE tc2048_readbyte_internal( WORD address );
BYTE tc2048_read_screen_memory(WORD offset);
void tc2048_writebyte(WORD address, BYTE b);
void tc2048_writebyte_internal( WORD address, BYTE b );

DWORD tc2048_contend_memory( WORD address );
DWORD tc2048_contend_port( WORD port );

int tc2048_init( fuse_machine_info *machine );

#endif			/* #ifndef FUSE_TC2048_H */
