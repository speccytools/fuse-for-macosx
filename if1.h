/* if2.c: Interface I handling routines
   Copyright (c) 2004 Gergely Szasz

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

int if1_reset( void );
void if1_page( void );
void if1_unpage( void );
void if1_memory_map( void );

void if1_port_out( libspectrum_word, libspectrum_byte );
libspectrum_byte if1_port_in( libspectrum_word, int * );

void if1_mdr_new( int );
void if1_mdr_insert( char *, int );
int if1_mdr_sync( char *, int );
int if1_mdr_eject( char *, int );
void if1_mdr_writep( int, int );
void if1_plug( char *, int );
void if1_unplug( int );

#endif				/* #ifndef FUSE_IF1_H */
