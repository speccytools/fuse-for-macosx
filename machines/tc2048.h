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

#include <libspectrum.h>

libspectrum_byte tc2048_readbyte( libspectrum_word address );
libspectrum_byte tc2048_readbyte_internal( libspectrum_word address );
libspectrum_byte tc2048_read_screen_memory( libspectrum_word offset);
void tc2048_writebyte( libspectrum_word address, libspectrum_byte b);
void tc2048_writebyte_internal( libspectrum_word address, libspectrum_byte b );

libspectrum_dword tc2048_contend_memory( libspectrum_word address );
libspectrum_dword tc2048_contend_port( libspectrum_word port );

int tc2048_init( fuse_machine_info *machine );
int tc2048_reset( void );

#endif			/* #ifndef FUSE_TC2048_H */
