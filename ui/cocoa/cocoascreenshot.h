/* cocoascreenshot.h: Routines for saving .png etc. screenshots
   Copyright (c) 2006 Fredrick Meunier

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

#ifndef COCOASCREENSHOT_H
#define COCOASCREENSHOT_H

int screenshot_write( const char *filename );

int get_rgb24_data( libspectrum_byte *rgb24_data, size_t stride,
                    size_t height, size_t width, size_t height_offset,
                    size_t width_offset );

#endif				/* #ifndef COCOASCREENSHOT_H */
