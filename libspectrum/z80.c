/* z80.c: Routines for handling .z80 snapshots
   Copyright (c) 2001-2002 Philip Kendall, Darren Salt

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libspectrum.h"

/* Length of the basic .z80 headers */
static const int LIBSPECTRUM_Z80_HEADER_LENGTH = 30;

/* Length of the v2 extensions */
#define LIBSPECTRUM_Z80_V2_LENGTH 23

/* Length of the v3 extensions */
#define LIBSPECTRUM_Z80_V3_LENGTH 54

/* Length of xzx's extensions */
#define LIBSPECTRUM_Z80_V3X_LENGTH 55

/* The signature used to designate the .slt extensions */
static libspectrum_byte slt_signature[] = "\0\0\0SLT";
static size_t slt_signature_length = 6;

static int read_v1_block( const libspectrum_byte *buffer, int is_compressed, 
			  libspectrum_byte **uncompressed,
			  const libspectrum_byte **next_block,
			  const libspectrum_byte *end );
static int
read_v2_block( const libspectrum_byte *buffer, libspectrum_byte **block,
	       size_t *length, int *page, const libspectrum_byte **next_block,
	       const libspectrum_byte *end );
static int libspectrum_z80_read_slt( libspectrum_snap *snap,
				     const libspectrum_byte **next_block,
				     const libspectrum_byte *end );
static int libspectrum_z80_write_slt( libspectrum_byte **buffer,
				      libspectrum_byte **ptr, size_t *length,
				      libspectrum_snap *snap );
static int libspectrum_z80_write_slt_entry( libspectrum_byte **buffer, 
					    libspectrum_byte **ptr,
					    size_t *length,
					    libspectrum_word type,
					    libspectrum_word id,
					    libspectrum_dword slt_length );

int libspectrum_z80_read( const libspectrum_byte *buffer, size_t buffer_length,
			  libspectrum_snap *snap )
{
  int error;
  const libspectrum_byte *data;

  error = libspectrum_z80_read_header( buffer, snap, &data );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = libspectrum_z80_read_blocks( data, buffer_length - (data - buffer),
				       snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

int libspectrum_z80_read_header( const libspectrum_byte *buffer,
				 libspectrum_snap *snap,
				 const libspectrum_byte **data )
{

  const libspectrum_byte *header = buffer;

  snap->a   = header[ 0]; snap->f  = header[ 1];
  snap->bc  = header[ 2] + header[ 3]*0x100;
  snap->de  = header[13] + header[14]*0x100;
  snap->hl  = header[ 4] + header[ 5]*0x100;
  snap->a_  = header[21]; snap->f_ = header[22];
  snap->bc_ = header[15] + header[16]*0x100;
  snap->de_ = header[17] + header[18]*0x100;
  snap->hl_ = header[19] + header[20]*0x100;
  snap->ix  = header[25] + header[26]*0x100;
  snap->iy  = header[23] + header[24]*0x100;
  snap->i   = header[10];
  snap->r   =(header[11]&0x7f) + ( (header[12]&0x01) << 7 );
  snap->pc  = header[ 6] + header[ 7]*0x100;
  snap->sp  = header[ 8] + header[ 9]*0x100;

  snap->iff1 = header[27] ? 1 : 0;
  snap->iff2 = header[28] ? 1 : 0;
  snap->im   = header[29] & 0x03;

  snap->out_ula = (header[12]&0x0e) >> 1;

  if( snap->pc == 0 ) {	/* PC == 0x0000 => v2 or greater .z80 */

    size_t extra_length;
    const libspectrum_byte *extra_header;
    libspectrum_dword quarter_tstates;

    extra_length = header[ LIBSPECTRUM_Z80_HEADER_LENGTH     ] +
                   header[ LIBSPECTRUM_Z80_HEADER_LENGTH + 1 ] * 0x100;

    switch( extra_length ) {
    case LIBSPECTRUM_Z80_V2_LENGTH:
      snap->version=2;
      break;
    case LIBSPECTRUM_Z80_V3_LENGTH:
    case LIBSPECTRUM_Z80_V3X_LENGTH:
      snap->version=3;
      break;
    default:
      libspectrum_print_error(
        "libspectrum_read_z80_header: unknown header length %d\n", extra_length
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;
      
    }

    extra_header = buffer + LIBSPECTRUM_Z80_HEADER_LENGTH + 2;

    snap->pc = extra_header[0] + extra_header[1] * 0x100;

    switch( snap->version ) {

    case 2:

      switch( extra_header[2] ) {
      case 0: case 1: case 2:
	snap->machine = LIBSPECTRUM_MACHINE_48;    break;
      case 3: case 4:
	snap->machine = LIBSPECTRUM_MACHINE_128;   break;
      default:
        libspectrum_print_error(
          "libspectrum_read_z80_header: unknown machine type %d\n",
	  extra_header[2]
        );
	return LIBSPECTRUM_ERROR_UNKNOWN;
      }
      break;

    case 3:

      switch( extra_header[2] ) {
      case 0: case 1: case 2: case 3:
	snap->machine = LIBSPECTRUM_MACHINE_48;    break;
      case 4: case 5: case 6:
	snap->machine = LIBSPECTRUM_MACHINE_128;   break;
      case 7: case 8: /* 8 mistakenly written by some versions of xzx */
	snap->machine = LIBSPECTRUM_MACHINE_PLUS3; break;
      case 9:
	snap->machine = LIBSPECTRUM_MACHINE_PENT;  break;
      default:
        libspectrum_print_error(
          "libspectrum_read_z80_header: unknown machine type %d\n",
	  extra_header[2]
        );
	return LIBSPECTRUM_ERROR_UNKNOWN;
      }
      break;

    default:
      libspectrum_print_error(
        "libspectrum_read_z80_header: unknown snap version %d\n", snap->version
      );
      return LIBSPECTRUM_ERROR_LOGIC;

    }

    if( snap->machine != LIBSPECTRUM_MACHINE_48 ) {

      if( extra_length == LIBSPECTRUM_Z80_V3X_LENGTH ) {
	snap->out_plus3_memoryport = extra_header[54];
      }

      snap->out_128_memoryport  = extra_header[ 3];
      snap->out_ay_registerport = extra_header[ 6];
      memcpy( snap->ay_registers, &extra_header[ 7], 16 );

    }

    /* 1/4 of the number of T-states in a frame */
    quarter_tstates = ( snap->machine==LIBSPECTRUM_MACHINE_48 ) ?
      17472 : 17727;

    /* This is correct, even if it does disagree with Z80 v3.05's
       `z80dump'; thanks to Pedro Gimeno for pointing this out when
       this code was part of SnapConv */
    snap->tstates= ( ( (extra_header[25]+1) % 4 ) + 1 ) * quarter_tstates -
      ( extra_header[23] + extra_header[24]*0x100 + 1 );
    
    /* Stop broken files from causing trouble... */
    if(snap->tstates>=quarter_tstates*4)
      snap->tstates = 0;
    
    (*data) = buffer + LIBSPECTRUM_Z80_HEADER_LENGTH + 2 + extra_length;

  } else {	/* v1 .z80 file */

    snap->machine=LIBSPECTRUM_MACHINE_48;
    snap->version=1;

    /* Need to flag this for later */
    snap->compressed = ( header[12] & 0x20 ) ? 1 : 0;

    /* A bit before an interrupt. Why this value? Because it's what
       z80's `convert' uses :-) */
    snap->tstates = 69664;

    (*data) = buffer + LIBSPECTRUM_Z80_HEADER_LENGTH;

  }

  return LIBSPECTRUM_ERROR_NONE;

}

int libspectrum_z80_read_blocks( const libspectrum_byte *buffer,
				 size_t buffer_length, libspectrum_snap *snap )
{
  const libspectrum_byte *end, *next_block;

  end = buffer + buffer_length; next_block = buffer;

  while( next_block < end ) {

    int error;

    error = libspectrum_z80_read_block( next_block, snap, &next_block, end );

    /* If it looks like some .slt data, try and parse that. That should
       then be the end of the file */
    if( error == LIBSPECTRUM_ERROR_SLT ) {
      error = libspectrum_z80_read_slt( snap, &next_block, end );
      if( error != LIBSPECTRUM_ERROR_NONE ) return error;
      
      /* If we haven't reached the end, return with error */
      if( next_block != end ) {
	libspectrum_print_error(
	  "libspectrum_z80_read_blocks: .slt data does not end file\n"
        );
	return LIBSPECTRUM_ERROR_CORRUPT;
      }

      /* If we have reached the end, that's OK */
      return LIBSPECTRUM_ERROR_NONE;
    }

    /* Return immediately with any other errors */
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

static int libspectrum_z80_read_slt( libspectrum_snap *snap,
				     const libspectrum_byte **next_block,
				     const libspectrum_byte *end )
{
  size_t slt_length[256], offsets[256];
  size_t whence = 0;

  size_t screen_length = 0, screen_offset = 0;

  int i;
  libspectrum_error error;

  /* Zero all lengths to imply `not present' */
  for( i=0; i<256; i++ ) slt_length[i]=0;

  while( 1 ) {

    int type, level, length;

    /* Check we've got enough data left */
    if( *next_block + 8 > end ) {
      libspectrum_print_error(
	"libspectrum_z80_read_slt: out of data in directory\n"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    type =   (*next_block)[0] + (*next_block)[1] * 0x100;
    level =  (*next_block)[2] + (*next_block)[3] * 0x100;
    (*next_block) += 4;
    length = libspectrum_read_dword( next_block );

    /* If this ends the table, exit */
    if( type == LIBSPECTRUM_SLT_TYPE_END ) break;

    /* Reset the pointer back to the start of the block */
    (*next_block) -= 8;

    switch( type ) {

    case LIBSPECTRUM_SLT_TYPE_LEVEL:	/* Level data */

      if( level >= 0x100 ) {
	libspectrum_print_error(
	  "libspectrum_z80_read_slt: unexpected level number %d\n", level
	);
	return LIBSPECTRUM_ERROR_CORRUPT;
      }

      /* Each level should appear once only */
      if( slt_length[ level ] ) {
	libspectrum_print_error(
          "libspectrum_z80_read_slt: level %d is duplicated\n", level
        );
        return LIBSPECTRUM_ERROR_CORRUPT;
      }

      offsets[ level ] = whence; slt_length[ level ] = length;

      break;

    case LIBSPECTRUM_SLT_TYPE_SCREEN:	/* Loading screen */

      /* Allow only one loading screen per .slt file */
      if( screen_length != 0 ) {
	libspectrum_print_error(
	  "libspectrum_z80_read_slt: duplicated loading screen\n"
	);
	return LIBSPECTRUM_ERROR_CORRUPT;
      }

      snap->slt_screen_level = level;
      screen_length = length; screen_offset = whence;

      break;

    default:

      libspectrum_print_error(
        "libspectrum_z80_read_slt: unknown data type %d\n", type
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;

    }

    /* and move both pointers along to the next block */
    *next_block += 8; whence += length;

  }

  /* Read in the data for each level */
  for( i=0; i<256; i++ ) {
    if( slt_length[i] ) {

      /* Check this data actually exists */
      if( *next_block + offsets[i] + slt_length[i] > end ) {
	libspectrum_print_error(
          "libspectrum_z80_read_slt: out of data reading level %d\n", i
	);
	return LIBSPECTRUM_ERROR_CORRUPT;
      }

      error = libspectrum_z80_uncompress_block(
	        &(snap->slt[i]), &(snap->slt_length[i]),
		*next_block + offsets[i], slt_length[i]
	      );
      if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    }
  }

  /* And the screen data */
  if( screen_length ) {

    /* Should expand to 6912 bytes, so give me a buffer that long */
    snap->slt_screen =
      (libspectrum_byte*)malloc( 6912 * sizeof( libspectrum_byte ) );
    if( snap->slt_screen == NULL ) {
      libspectrum_print_error(
        "libspectrum_z80_read_slt: out of memory\n"
      );
      return LIBSPECTRUM_ERROR_MEMORY;
    }

    if( screen_length == 6912 ) {	/* Not compressed */
      memcpy( snap->slt_screen, (*next_block) + screen_offset, 6912 );
    } else {				/* Compressed */
      
      error = libspectrum_z80_uncompress_block(
	        &(snap->slt_screen), &screen_length,
		(*next_block) + screen_offset, screen_length
	      );

      /* A screen should be 6912 bytes long */
      if( screen_length != 6912 ) {
	libspectrum_print_error(
	  "libspectrum_z80_read_slt: screen is not 6912 bytes long\n"
	);
	free( snap->slt_screen ); snap->slt_screen = NULL;
	return LIBSPECTRUM_ERROR_CORRUPT;
      }
    }
  }

  /* Move past the data */
  *next_block += whence;

  return LIBSPECTRUM_ERROR_NONE;
}

int libspectrum_z80_read_block( const libspectrum_byte *buffer,
				libspectrum_snap *snap,
				const libspectrum_byte **next_block,
				const libspectrum_byte *end )
{
  int error;
  libspectrum_byte *uncompressed;
  
  if( snap->version == 1 ) {

    error = read_v1_block( buffer, snap->compressed, &uncompressed,
			   next_block, end );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    libspectrum_split_to_48k_pages( snap, uncompressed );

    free( uncompressed );

  } else {

    size_t length;
    int page;

    error = read_v2_block( buffer, &uncompressed, &length, &page, next_block,
			   end );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    if( page <= 0 || page > 11 ) {
      libspectrum_print_error(
        "libspectrum_z80_read_block: unknown page %d\n", page
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    /* If it's a ROM page, just throw it away */
    if( page < 3 || page > 10 ) {
      free( uncompressed );
      return LIBSPECTRUM_ERROR_NONE;
    }

    /* Deal with 48K snaps -- first, throw away page 3, as it's a ROM.
       Then remap the numbers slightly */
    if( snap->machine == LIBSPECTRUM_MACHINE_48 ) {

      switch( page ) {

      case 3:
	free( uncompressed );
	return LIBSPECTRUM_ERROR_NONE;
      case 4:
	page=5;	break;
      case 5:
	page=3;	break;

      }
    }

    /* Now map onto RAM page numbers */
    page -= 3;

    if( snap->pages[page] == NULL ) {
      snap->pages[page] = uncompressed;
    } else {
      free( uncompressed );
      libspectrum_print_error(
        "libspectrum_z80_read_block: page %d duplicated\n", page
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

  }    

  return LIBSPECTRUM_ERROR_NONE;

}

static int read_v1_block( const libspectrum_byte *buffer, int is_compressed, 
			  libspectrum_byte **uncompressed,
			  const libspectrum_byte **next_block,
			  const libspectrum_byte *end )
{
  if( is_compressed ) {
    
    const libspectrum_byte *ptr;
    int state,error;
    size_t uncompressed_length = 0;

    state = 0; ptr = buffer;

    while( state != 4 ) {

      if( ptr == end ) {
	libspectrum_print_error( "read_v1_block: end marker not found\n" );
	return LIBSPECTRUM_ERROR_CORRUPT;
      }

      switch( state ) {
      case 0:
	switch( *ptr++ ) {
	case 0x00: state = 1; break;
	  default: state = 0; break;
	}
	break;
      case 1:
	switch( *ptr++ ) {
	case 0x00: state = 1; break;
	case 0xed: state = 2; break;
	  default: state = 0; break;
	}
	break;
      case 2:
	switch( *ptr++ ) {
	case 0x00: state = 1; break;
	case 0xed: state = 3; break;
	  default: state = 0; break;
	}
	break;
      case 3:
	switch( *ptr++ ) {
	case 0x00: state = 4; break;
	  default: state = 0; break;
	}
	break;
      default:
	libspectrum_print_error( "read_v1_block: unknown state %d\n", state );
	return LIBSPECTRUM_ERROR_LOGIC;
      }

    }

    /* Length passed here is reduced by 4 to remove the end marker */
    error = libspectrum_z80_uncompress_block(
      uncompressed, &uncompressed_length, buffer, ( ptr - buffer - 4 )
    );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    /* Uncompressed data must be exactly 48Kb long */
    if( uncompressed_length != 0xc000 ) {
      free( (*uncompressed) );
      libspectrum_print_error(
        "read_v1_block: data does not uncompress to 48Kb\n"
      );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    (*next_block) = ptr;

  } else {	/* Snap isn't compressed */

    /* Check we've got enough bytes to read */
    if( end - (*next_block) < 0xc000 ) {
      libspectrum_print_error( "read_v1_block: not enough data in buffer\n" );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    (*uncompressed) = (libspectrum_byte*)malloc( 0xc000 * sizeof(libspectrum_byte) );
    if( (*uncompressed) == NULL ) {
      libspectrum_print_error( "read_v1_block: out of memory\n" );
      return LIBSPECTRUM_ERROR_MEMORY;
    }

    memcpy( (*uncompressed), buffer, 0xc000 );

    (*next_block) = buffer + 0xc000;

  }

  return LIBSPECTRUM_ERROR_NONE;

}

static int
read_v2_block( const libspectrum_byte *buffer, libspectrum_byte **block,
	       size_t *length, int *page, const libspectrum_byte **next_block,
	       const libspectrum_byte *end )
{
  size_t length2;

  length2 = buffer[0] + buffer[1] * 0x100;
  (*page) = buffer[2];

  if (length2 == 0 && *page == 0) {
    if (buffer + 8 < end
	&& !memcmp( buffer, slt_signature, slt_signature_length ) )
    {
      /* Ah, we have what looks like SLT data... */
      *next_block = buffer + 6;
      return LIBSPECTRUM_ERROR_SLT;
    }
  }

  /* A length of 0xffff => 16384 bytes of uncompressed data */ 
  if( length2 != 0xffff ) {

    int error;

    /* Check we're not going to run over the end of the buffer */
    if( buffer + 3 + length2 > end ) {
      libspectrum_print_error( "read_v2_block: not enough data in buffer\n" );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    (*length)=0;
    error = libspectrum_z80_uncompress_block(
      block, length, buffer+3, length2
    );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    (*next_block) = buffer + 3 + length2;

  } else { /* Uncompressed block */

    /* Check we're not going to run over the end of the buffer */
    if( buffer + 3 + 0x4000 > end ) {
      libspectrum_print_error( "read_v2_block: not enough data in buffer\n" );
      return LIBSPECTRUM_ERROR_CORRUPT;
    }

    (*block) = (libspectrum_byte*)malloc( 0x4000 * sizeof(libspectrum_byte) );
    if( (*block) == NULL ) {
      libspectrum_print_error( "read_v2_block: out of memory\n" );
      return LIBSPECTRUM_ERROR_MEMORY;
    }

    memcpy( (*block), buffer+3, 0x4000 );

    (*length) = 0x4000;
    (*next_block) = buffer + 3 + 0x4000;

  }

  return LIBSPECTRUM_ERROR_NONE;

}

int libspectrum_z80_write( libspectrum_byte **buffer, size_t *length,
			   libspectrum_snap *snap )
{
  int error;
  libspectrum_byte *ptr = *buffer;

  error = libspectrum_z80_write_header( buffer, &ptr, length, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = libspectrum_z80_write_pages( buffer, &ptr, length, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  (*length) = ptr - *buffer;

  return LIBSPECTRUM_ERROR_NONE;
}

int libspectrum_z80_write_header( libspectrum_byte **buffer,
				  libspectrum_byte **ptr, size_t *length,
				  libspectrum_snap *snap )
{
  int error;

  error = libspectrum_z80_write_base_header( buffer, ptr, length, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = libspectrum_z80_write_extended_header( buffer, ptr, length, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

int libspectrum_z80_write_base_header( libspectrum_byte **buffer,
				       libspectrum_byte **ptr, size_t *length,
				       libspectrum_snap *snap )
{
  int error;

  error = libspectrum_make_room( buffer, LIBSPECTRUM_Z80_HEADER_LENGTH, ptr,
				 length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = snap->a; *(*ptr)++ = snap->f;
  libspectrum_write_word( ptr, snap->bc  );
  libspectrum_write_word( ptr, snap->hl  );

  libspectrum_write_word( ptr, 0 ); /* PC; zero denotes >= v2 */

  libspectrum_write_word( ptr, snap->sp  );
  *(*ptr)++ = snap->i; *(*ptr)++ = ( snap->r & 0x7f );

  *(*ptr)++ = ( snap->r >> 7 ) + ( ( snap->out_ula & 0x07 ) << 1 );

  libspectrum_write_word( ptr, snap->de  );
  libspectrum_write_word( ptr, snap->bc_ );
  libspectrum_write_word( ptr, snap->de_ );
  libspectrum_write_word( ptr, snap->hl_ );
  *(*ptr)++ = snap->a_; *(*ptr)++ = snap->f_;
  libspectrum_write_word( ptr, snap->iy  );
  libspectrum_write_word( ptr, snap->ix  );

  *(*ptr)++ = snap->iff1 ? 0xff : 0x00;
  *(*ptr)++ = snap->iff2 ? 0xff : 0x00;
  *(*ptr)++ = snap->im;

  return LIBSPECTRUM_ERROR_NONE;
}

int libspectrum_z80_write_extended_header( libspectrum_byte **buffer,
					   libspectrum_byte **ptr,
					   size_t *length,
					   libspectrum_snap *snap )
{
  int i, error;

  libspectrum_dword quarter_states;

  /* +2 here to deal with the two length bytes */
  error = libspectrum_make_room( buffer, LIBSPECTRUM_Z80_V3_LENGTH + 2, ptr,
				 length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  libspectrum_write_word( ptr, LIBSPECTRUM_Z80_V3_LENGTH );
  
  libspectrum_write_word( ptr, snap->pc );

  switch( snap->machine ) {
  case LIBSPECTRUM_MACHINE_48:
    *(*ptr)++ = 0; break;
  case LIBSPECTRUM_MACHINE_128:
    *(*ptr)++ = 4; break;
  case LIBSPECTRUM_MACHINE_PLUS3:
    *(*ptr)++ = 7; break;
  case LIBSPECTRUM_MACHINE_PENT:
    *(*ptr)++ = 9; break;
  }

  *(*ptr)++ = snap->out_128_memoryport;
  *(*ptr)++ = '\0';		/* IF1 disabled */
  *(*ptr)++ = '\0';		/* No special emulation features */
  *(*ptr)++ = snap->out_ay_registerport;
  memcpy( *ptr, snap->ay_registers, 16 ); *ptr += 16;

  /* Number of T-states in 1/4 of a frame */
  quarter_states = ( snap->machine == LIBSPECTRUM_MACHINE_48 ) ?
    17472 : 17727;
  libspectrum_write_word(
    ptr, quarter_states - ( snap->tstates % quarter_states ) - 1
  );

  *(*ptr)++ = ( ( snap->tstates / quarter_states ) + 3 ) % 4;

  /* Spectator, MGT and Multiface disabled */
  *(*ptr)++ = '\0'; *(*ptr)++ = '\0'; *(*ptr)++ = '\0';

  /* Is 0x0000 to 0x3fff RAM? Currently iff we're in a +3 special
     configuration */
  *(*ptr)++ = ( ( snap->machine == LIBSPECTRUM_MACHINE_PLUS3 ) && 
	      ( snap->out_plus3_memoryport & 0x01 ) ? 0xff : 0x00 );
  /* Ditto for 0x3fff to 0x7fff */
  *(*ptr)++ = ( ( snap->machine == LIBSPECTRUM_MACHINE_PLUS3 ) && 
	      ( snap->out_plus3_memoryport & 0x01 ) ? 0xff : 0x00 );

  /* Joystick settings, etc */
  for( i=32; i<=LIBSPECTRUM_Z80_V3_LENGTH; i++ ) *(*ptr)++ = '\0';

  return LIBSPECTRUM_ERROR_NONE;
}

int libspectrum_z80_write_pages( libspectrum_byte **buffer,
				 libspectrum_byte **ptr, size_t *length,
				 libspectrum_snap *snap )
{
  int i, error;
  int write_slt;

  if( snap->machine == LIBSPECTRUM_MACHINE_48 ) {
      error = libspectrum_z80_write_page( buffer, ptr, length, 4,
					  snap->pages[2] );
      if( error != LIBSPECTRUM_ERROR_NONE ) return error;
      error = libspectrum_z80_write_page( buffer, ptr, length, 5,
					  snap->pages[0] );
      if( error != LIBSPECTRUM_ERROR_NONE ) return error;
      error = libspectrum_z80_write_page( buffer, ptr, length, 8,
					  snap->pages[5] );
      if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  } else {

    for( i=0; i<8; i++ ) {
      if( snap->pages[i] != NULL ) {
	error = libspectrum_z80_write_page( buffer, ptr, length, i+3,
					    snap->pages[i] );
	if( error != LIBSPECTRUM_ERROR_NONE ) return error;
      }
    }

  }

  /* Do we want to write .slt data? Definitely if we've got a loading
     screen */
  write_slt = ( snap->slt_screen != NULL );

  /* If not, see if any level data exists */
  for( i=0; !write_slt && i<256; i++ )
    if( snap->slt_length[i] ) write_slt = 1;

  /* Write the data if we've got any */
  if( write_slt ) {
    error = libspectrum_z80_write_slt( buffer, ptr, length, snap );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  }

  return LIBSPECTRUM_ERROR_NONE;

}

int libspectrum_z80_write_page( libspectrum_byte **buffer,
				libspectrum_byte **ptr, size_t *length,
				int page_num, libspectrum_byte *page )
{
  libspectrum_byte *compressed; size_t compressed_length;
  int error;

  compressed_length = 0;

  error = libspectrum_z80_compress_block( &compressed, &compressed_length,
					  page, 0x4000 );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  if( compressed_length >= 0x4000 ) {

    error = libspectrum_make_room( buffer, 3 + 0x4000, ptr, length );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    libspectrum_write_word( ptr, 0xffff );
    *(*ptr)++ = page_num;
    memcpy( *ptr, page, 0x4000 ); *ptr += 0x4000;

  } else {

    libspectrum_make_room( buffer, 3 + compressed_length, ptr, length );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;

    libspectrum_write_word( ptr, compressed_length );
    *(*ptr)++ = page_num;
    memcpy( *ptr, compressed, compressed_length ); *ptr += compressed_length;

  }

  free( compressed );

  return LIBSPECTRUM_ERROR_NONE;

}

static int libspectrum_z80_write_slt( libspectrum_byte **buffer,
				      libspectrum_byte **ptr, size_t *length,
				      libspectrum_snap *snap )
{
  int i,j;

  size_t compressed_length[256];
  libspectrum_byte* compressed_data[256];

  size_t compressed_screen_length; libspectrum_byte* compressed_screen;

  libspectrum_error error;

  /* Make room for the .slt signature */
  error = libspectrum_make_room( buffer, slt_signature_length, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  memcpy( *ptr, slt_signature, slt_signature_length );
  *ptr += slt_signature_length;

  /* Now write out the .slt directory, compressing the data along
     the way */
  for( i=0; i<256; i++ ) {
    if( snap->slt_length[i] ) {

      /* Zero the compressed length so it will be allocated memory
	 by libspectrum_z80_compress_block */
      compressed_length[i] = 0;

      error = libspectrum_z80_compress_block(
                &compressed_data[i], &compressed_length[i],
		snap->slt[i], snap->slt_length[i]
	      );
      if( error != LIBSPECTRUM_ERROR_NONE ) {
	for( j=0; j<i; j++ ) if(snap->slt_length[j]) free(compressed_data[j]);
	return error;
      }

      error = libspectrum_z80_write_slt_entry(
                buffer, ptr, length,
                LIBSPECTRUM_SLT_TYPE_LEVEL, i, compressed_length[i]
              );
      if( error != LIBSPECTRUM_ERROR_NONE ) {
	for( j=0; j<i; j++ ) if(snap->slt_length[j]) free(compressed_data[j]);
	return error;
      }
    }
  }

  /* Write the directory entry for the loading screen out if we've got one */
  if( snap->slt_screen ) {

    compressed_screen_length = 0;
    error = libspectrum_z80_compress_block(
              &compressed_screen, &compressed_screen_length,
	      snap->slt_screen, 6912
	    );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      for( i=0; i<256; i++ ) if(snap->slt_length[i]) free(compressed_data[i]);
      return error;
    }

    /* If length >= 6912, write out uncompressed */
    if( compressed_screen_length >= 6912 ) {
      compressed_screen_length = 6912;
      memcpy( compressed_screen, snap->slt_screen, 6912 );
    }

    /* Write the directory entry */
    error = libspectrum_z80_write_slt_entry(
	      buffer, ptr, length,
	      LIBSPECTRUM_SLT_TYPE_SCREEN, snap->slt_screen_level,
	      compressed_screen_length
	    );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      for( i=0; i<256; i++ ) if(snap->slt_length[i]) free(compressed_data[i]);
      free( compressed_screen );
      return error;
    }
  }

  /* and the directory end marker */
  error = libspectrum_z80_write_slt_entry( buffer, ptr, length,
					   LIBSPECTRUM_SLT_TYPE_END, 0, 0 );
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    for( i=0; i<256; i++ ) if( snap->slt_length[i] ) free(compressed_data[i]);
    if( snap->slt_screen ) free( compressed_screen );
    return error;
  }

  /* Then write the actual level data */
  for( i=0; i<256; i++ ) {
    if( snap->slt_length[i] ) {
      
      /* Make room for the data */
      error = libspectrum_make_room( buffer, compressed_length[i], ptr,
				     length );
      if( error != LIBSPECTRUM_ERROR_NONE ) {
	for(j=0; j<256; j++) if(snap->slt_length[j]) free(compressed_data[j]);
	if( snap->slt_screen ) free( compressed_screen );
	return error;
      }

      /* And copy it across */
      memcpy( *ptr, compressed_data[i], compressed_length[i] );
      *ptr += compressed_length[i];
    }
  }

  /* And the loading screen */
  if( snap->slt_screen ) {

    /* Make room */
    error = libspectrum_make_room( buffer, compressed_screen_length, ptr,
				   length );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      for(i=0; i<256; i++) if( snap->slt_length[i] ) free(compressed_data[i]);
      if( snap->slt_screen ) free( compressed_screen );
      return error;
    }

    /* Copy the data */
    memcpy( *ptr, compressed_screen, compressed_screen_length );
    *ptr += compressed_screen_length;
  }

  /* Free up the compressed data */
  for( i=0; i<256; i++ ) if( snap->slt_length[i] ) free( compressed_data[i] );
  if( snap->slt_screen ) free( compressed_screen );

  /* That's your lot */
  return LIBSPECTRUM_ERROR_NONE;
}

static int libspectrum_z80_write_slt_entry( libspectrum_byte **buffer,
					    libspectrum_byte **ptr,
					    size_t *length,
					    libspectrum_word type,
					    libspectrum_word id,
					    libspectrum_dword slt_length )
{
  libspectrum_error error;

  /* We need 8 bytes of space */
  error = libspectrum_make_room( buffer, 8, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  libspectrum_write_word( ptr, type );
  libspectrum_write_word( ptr, id );
  libspectrum_write_word( ptr, slt_length & 0xffff );
  libspectrum_write_word( ptr, slt_length >> 16 );

  return LIBSPECTRUM_ERROR_NONE;
}

int libspectrum_z80_compress_block( libspectrum_byte **dest,
				    size_t *dest_length,
				    const libspectrum_byte *src,
				    size_t src_length)
{
  const libspectrum_byte *in_ptr;
  libspectrum_byte *out_ptr;
  int last_char_ed = 0;

  /* Allocate memory for dest if requested */
  if( *dest_length == 0 ) {
    *dest_length = src_length/2;
    *dest =
      (libspectrum_byte*)malloc( *dest_length * sizeof(libspectrum_byte) );
  }
  if( *dest == NULL ) {
    libspectrum_print_error("libspectrum_z80_compress_block: out of memory\n");
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  in_ptr = src;
  out_ptr = *dest;

  /* Now loop over the entire input block */
  while( in_ptr < src + src_length ) {

    /* If we're pointing at the last byte, just copy it across
       and exit */
    if( in_ptr == src + src_length - 1 ) {
      if( libspectrum_make_room( dest, 1, &out_ptr, dest_length ) ) {
	libspectrum_print_error( 
	  "libspectrum_z80_compress_block: out of memory\n"
	);
	return LIBSPECTRUM_ERROR_MEMORY;
      }
      *out_ptr++ = *in_ptr++;
      continue;
    }

    /* Now see if we're pointing to a run of identical bytes, and
       the last thing output wasn't a single 0xed */
    if( *in_ptr == *(in_ptr+1) && !last_char_ed ) {

      libspectrum_byte repeated;
      size_t run_length;
      
      /* Reset the `last character was a 0xed' flag */
      last_char_ed = 0;

      /* See how long the run is */
      repeated = *in_ptr;
      in_ptr += 2;
      run_length = 2;

      /* Find the length of the run (but cap it at 255 bytes) */
      while( in_ptr < src + src_length && *in_ptr == repeated && 
	     run_length < 0xff ) {
	run_length++;
	in_ptr++;
      }

      if( run_length >= 5 || repeated == 0xed ) {
	/* Output this in compressed form if it's of length 5 or longer,
	   _or_ if it's a run of 0xed */

	if( libspectrum_make_room( dest, 4, &out_ptr, dest_length ) ) {
	  libspectrum_print_error( 
  	    "libspectrum_z80_compress_block: out of memory\n"
	  );
	  return LIBSPECTRUM_ERROR_MEMORY;
	}

	*out_ptr++ = 0xed;
	*out_ptr++ = 0xed;
	*out_ptr++ = run_length;
	*out_ptr++ = repeated;

      } else {

	/* If not, just output the bytes */
	if( libspectrum_make_room( dest, run_length, &out_ptr, dest_length )) {
	  libspectrum_print_error( 
  	    "libspectrum_z80_compress_block: out of memory\n"
	  );
	  return LIBSPECTRUM_ERROR_MEMORY;
	}
	while(run_length--) *out_ptr++ = repeated;

      }

    } else {

      /* Not a repeated character, so just output the byte */
      last_char_ed = ( *in_ptr == 0xed ) ? 1 : 0;
      if( libspectrum_make_room( dest, 1, &out_ptr, dest_length ) ) {
	libspectrum_print_error( 
  	  "libspectrum_z80_compress_block: out of memory\n"
	);
	return LIBSPECTRUM_ERROR_MEMORY;
      }
      *out_ptr++ = *in_ptr++;

    }
      
  }

  (*dest_length) = out_ptr - *dest;

  return LIBSPECTRUM_ERROR_NONE;

}

int libspectrum_z80_uncompress_block( libspectrum_byte **dest,
				      size_t *dest_length,
				      const libspectrum_byte *src,
				      size_t src_length)
{
  const libspectrum_byte *in_ptr;
  libspectrum_byte *out_ptr;

  /* Allocate memory for dest if requested */
  if( *dest_length == 0 ) {
    *dest_length = src_length/2;
    *dest =
      (libspectrum_byte*)malloc( *dest_length * sizeof(libspectrum_byte) );
  }
  if( *dest == NULL ) {
    libspectrum_print_error(
      "libspectrum_z80_uncompress_block: out of memory\n"
    );
    return LIBSPECTRUM_ERROR_MEMORY;
  }

  in_ptr = src;
  out_ptr = *dest;

  while( in_ptr < src + src_length ) {

    /* If we're pointing at the last byte, just copy it across
       and exit */
    if( in_ptr == src + src_length - 1 ) {
      if( libspectrum_make_room( dest, 1, &out_ptr, dest_length ) ) {
	libspectrum_print_error(
          "libspectrum_z80_uncompress_block: out of memory\n"
	);
        return LIBSPECTRUM_ERROR_MEMORY;
      }
      *out_ptr++ = *in_ptr++;
      continue;
    }

    /* If we're pointing at two successive 0xed bytes, that's
       a run. If not, just copy the byte across */
    if( *in_ptr == 0xed && *(in_ptr+1) == 0xed ) {

      size_t run_length;
      libspectrum_byte repeated;
      
      in_ptr+=2;
      run_length = *in_ptr++;
      repeated = *in_ptr++;

      if( libspectrum_make_room( dest, run_length, &out_ptr, dest_length ) ) {
	libspectrum_print_error(
          "libspectrum_z80_uncompress_block: out of memory\n"
        );
	return LIBSPECTRUM_ERROR_MEMORY;
      }
      while(run_length--) *out_ptr++ = repeated;

    } else {

      if( libspectrum_make_room( dest, 1, &out_ptr, dest_length ) ) {
	libspectrum_print_error(
          "libspectrum_z80_uncompress_block: out of memory\n"
        );
	return LIBSPECTRUM_ERROR_MEMORY;
      }
      *out_ptr++ = *in_ptr++;

    }

  }

  (*dest_length) = out_ptr - *dest;

  return LIBSPECTRUM_ERROR_NONE;

}
