/* if2.c: Interface I handling routines
   Copyright (c) 2004-2005 Gergely Szasz, Philip Kendall

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

   Gergely: szaszg@hu.inter.net

*/

#ifndef FUSE_IF1_H
#define FUSE_IF1_H

#include <libspectrum.h>

/* IF1 */
extern int if1_active;
extern int if1_available;

int if1_init( void );
libspectrum_error if1_end( void );

int if1_reset( void );
void if1_page( void );
void if1_unpage( void );
void if1_memory_map( void );

void if1_port_out( libspectrum_word port, libspectrum_byte val );
libspectrum_byte if1_port_in( libspectrum_word port, int *attached );

void if1_mdr_new( int drive );
void if1_mdr_insert( const char *filename, int drive );
int if1_mdr_sync( const char *filename, int drive );
int if1_mdr_eject( const char *filename, int drive );
void if1_mdr_writep( int w, int drive );
void if1_plug( const char *filename, int what );
void if1_unplug( int what );

void if1_update_menu( void );

#endif				/* #ifndef FUSE_IF1_H */
