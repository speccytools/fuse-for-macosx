/* machine.h: Routines for handling the various machine types
   Copyright (c) 1999-2003 Philip Kendall

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

#include <libspectrum.h>

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

typedef BYTE (*spectrum_unattached_port_fn)( void );

/* How long do things take to happen; fields are pulled from libspectrum
   via the libspectrum_timings_* functions */
typedef struct machine_timings {

  /* Processor speed */
  DWORD processor_speed;

  /* Line timings in tstates */
  WORD left_border, horizontal_screen, right_border;
  WORD tstates_per_line;

  /* Frame timing */
  DWORD tstates_per_frame;

} machine_timings;

typedef struct fuse_machine_info {

  libspectrum_machine machine; /* which machine type is this? */
  const char *id;	/* Used to select from command line */

  int (*reset)(void);	/* Reset function */

  int timex;      /* Timex machine (keyboard emulation/loading sounds etc.) */

  machine_timings timings; /* How long do things take to happen? */
  /* Redraw line y this many tstates after interrupt */
  DWORD	line_times[DISPLAY_SCREEN_HEIGHT+1];

  spectrum_raminfo ram; /* How do we access memory, and what's currently
			   paged in */

  size_t rom_count;	/* How many ROMs does this machine use? */
  size_t *rom_length;	/* And how long is each ROM? */

  spectrum_port_info *peripherals; /* Which peripherals do we have? */
  spectrum_unattached_port_fn unattached_port; /* What to return if we read
						  from a port which isn't
						  attached to anything */

  ayinfo ay;		/* The AY-8-3912 chip */

  int (*shutdown)( void );

} fuse_machine_info;

extern fuse_machine_info **machine_types;	/* All available machines */
extern int machine_count;		/* of which there are this many */

extern fuse_machine_info *machine_current;	/* The currently selected machine */

int machine_init_machines( void );

int machine_select( int type );
int machine_select_id( const char *id );
const char* machine_get_id( libspectrum_machine type );

int machine_set_timings( fuse_machine_info *machine );

int machine_allocate_roms( fuse_machine_info *machine, size_t count );
int machine_load_rom( BYTE **data, char *filename, size_t expected_length );
int machine_find_rom( const char *filename );

int machine_reset( void );
int machine_end( void );

#endif			/* #ifndef FUSE_MACHINE_H */
