/* tape.h: Routines for handling tape files
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

#ifndef LIBSPECTRUM_TAPE_H
#define LIBSPECTRUM_TAPE_H

#include <config.h>

#ifdef HAVE_LIB_GLIB
#include <glib.h>
#else				/* #ifdef HAVE_LIB_GLIB */
#include <myglib/myglib.h>	/* if not, use the local replacement */
#endif				/* #ifdef HAVE_LIB_GLIB */

#ifndef LIBSPECTRUM_LIBSPECTRUM_H
#include "libspectrum.h"
#endif			/* #ifndef LIBSPECTRUM_LIBSPECTRUM_H */

/*** Generic tape routines ***/

/* The various types of block available
   The values here are chosen to be the same as used in the .tzx format */
typedef enum libspectrum_tape_type {
  LIBSPECTRUM_TAPE_BLOCK_ROM = 0x10,
  LIBSPECTRUM_TAPE_BLOCK_TURBO,

  LIBSPECTRUM_TAPE_BLOCK_GROUP_START = 0x21,

  LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO = 0x32,
} libspectrum_tape_type;

/* A huge number of states available; encompasses all possible block types */
typedef enum libspectrum_tape_state_type {

  /* Normal/turbo loaders */
  LIBSPECTRUM_TAPE_STATE_PILOT,		/* Pilot pulses */
  LIBSPECTRUM_TAPE_STATE_SYNC1,		/* First sync pulse */
  LIBSPECTRUM_TAPE_STATE_SYNC2,		/* Second sync pulse */
  LIBSPECTRUM_TAPE_STATE_DATA1,		/* First edge of a data bit */
  LIBSPECTRUM_TAPE_STATE_DATA2,		/* Second edge of a data bit */

  /* Generic */
  LIBSPECTRUM_TAPE_STATE_PAUSE,		/* The pause at the end of a block */

} libspectrum_tape_state_type;

/* A standard ROM loading block */
typedef struct libspectrum_tape_rom_block {
  
  size_t length;		/* How long is this block */
  libspectrum_byte *data;	/* The actual data */
  libspectrum_dword pause;	/* Pause after block (milliseconds) */

  /* Private data */

  libspectrum_tape_state_type state;

  size_t edge_count;		/* Number of pilot pulses to go */

  size_t bytes_through_block;
  size_t bits_through_byte;	/* How far through the data are we? */

  libspectrum_byte current_byte; /* The current data byte; gets shifted out
				    as we read bits from it */
  libspectrum_dword bit_tstates; /* How long is an edge for the current bit */

} libspectrum_tape_rom_block;

/* The timings for the standard ROM loader */
extern const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_PILOT; /* Pilot */
extern const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_SYNC1; /* Sync 1 */
extern const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_SYNC2; /* Sync 2 */
extern const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_DATA0; /* Reset bit */
extern const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_DATA1; /* Set bit */

/* The number of pilot pulses for the standard ROM loader */
extern const size_t LIBSPECTRUM_TAPE_PILOTS_HEADER;
extern const size_t LIBSPECTRUM_TAPE_PILOTS_DATA;

/* A turbo loading block */
typedef struct libspectrum_tape_turbo_block {

  size_t length;		/* Length of data */
  size_t bits_in_last_byte;	/* How many bits are in the last byte? */
  libspectrum_byte *data;	/* The actual data */
  libspectrum_dword pause;	/* Pause after data (in ms) */

  libspectrum_dword pilot_length; /* Length of pilot pulse (in tstates) */
  size_t pilot_pulses;		/* Number of pilot pulses */

  libspectrum_dword sync1_length, sync2_length; /* Length of the sync pulses */
  libspectrum_dword bit0_length, bit1_length; /* Length of (re)set bits */

  /* Private data */

  libspectrum_tape_state_type state;

  size_t edge_count;		/* Number of pilot pulses to go */

  size_t bytes_through_block;
  size_t bits_through_byte;	/* How far through the data are we? */

  libspectrum_byte current_byte; /* The current data byte; gets shifted out
				    as we read bits from it */
  libspectrum_dword bit_tstates; /* How long is an edge for the current bit */

} libspectrum_tape_turbo_block;

/* A group start block */
typedef struct libspectrum_tape_group_start_block {

  libspectrum_byte *name;

} libspectrum_tape_group_start_block;

/* An archive info block */
typedef struct libspectrum_tape_archive_info_block {

  /* Number of strings */
  size_t count;

  /* ID for each string */
  int *ids;

  /* Text of each string */
  libspectrum_byte **strings;

} libspectrum_tape_archive_info_block;

/* A generic tape block */
typedef struct libspectrum_tape_block {

  libspectrum_tape_type type;

  union {
    libspectrum_tape_rom_block rom;
    libspectrum_tape_turbo_block turbo;
    libspectrum_tape_group_start_block group_start;
    libspectrum_tape_archive_info_block archive_info;
  } types;

} libspectrum_tape_block;

/* A linked list of tape blocks */
typedef struct libspectrum_tape {

  /* All the blocks */
  GSList* blocks;

  /* The current block */
  GSList* current_block;

} libspectrum_tape;

/* Routines to manipulate tape blocks */

libspectrum_error
libspectrum_tape_free( libspectrum_tape *tape );

libspectrum_error
libspectrum_tape_init_block( libspectrum_tape_block *block );

libspectrum_error
libspectrum_tape_get_next_edge( libspectrum_tape *tape,
				libspectrum_dword *tstates, int *end_of_tape );

libspectrum_error
libspectrum_tape_block_description( libspectrum_tape_block *block,
				    char *buffer, size_t length );

/*** Routines for .tap format files ***/

libspectrum_error
libspectrum_tap_create( libspectrum_tape *tape, const libspectrum_byte *buffer,
			const size_t length );

/*** Routines for .tzx format files ***/

libspectrum_error
libspectrum_tzx_create( libspectrum_tape *tape, const libspectrum_byte *buffer,
			const size_t length );

#endif				/* #ifndef LIBSPECTRUM_TAPE_H */
