/* libspectrum.h
   Copyright (c) 2001,2002 Philip Kendall, Darren Salt

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

#ifndef LIBSPECTRUM_LIBSPECTRUM_H
#define LIBSPECTRUM_LIBSPECTRUM_H

#include <config.h>

#ifndef _STDLIB_H
#include <stdlib.h>
#endif			/* #ifndef _STDLIB_H */

#if   SIZEOF_CHAR  == 1
typedef unsigned  char libspectrum_byte;
#elif SIZEOF_SHORT == 1
typedef unsigned short libspectrum_byte;
#else
#error No plausible 8 bit types found
#endif

#if   SIZEOF_SHORT == 2
typedef unsigned short libspectrum_word;
#elif SIZEOF_INT   == 2
typedef unsigned   int libspectrum_word;
#else
#error No plausible 16 bit types found
#endif

#if   SIZEOF_INT   == 4
typedef unsigned  int libspectrum_dword;
#elif SIZEOF_LONG  == 4
typedef unsigned long libspectrum_dword;
#else
#error No plausible 32 bit types found
#endif

typedef unsigned char uchar;

/* The various errors which can occur */
typedef enum libspectrum_error {

  LIBSPECTRUM_ERROR_NONE = 0,
  LIBSPECTRUM_ERROR_MEMORY,
  LIBSPECTRUM_ERROR_UNKNOWN,
  LIBSPECTRUM_ERROR_CORRUPT,
  LIBSPECTRUM_ERROR_SIGNATURE,
  LIBSPECTRUM_ERROR_SLT,	/* .slt data found at end of a .z80 file */

  LIBSPECTRUM_ERROR_LOGIC = -1,

} libspectrum_error;

/* The machine types we can handle */
typedef enum libspectrum_machine {

  LIBSPECTRUM_MACHINE_48,
  LIBSPECTRUM_MACHINE_128,
  LIBSPECTRUM_MACHINE_PLUS3,
  LIBSPECTRUM_MACHINE_PENT,

} libspectrum_machine;

typedef struct libspectrum_snap {

  /* Which machine are we using here? */

  libspectrum_machine machine;

  /* Registers and the like */

  libspectrum_byte a , f ; libspectrum_word bc , de , hl ;
  libspectrum_byte a_, f_; libspectrum_word bc_, de_, hl_;

  libspectrum_word ix, iy; libspectrum_byte i, r;
  libspectrum_word sp, pc;

  libspectrum_byte iff1, iff2, im;

  /* RAM */

  libspectrum_byte *pages[8];

  /* Level data from .slt files */

  libspectrum_byte *slt[256];
  size_t slt_length[256];

  /* Peripheral status */

  libspectrum_byte out_ula; libspectrum_dword tstates;

  libspectrum_byte out_128_memoryport;

  libspectrum_byte out_ay_registerport, ay_registers[16];

  libspectrum_byte out_plus3_memoryport;

  /* Internal use only */

  int version;
  int compressed;

} libspectrum_snap;

/* Generic routines */

int libspectrum_snap_initalise( libspectrum_snap *snap );
int libspectrum_snap_destroy( libspectrum_snap *snap );

/* Whether to print messages on errors from libspectrum routines,
   and a function to print them */
extern int libspectrum_show_errors;
libspectrum_error libspectrum_print_error( const char *format, ... );

const char* libspectrum_error_message( libspectrum_error error );

int libspectrum_split_to_48k_pages( libspectrum_snap *snap,
				    const uchar* data );

int libspectrum_make_room( uchar **dest, size_t requested, uchar **ptr,
			   size_t *allocated );

int libspectrum_write_word( uchar *buffer, libspectrum_word w );

/* .sna specific routines */

int libspectrum_sna_read( uchar *buffer, size_t buffer_length,
			  libspectrum_snap *snap );
int libspectrum_sna_read_header( uchar *buffer, size_t buffer_length,
				 libspectrum_snap *snap );
int libspectrum_sna_read_data( uchar *buffer, size_t buffer_length,
			       libspectrum_snap *snap );

/* .z80 specific routines */

int libspectrum_z80_read( uchar *buffer, size_t buffer_length,
			  libspectrum_snap *snap );
int libspectrum_z80_read_header( uchar *buffer, libspectrum_snap *snap,
				 uchar **data );
int libspectrum_z80_read_blocks( uchar *buffer, size_t buffer_length,
				 libspectrum_snap *snap );
int libspectrum_z80_read_block( uchar *buffer, libspectrum_snap *snap,
				uchar **next_block, uchar *end );

int libspectrum_z80_write( uchar **buffer, size_t *length,
			   libspectrum_snap *snap );
int libspectrum_z80_write_header( uchar **buffer, size_t *offset,
				  size_t *length, libspectrum_snap *snap );
int libspectrum_z80_write_base_header( uchar **buffer, size_t *offset,
				       size_t *length,
				       libspectrum_snap *snap );
int libspectrum_z80_write_extended_header( uchar **buffer, size_t *offset,
					   size_t *length,
					   libspectrum_snap *snap );
int libspectrum_z80_write_pages( uchar **buffer, size_t *offset,
				 size_t *length, libspectrum_snap *snap );
int libspectrum_z80_write_page( uchar **buffer, size_t *offset,
				size_t *length, int page_num,
				uchar *page );

int libspectrum_z80_compress_block( uchar **dest, size_t *dest_length,
				    const uchar *src, size_t src_length);
int libspectrum_z80_uncompress_block( uchar **dest, size_t *dest_length,
				      const uchar *src, size_t src_length);

#endif				/* #ifndef LIBSPECTRUM_LIBSPECTRUM_H */
