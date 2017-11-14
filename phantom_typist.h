/* phantom_typist.h: starting game loading automatically
   Copyright (c) 2017 Philip Kendall

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#ifndef FUSE_PHANTOM_TYPIST_H
#define FUSE_PHANTOM_TYPIST_H

void
phantom_typist_activate( libspectrum_machine machine );

libspectrum_byte
phantom_typist_ula_read( libspectrum_word port );

void
phantom_typist_frame( void );

#endif /* #ifndef FUSE_PHANTOM_TYPIST_H */
