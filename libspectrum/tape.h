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

#include <glib.h>	/* FIXME: what to do if we don't have glib? */

#ifndef LIBSPECTRUM_LIBSPECTRUM_H
#include "libspectrum.h"
#endif			/* #ifndef LIBSPECTRUM_LIBSPECTRUM_H */

/* The various types of block available */
typedef enum libspectrum_tape_type {
  LIBSPECTRUM_TAPE_BLOCK_ROM,
} libspectrum_tape_type;

/* A standard ROM loading block */
typedef struct libspectrum_tape_rom_block {
  
  size_t length;
  libspectrum_byte *data;

} libspectrum_tape_rom_block;

/* A generic tape block */
typedef struct libspectrum_tape_block {

  libspectrum_tape_type type;

  union {
    libspectrum_tape_rom_block rom;
  } types;

} libspectrum_tape_block;

typedef struct libspectrum_tape {

  /* All the blocks */
  GSList* blocks;

  /* The current block */
  GSList* current_block;

} libspectrum_tape;

#endif				/* #ifndef LIBSPECTRUM_TAPE_H */
