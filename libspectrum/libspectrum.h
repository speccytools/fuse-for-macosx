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

#ifdef __GNUC__
#define GCC_UNUSED __attribute__ ((unused))
#else				/* #ifdef __GNUC__ */
#define GCC_UNUSED
#endif				/* #ifdef __GNUC__ */

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

typedef enum libspectrum_slt_type {

  LIBSPECTRUM_SLT_TYPE_END = 0,
  LIBSPECTRUM_SLT_TYPE_LEVEL,
  LIBSPECTRUM_SLT_TYPE_INSTRUCTIONS,
  LIBSPECTRUM_SLT_TYPE_SCREEN,
  LIBSPECTRUM_SLT_TYPE_PICTURE,
  LIBSPECTRUM_SLT_TYPE_POKE,

} libspectrum_slt_type;

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

  /* Data from .slt files */

  libspectrum_byte *slt[256];	/* Level data */
  size_t slt_length[256];	/* Length of each level */

  libspectrum_byte *slt_screen;	/* Loading screen */
  int slt_screen_level;		/* The id of the loading screen. Not used
				   for anything AFAIK, but I'll copy it
				   around just in case */

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

libspectrum_dword libspectrum_read_dword( const libspectrum_byte **buffer );
int libspectrum_write_word( uchar **buffer, libspectrum_word w );
int libspectrum_write_dword( libspectrum_byte **buffer, libspectrum_dword d );

/* .sna specific routines */

int libspectrum_sna_read( const uchar *buffer, size_t buffer_length,
			  libspectrum_snap *snap );
int libspectrum_sna_read_header( const uchar *buffer, size_t buffer_length,
				 libspectrum_snap *snap );
int libspectrum_sna_read_data( const uchar *buffer, size_t buffer_length,
			       libspectrum_snap *snap );

/* .z80 specific routines */

int libspectrum_z80_read( const libspectrum_byte *buffer, size_t buffer_length,
			  libspectrum_snap *snap );
int libspectrum_z80_read_header( const libspectrum_byte *buffer,
				 libspectrum_snap *snap,
				 const libspectrum_byte **data );
int libspectrum_z80_read_blocks( const libspectrum_byte *buffer,
				 size_t buffer_length,
				 libspectrum_snap *snap );
int libspectrum_z80_read_block( const libspectrum_byte *buffer,
				libspectrum_snap *snap,
				const libspectrum_byte **next_block,
				const libspectrum_byte *end );

int libspectrum_z80_write( libspectrum_byte **buffer, size_t *length,
			   libspectrum_snap *snap );
int libspectrum_z80_write_header( libspectrum_byte **buffer,
				  libspectrum_byte **ptr, size_t *length,
				  libspectrum_snap *snap );
int libspectrum_z80_write_base_header( libspectrum_byte **buffer,
				       libspectrum_byte **ptr, size_t *length,
				       libspectrum_snap *snap );
int libspectrum_z80_write_extended_header( libspectrum_byte **buffer,
					   libspectrum_byte **ptr,
					   size_t *length,
					   libspectrum_snap *snap );
int libspectrum_z80_write_pages( libspectrum_byte **buffer,
				 libspectrum_byte **ptr, size_t *length,
				 libspectrum_snap *snap );
int libspectrum_z80_write_page( libspectrum_byte **buffer,
				libspectrum_byte **ptr, size_t *length,
				int page_num, libspectrum_byte *page );

int libspectrum_z80_compress_block( libspectrum_byte **dest,
				    size_t *dest_length,
				    const libspectrum_byte *src,
				    size_t src_length);
int libspectrum_z80_uncompress_block( libspectrum_byte **dest,
				      size_t *dest_length,
				      const libspectrum_byte *src,
				      size_t src_length);

libspectrum_error 
libspectrum_zlib_inflate( const libspectrum_byte *gzptr, size_t gzlength,
			  libspectrum_byte **outptr, size_t *outlength );
libspectrum_error
libspectrum_zlib_compress( const libspectrum_byte *data, size_t length,
			   libspectrum_byte **gzptr, size_t *gzlength );

#endif				/* #ifndef LIBSPECTRUM_LIBSPECTRUM_H */
