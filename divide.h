/* divide.h: DivIDE interface routines
   Copyright (c) 2005 Matthew Westcott

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

   E-mail: Philip Kendall <pak21-fuse@srcf.ucam.org>
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_DIVIDE_H
#define FUSE_DIVIDE_H

/* #include <libspectrum.h> */
#include "periph.h"

extern const periph_t divide_peripherals[];
extern const size_t divide_peripherals_count;

/* Whether DivIDE is currently paged in */
extern int divide_active;

/* Notify DivIDE hardware of an opcode fetch to one of the designated
   entry / exit points. Depending on configuration, it may or may not
   result in the DivIDE memory being paged in */
void divide_set_automap( int state );

/* Call this after some state change other than an opcode fetch which could
   trigger DivIDE paging (such as updating the write-protect flag), to
   re-evaluate whether paging will actually happen */
void divide_refresh_page_state( void );

void divide_reset( void );
void divide_memory_map( void );

#endif			/* #ifndef FUSE_ZXATASP_H */
