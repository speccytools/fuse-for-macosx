/* machine.h: Routines for handling the various machine types
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

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_MACHINE_H
#define FUSE_MACHINE_H

#ifndef _STDLIB_H
#include <stdlib.h>
#endif			/* #ifndef _STDLIB_H */

#ifndef FUSE_AY_H
#include "ay.h"
#endif			/* #ifndef FUSE_AY_H */

#ifndef FUSE_DISPLAY_H
#include "display.h"
#endif			/* #ifndef FUSE_DISPLAY_H */

#ifndef FUSE_TYPES_H
#include "types.h"
#endif			/* #ifndef FUSE_TYPES_H */

#ifndef FUSE_SPECTRUM_H
#include "spectrum.h"
#endif			/* #ifndef FUSE_SPECTRUM_H */

typedef struct machine_timings {

  DWORD hz;		    /* Processor speed in Hz */

  WORD left_border_cycles;  /* T-states spent drawing left border */
  WORD screen_cycles;	    /* T-states spent drawing screen */
  WORD right_border_cycles; /* T-states spent drawing right border */
  WORD retrace_cycles;	    /* T-states spent in horizontal retrace */
  WORD cycles_per_line;	    /* = sum of above four values */

  WORD lines_per_frame;

  DWORD cycles_per_frame;   /* = cycles_per_line * lines_per_frame */

} machine_timings;  

typedef struct machine_info {

  int machine;		/* which machine type is this? */
  char *description;	/* Textual description of this machine */

  int (*reset)(void);	/* Reset function */

  machine_timings timings; /* How long do things take to happen? */
  /* Redraw line y this many tstates after interrupt */
  DWORD	line_times[DISPLAY_SCREEN_HEIGHT+1];

  spectrum_raminfo ram; /* How do we access memory, and what's currently
			   paged in */

  size_t rom_count;
  BYTE **roms;

  spectrum_port_info *peripherals; /* Which peripherals do we have? */
  ayinfo ay;		/* The AY-8-3912 chip */

} machine_info;

extern machine_info **machine_types;	/* All available machines */
extern int machine_count;		/* of which there are this many */

extern machine_info *machine_current;	/* The currently selected machine */

int machine_init_machines( void );

int machine_select_first( void );
int machine_select_next( void );
int machine_select( int type );

void machine_set_timings( machine_info *machine, DWORD hz,
			  WORD left_border_cycles,  WORD screen_cycles,
			  WORD right_border_cycles, WORD retrace_cycles,
			  WORD lines_per_frame, DWORD first_line);

int machine_allocate_roms( machine_info *machine, size_t count );
int machine_read_rom( machine_info *machine, size_t number,
		      const char* filename );
int machine_find_rom( const char *filename );

int machine_end( void );

#endif			/* #ifndef FUSE_MACHINE_H */
