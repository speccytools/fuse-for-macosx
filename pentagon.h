/* pentagon.h: Pentagon 128K specific routines
   Copyright (c) 1999-2003 Philip Kendall and Fredrick Meunier

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

#ifndef FUSE_PENTAGON_H
#define FUSE_PENTAGON_H

#ifndef FUSE_MACHINE_H
#include "machine.h"
#endif			/* #ifndef FUSE_MACHINE_H */

extern spectrum_port_info pentagon_peripherals[];

BYTE pentagon_unattached_port( void );

BYTE pentagon_readbyte(WORD address);
BYTE pentagon_readbyte_internal(WORD address);
BYTE pentagon_read_screen_memory(WORD offset);
void pentagon_writebyte(WORD address, BYTE b);
void pentagon_writebyte_internal(WORD address, BYTE b);

DWORD pentagon_contend_memory( WORD address );
DWORD pentagon_contend_port( WORD port );

int pentagon_init( fuse_machine_info *machine );
int pentagon_reset(void);

void pentagon_memoryport_write(WORD port, BYTE b);

#endif			/* #ifndef FUSE_PENTAGON_H */
