/* tape.c: Routines for handling tape files
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

#include "tape.h"

/* Local function prototypes */
static libspectrum_error
rom_edge( libspectrum_tape *tape, libspectrum_dword *tstates );

/* The main function: called with a tape object and returns the number of
   t-states until the next edge (or LIBSPECTRUM_TAPE_END if that was the
   last edge) */
libspectrum_error
libspectrum_tape_get_next_edge( libspectrum_tape *tape,
				libspectrum_dword *tstates )
{
  libspectrum_tape_block *block =
    (libspectrum_tape_block*)tape->current_block->data;

  switch( block->type ) {
  case LIBSPECTRUM_TAPE_BLOCK_ROM:
    return rom_edge( tape, tstates );
  default:
    tstates = 0;
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }
}

static libspectrum_error
rom_edge( libspectrum_tape *tape, libspectrum_dword *tstates )
{
  *tstates = 855;
  return LIBSPECTRUM_ERROR_NONE;
}
