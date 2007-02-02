/* ggi_internals.h: Routines for dealing with the GGI user interface
   Copyright (c) 2003 Catalin Mihaila <catalin@idgrup.ro>

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

   E-mail: philip-fuse@shadowmagic.org.uk

   Catalin: catalin@idgrup.ro

*/

#include "config.h"

#ifdef UI_GGI

void uiggi_event( void );

void ggikeyboard_keypress( int keysym );
void ggikeyboard_keyrelease( int keysym );

#endif
