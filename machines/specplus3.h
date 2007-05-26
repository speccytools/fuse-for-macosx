/* specplus3.h: Spectrum +2A/+3 specific routines
   Copyright (c) 1999-2004 Philip Kendall

   $Id$

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#ifndef FUSE_SPECPLUS3_H
#define FUSE_SPECPLUS3_H

#include <libspectrum.h>

#include "machine.h"
#include "periph.h"

#ifdef HAVE_765_H
#include <limits.h>	/* Needed to get PATH_MAX */
#include <sys/types.h>

#include <765.h>
#endif			/* #ifdef HAVE_765_H */

extern const periph_t specplus3_peripherals[];
extern const size_t specplus3_peripherals_count;

libspectrum_byte specplus3_unattached_port( void );
int specplus3_port_contended( libspectrum_word port );
libspectrum_byte specplus3_contend_delay( libspectrum_dword time );

int specplus3_init( fuse_machine_info *machine );
void specplus3_765_init( void );

int specplus3_plus2a_common_reset( void );
void specplus3_fdc_reset( void );
void specplus3_menu_items( void );
int specplus3_shutdown( void );

void specplus3_memoryport_write( libspectrum_word port, libspectrum_byte b );
void specplus3_memoryport2_write( libspectrum_word port, libspectrum_byte b );

int specplus3_memory_map( void );

/* We need these outside the HAVE_765_H guards as they're also used
   for identifying the TRDOS drives */
typedef enum specplus3_drive_number {
  SPECPLUS3_DRIVE_A = 0,	/* First drive must be number zero */
  SPECPLUS3_DRIVE_B,
} specplus3_drive_number;

#ifdef HAVE_765_H
/* The +3's drives */

typedef struct specplus3_drive_t {
  int fd;			/* The file descriptor for the
				   temporary file we are using for
				   this disk */
  char filename[ PATH_MAX ];	/* And the name of the temporary file */

  FDRV_PTR drive;		/* The lib765 structure for this drive */
} specplus3_drive_t;

int specplus3_disk_present( specplus3_drive_number which );
int specplus3_disk_insert_default_autoload( specplus3_drive_number which,
                                            const char *filename );
int specplus3_disk_insert( specplus3_drive_number which, const char *filename,
                           int autoload );
int specplus3_disk_eject( specplus3_drive_number which, int save );
int specplus3_disk_write( specplus3_drive_number which, const char *filename );
#endif			/* #ifdef HAVE_765_H */

#endif			/* #ifndef FUSE_SPECPLUS3_H */
