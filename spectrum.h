/* spectrum.h: Spectrum 48K specific routines
   Copyright (c) 1999-2003 Philip Kendall, Darren Salt

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

#ifndef FUSE_SPECTRUM_H
#define FUSE_SPECTRUM_H

#include <stdlib.h>

#include <libspectrum.h>

/* How many tstates have elapsed since the last interrupt? (or more
   precisely, since the ULA last pulled the /INT line to the Z80 low) */
extern libspectrum_dword tstates;

/* The last byte written to the ULA */
extern libspectrum_byte spectrum_last_ula;

/* Things relating to memory */

/* The number of ROMs we have allocated space for; they might not all be
   in use at the moment */
extern size_t spectrum_rom_count;

/* The ROMs themselves */
extern libspectrum_byte **ROM;

/* And the RAM */
extern libspectrum_byte RAM[8][0x4000];

typedef libspectrum_byte
  (*spectrum_screen_read_function)( libspectrum_word offset );

typedef libspectrum_dword
  (*spectrum_memory_contention_function)( libspectrum_word address );
typedef libspectrum_dword
  (*spectrum_port_contention_function)( libspectrum_word port );

typedef struct spectrum_raminfo {

  spectrum_screen_read_function read_screen; /* Read a byte from the
						current screen */

  spectrum_memory_contention_function contend_memory; /* How long must we wait
							 to access memory? */
  spectrum_port_contention_function contend_port; /* And how long to access
						     an IO port? */

  int locked;			/* Is the memory configuration locked? */
  int current_page,current_rom,current_screen; /* Current paged memory */

  libspectrum_byte last_byte;	/* The last byte sent to the 128K port */
  libspectrum_byte last_byte2;	/* The last byte sent to +3 port */

  int special;			/* Is a +3 special config in use? */

} spectrum_raminfo;

/* Set these every time we change machine to avoid having to do a
   structure lookup too often */
extern spectrum_screen_read_function read_screen_memory;

extern spectrum_memory_contention_function contend_memory;
extern spectrum_port_contention_function contend_port;

/* Things relating to peripherals */

typedef libspectrum_byte
  (*spectrum_port_read_function)( libspectrum_word port );
typedef void (*spectrum_port_write_function)( libspectrum_word port,
					      libspectrum_byte data );

typedef struct spectrum_port_info {
  libspectrum_word mask;	/* Mask port with these bits */
  libspectrum_word data;	/* Then see if it's equal to these bits */
  spectrum_port_read_function read; /* If so, call this function on read */
  spectrum_port_write_function write; /* Or this one on write */
} spectrum_port_info;

libspectrum_byte readport( libspectrum_word port );
void writeport( libspectrum_word port, libspectrum_byte b );

libspectrum_byte spectrum_port_noread( libspectrum_word port );
void spectrum_port_nowrite( libspectrum_word port, libspectrum_byte b );

libspectrum_byte spectrum_ula_read( libspectrum_word port );
void spectrum_ula_write( libspectrum_word port, libspectrum_byte b );

libspectrum_byte spectrum_unattached_port( int offset );

/* Miscellaneous stuff */

int spectrum_frame( void );

/* Data from .slt files */

extern size_t slt_length[256];		/* Length of data for this value of A
					   0 => no data */
extern libspectrum_byte *slt[256];	/* Actual data for each value of A */

extern libspectrum_byte *slt_screen;	/* The screenshot from the .slt file */
extern int slt_screen_level;		/* The level of the screenshot.
					   Not used for anything AFAIK */

#endif			/* #ifndef FUSE_SPECTRUM_H */
