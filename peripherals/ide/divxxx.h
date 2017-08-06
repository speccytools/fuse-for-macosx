/* divxxx.h: Shared DivIDE/DivMMC interface routines
   Copyright (c) 2005-2017 Matthew Westcott, Philip Kendall

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#ifndef FUSE_DIVXXX_H
#define FUSE_DIVXXX_H

#include <libspectrum.h>

/* Type definition */

typedef struct divxxx_t divxxx_t;

/* Allocation and deallocation */

divxxx_t*
divxxx_alloc( void );

void
divxxx_free( divxxx_t *divxxx );

/* Getters */

libspectrum_byte
divxxx_get_control( divxxx_t *divxxx );

/* Actions */

void
divxxx_reset( divxxx_t *divxxx, int enabled, int write_protect, int hard_reset, int *is_active, int *is_automapped, int page_event, int unpage_event );

void
divxxx_control_write( divxxx_t *divxxx, libspectrum_byte data, int is_automapped, int write_protect, int *is_active, int page_event, int unpage_event );

void
divxxx_control_write_internal( divxxx_t *divxxx, libspectrum_byte data, int is_automapped, int write_protect, int *is_active, int page_event, int unpage_event );

void
divxxx_set_automap( divxxx_t *divxxx, int *is_automapped, int automap, int write_protect, int *is_active, int page_event, int unpage_event );

void
divxxx_refresh_page_state( divxxx_t *divxxx, int is_automapped, int write_protect, int *is_active, int page_event, int unpage_event );

void
divxxx_memory_map( divxxx_t *divxxx, int is_active, int write_protect, size_t page_count, memory_page *memory_map_eprom, memory_page memory_map_ram[][ MEMORY_PAGES_IN_8K ] );

void
divxxx_page( int *is_active, int page_event );

void
divxxx_unpage( int *is_active, int page_event );

#endif			/* #ifndef FUSE_DIVXXX_H */
