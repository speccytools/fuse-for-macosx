/* sna.c: Routines for handling .sna snapshots
   Copyright (c) 2001 Philip Kendall

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

#include <string.h>

#include "libspectrum.h"

#define LIBSPECTRUM_SNA_HEADER_LENGTH 27
#define LIBSPECTRUM_SNA_128_HEADER_LENGTH 4

static int libspectrum_sna_identify( size_t buffer_length, 
				     libspectrum_machine *type );
static int libspectrum_sna_read_128_header( const uchar *buffer,
					    size_t buffer_length,
					    libspectrum_snap *snap );
static int libspectrum_sna_read_128_data( const uchar *buffer,
					  size_t buffer_length,
					  libspectrum_snap *snap );

int libspectrum_sna_read( const uchar *buffer, size_t buffer_length,
			  libspectrum_snap *snap )
{
  int error;

  error = libspectrum_sna_identify( buffer_length, &snap->machine );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = libspectrum_sna_read_header( buffer, buffer_length, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = libspectrum_sna_read_data(
    &buffer[LIBSPECTRUM_SNA_HEADER_LENGTH],
    buffer_length - LIBSPECTRUM_SNA_HEADER_LENGTH, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static int libspectrum_sna_identify( size_t buffer_length, 
				     libspectrum_machine *type )
{
  switch( buffer_length ) {
  case 49179:
    *type = LIBSPECTRUM_MACHINE_48;
    break;
  case 131103:
  case 147487:
    *type = LIBSPECTRUM_MACHINE_128;
    break;
  default:
    libspectrum_print_error( "libspectrum_sna_identify: unknown length\n" );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

int libspectrum_sna_read_header( const uchar *buffer, size_t buffer_length,
				 libspectrum_snap *snap )
{
  if( buffer_length < LIBSPECTRUM_SNA_HEADER_LENGTH ) {
    libspectrum_print_error(
      "libspectrum_sna_read_header: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  snap->a   = buffer[22]; snap->f  = buffer[21];
  snap->bc  = buffer[13] + buffer[14]*0x100;
  snap->de  = buffer[11] + buffer[12]*0x100;
  snap->hl  = buffer[ 9] + buffer[10]*0x100;
  snap->a_  = buffer[ 8]; snap->f_ = buffer[ 7];
  snap->bc_ = buffer[ 5] + buffer[ 6]*0x100;
  snap->de_ = buffer[ 3] + buffer[ 4]*0x100;
  snap->hl_ = buffer[ 1] + buffer[ 2]*0x100;
  snap->ix  = buffer[17] + buffer[18]*0x100;
  snap->iy  = buffer[15] + buffer[16]*0x100;
  snap->i   = buffer[ 0];
  snap->r   = buffer[20];
  snap->pc  = buffer[ 6] + buffer[ 7]*0x100;
  snap->sp  = buffer[23] + buffer[24]*0x100;

  snap->iff1 = snap->iff2 = ( buffer[19] & 0x04 ) ? 1 : 0;
  snap->im   = buffer[25] & 0x03;

  snap->out_ula = buffer[26] & 0x07;

  /* A bit before an interrupt. Why this value? Because it's what
     z80's `convert' uses :-) */
  snap->tstates = 69664;

  return LIBSPECTRUM_ERROR_NONE;
}

int libspectrum_sna_read_data( const uchar *buffer, size_t buffer_length,
			       libspectrum_snap *snap )
{
  int error;
  int offset; int page;
  int i,j;

  if( buffer_length < 0xc000 ) {
    libspectrum_print_error(
      "libspectrum_sna_read_data: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  switch( snap->machine ) {
  case LIBSPECTRUM_MACHINE_48:

    /* Rescue PC from the stack */
    offset = snap->sp - 0x4000;
    snap->pc = buffer[offset] + 0x100 * buffer[offset+1];

    /* Increase SP as PC has been unstacked */
    snap->sp += 2;

    /* And split the pages up */
    error = libspectrum_split_to_48k_pages( snap, buffer );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    break;

  case LIBSPECTRUM_MACHINE_128:
    
    for( i=0; i<8; i++ ) {
      snap->pages[i] = (uchar*)malloc( 0x4000 * sizeof(uchar) );
      if( snap->pages[i] == NULL ) {
	for( j=0; j<i; j++ ) { free( snap->pages[i] ); snap->pages[i] = NULL; }
	libspectrum_print_error("libspectrum_sna_read_data: out of memory\n");
	return LIBSPECTRUM_ERROR_MEMORY;
      }
    }

    memcpy( snap->pages[5], &buffer[0x0000], 0x4000 );
    memcpy( snap->pages[2], &buffer[0x4000], 0x4000 );

    buffer += 0xc000; buffer_length -= 0xc000;
    error = libspectrum_sna_read_128_header( buffer + 0xc000,
					     buffer_length - 0xc000, snap );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    page = snap->out_ula & 0x07;
    if( page == 5 || page == 2 ) {
      if( memcmp( snap->pages[page], &buffer[0x8000], 0x4000 ) ) {
	libspectrum_print_error(
	  "libspectrum_sna_read_data: duplicated page not identical\n"
	);
	return LIBSPECTRUM_ERROR_CORRUPT;
      }
    } else {
      memcpy( snap->pages[page], &buffer[0x8000], 0x4000 );
    }

    buffer += 0xc000 + LIBSPECTRUM_SNA_128_HEADER_LENGTH;
    buffer_length -= 0xc000 - LIBSPECTRUM_SNA_128_HEADER_LENGTH;
    error = libspectrum_sna_read_128_data( buffer, buffer_length, snap );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    break;

  default:
    libspectrum_print_error( "libspectrum_sna_read_data: unknown machine\n" );
    return LIBSPECTRUM_ERROR_LOGIC;
  }
  
  return LIBSPECTRUM_ERROR_NONE;
}

static int libspectrum_sna_read_128_header( const uchar *buffer,
					    size_t buffer_length,
					    libspectrum_snap *snap )
{
  if( buffer_length < 4 ) {
    libspectrum_print_error(
      "libspectrum_sna_read_128_header: not enough data in buffer\n"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  snap->pc = buffer[0] + 0x100 * buffer[1];
  snap->out_ula = buffer[2];

  return LIBSPECTRUM_ERROR_NONE;
}

static int
libspectrum_sna_read_128_data( const uchar *buffer, size_t buffer_length,
			       libspectrum_snap *snap )
{
  int i, page;

  page = snap->out_ula & 0x07;
  
  for( i=0; i<=7; i++ ) {

    if( i==2 || i==5 || i==page ) continue; /* Already got this page */

    /* Check we've still got some data to read */
    if( buffer_length < 0x4000 ) {
      libspectrum_print_error(
        "libspectrum_sna_read_128_data: not enough data in buffer\n"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }
    
    /* Copy the data across */
    memcpy( snap->pages[i], buffer, 0x4000 );
    
    /* And update what we're looking at here */
    buffer += 0x4000; buffer_length -= 0x4000;

  }

  return LIBSPECTRUM_ERROR_NONE;
}
