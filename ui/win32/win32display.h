/* win32display.h: Routines for dealing with the Win32 GDI display
   Copyright (c) 2003 Marek Januszewski

   $Id$

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

#include <config.h>

#include "win32internals.h"

extern BITMAPINFO fuse_BMI;
extern HBITMAP fuse_BMP;
extern int fuse_nCmdShow;

extern void *win32_pixdata;
extern int win32display_sizechanged;

extern libspectrum_dword win32display_colours[16];

int win32display_init( void );
int win32display_end( void );
void win32display_resize( int size );
void win32display_resize_update( void );

void blit( void );
