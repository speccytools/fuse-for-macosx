/* spec128.h: Spectrum 128K specific routines
   Copyright (c) 1999-2001 Philip Kendall

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

   E-mail: pak@ast.cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_SPEC128_H
#define FUSE_SPEC128_H

#ifndef FUSE_SPECTRUM_H
#include "spectrum.h"
#endif			/* #ifndef FUSE_SPECTRUM_H */

extern spectrum_port_info spec128_peripherals[];

BYTE spec128_readbyte(WORD address);
BYTE spec128_read_screen_memory(WORD offset);
void spec128_writebyte(WORD address, BYTE b);
int spec128_init(void);
int spec128_reset(void);

void spec128_memoryport_write(WORD port, BYTE b);

#endif			/* #ifndef FUSE_SPEC128_H */
