/* spectrum.h: Spectrum 48K specific routines
   Copyright (c) 1999 Philip Kendall

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

   E-mail: pak21@cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_SPECTRUM_H
#define FUSE_SPECTRUM_H

#ifndef FUSE_TYPES_H
#include "types.h"
#endif			/* #ifndef FUSE_TYPES_H */

#include "ay.h"

typedef struct raminfo {
  int type;
  WORD port;			/* The main paging port */
  WORD port2;			/* The special paging port */
  int locked;
  int current_page,current_rom,current_screen;
  BYTE last_byte;		/* The last byte sent to `port' */
  BYTE last_byte2;		/* The last byte sent to `port2' */
  int special;			/* Is a +3 special config in use? */
  int specialcfg;		/* If so, which one? */
} raminfo;

typedef struct machine_info {
  int machine;

  WORD cycles_per_line;
  WORD lines_per_frame;
  DWORD cycles_per_frame; /* = cycles_per_line*lines_per_frame */

  DWORD hz;		/* Processor speed in Hz */

  /* Redraw line y this many tstates after interrupt; l_t[192] is just an
     end marker */
  DWORD	line_times[192+1];

  void (*reset)(void);	/* Reset function */

  raminfo ram;          /* RAM paging information */
  ayinfo ay;		/* The AY-8-3912 chip */

} machine_info;

int spectrum_init();
void spectrum_set_timings(WORD cycles_per_line,WORD lines_per_frame,DWORD hz,
			  DWORD first_line);
int spectrum_interrupt(void);

BYTE (*readbyte)(WORD address);
BYTE (*read_screen_memory)(WORD offset);
void (*writebyte)(WORD address,BYTE b);
BYTE readport(WORD port);
void writeport(WORD port,BYTE b);

extern BYTE ROM[4][0x4000];
extern BYTE RAM[8][0x4000];
extern DWORD tstates;
extern machine_info machine;

/* The machines available */
enum { SPECTRUM_MACHINE_48, SPECTRUM_MACHINE_128, SPECTRUM_MACHINE_PLUS2,
       SPECTRUM_MACHINE_PLUS3 };

#endif			/* #ifndef FUSE_SPECTRUM_H */
