/* periph.h: code for handling peripherals
   Copyright (c) 2004 Philip Kendall

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

#ifndef FUSE_PERIPH_H
#define FUSE_PERIPH_H

#include <libspectrum.h>

/*
 * General peripheral list handling routines
 */

typedef libspectrum_byte (*periph_port_read_function)( libspectrum_word port );
typedef void (*periph_port_write_function)( libspectrum_word port,
					    libspectrum_byte data );

/* Information about a peripheral */
typedef struct periph_t {

  /* This peripheral responds to all port values where
     <port> & mask == value */
  libspectrum_word mask;
  libspectrum_word value;

  periph_port_read_function read;
  periph_port_write_function write;

} periph_t;

int periph_register( const periph_t *peripheral );
int periph_register_n( const periph_t *peripherals_list, size_t n );
int periph_set_active( int id, int active );
void periph_clear( void );

/*
 * The actual routines to read and write a port
 */

libspectrum_byte readport( libspectrum_word port );
void writeport( libspectrum_word port, libspectrum_byte b );

/*
 * The more Fuse-specific peripheral handling routines
 */

int periph_setup( const periph_t *peripherals_list, size_t n, int kempston );
void periph_update( void );

#endif				/* #ifndef FUSE_PERIPH_H */
