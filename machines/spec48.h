/* spec48.h: Spectrum 48K specific routines
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

#ifndef FUSE_SPEC48_H
#define FUSE_SPEC48_H

#include <libspectrum.h>

libspectrum_byte spec48_read_screen_memory( libspectrum_word offset );
libspectrum_dword spec48_contend_port( libspectrum_word port );
libspectrum_byte spec48_contend_delay( libspectrum_dword time );

int spec48_init( fuse_machine_info *machine );

#endif			/* #ifndef FUSE_SPEC48_H */
