/* if2.c: Interface II cartridge handling routines
   Copyright (c) 2004 Fredrick Meunier

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

   Philip: philip-fuse@shadowmagic.org.uk

   Fred: fredm@spamcop.net

*/

#ifndef FUSE_IF2_H
#define FUSE_IF2_H

#include <libspectrum.h>

/* IF2 cart inserted? */
extern int if2_active;

int if2_insert( const char *filename );
void if2_eject( void );
int if2_reset( void );
void if2_memory_map( void );

int if2_from_snapshot( libspectrum_snap *snap );
int if2_to_snapshot( libspectrum_snap *snap );

#endif				/* #ifndef FUSE_IF2_H */
