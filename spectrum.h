/* spectrum.h: Spectrum 48K specific routines
   Copyright (c) 1999-2001 Philip Kendall

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

   E-mail: pak21-fuse.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_SPECTRUM_H
#define FUSE_SPECTRUM_H

#ifndef FUSE_TYPES_H
#include "types.h"
#endif			/* #ifndef FUSE_TYPES_H */

extern DWORD tstates;

/* Things relating to memory */

extern BYTE **ROM;
extern BYTE RAM[8][0x4000];

typedef BYTE (*spectrum_memory_read_function) ( WORD address );
typedef BYTE (*spectrum_screen_read_function) ( WORD offset );
typedef void (*spectrum_memory_write_function)( WORD address, BYTE data );

typedef struct spectrum_raminfo {

  spectrum_memory_read_function read_memory; /* Read a byte from anywhere in 
						paged in memory */
  spectrum_screen_read_function read_screen; /* Read a byte from the
						current screen */
  spectrum_memory_write_function write_memory; /* Write to paged-in memory */

  int locked;			/* Is the memory configuration locked? */
  int current_page,current_rom,current_screen; /* Current paged memory */

  BYTE last_byte;		/* The last byte sent to the 128K port */
  BYTE last_byte2;		/* The last byte sent to +3 port */

  int special;			/* Is a +3 special config in use? */
  int specialcfg;		/* If so, which one? */

} spectrum_raminfo;

/* Set these every time we change machine to avoid having to do a
   structure lookup on every memory access */
spectrum_memory_read_function readbyte;
spectrum_screen_read_function read_screen_memory;
spectrum_memory_write_function writebyte;

/* Things relating to peripherals */

typedef BYTE (*spectrum_port_read_function)(WORD port);
typedef void (*spectrum_port_write_function)(WORD port, BYTE data);

typedef struct spectrum_port_info {
  WORD mask;			/* Mask port with these bits */
  WORD data;			/* Then see if it's equal to these bits */
  spectrum_port_read_function read; /* If so, call this function on read */
  spectrum_port_write_function write; /* Or this one on write */
} spectrum_port_info;

BYTE readport(WORD port);
void writeport(WORD port, BYTE b);

BYTE spectrum_port_noread(WORD port);

BYTE spectrum_ula_read(WORD port);
void spectrum_ula_write(WORD port, BYTE b);

/* Miscellaneous stuff */

int spectrum_interrupt(void);

/* The machines available */
enum { SPECTRUM_MACHINE_48, SPECTRUM_MACHINE_128, SPECTRUM_MACHINE_PLUS2,
       SPECTRUM_MACHINE_PLUS3 };

#endif			/* #ifndef FUSE_SPECTRUM_H */
