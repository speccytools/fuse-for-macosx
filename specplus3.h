/* specplus3.h: Spectrum +2A/+3 specific routines
   Copyright (c) 1999-2002 Philip Kendall

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

#ifndef FUSE_SPECPLUS3_H
#define FUSE_SPECPLUS3_H

#ifndef FUSE_TYPES_H
#include "types.h"
#endif			/* #ifndef FUSE_TYPES_H */

#ifndef FUSE_MACHINE_H
#include "machine.h"
#endif			/* #ifndef FUSE_MACHINE_H */

#ifdef HAVE_765_H
#include <limits.h>	/* Needed to get PATH_MAX */
#include <sys/types.h>

#include <765.h>
#endif			/* #ifdef HAVE_765_H */

BYTE specplus3_unattached_port( void );

BYTE specplus3_readbyte(WORD address);
BYTE specplus3_readbyte_internal( WORD address );
BYTE specplus3_read_screen_memory(WORD offset);
void specplus3_writebyte(WORD address, BYTE b);
void specplus3_writebyte_internal( WORD address, BYTE b );

DWORD specplus3_contend_memory( WORD address );
DWORD specplus3_contend_port( WORD address );

int specplus3_init( fuse_machine_info *machine );
int specplus3_reset(void);

void specplus3_memoryport_write(WORD port, BYTE b);

/* We need these outside the HAVE_765_H guards as they're also used
   for identifying the TRDOS drives */
typedef enum specplus3_drive_number {
  SPECPLUS3_DRIVE_A = 0,	/* First drive must be number zero */
  SPECPLUS3_DRIVE_B,
} specplus3_drive_number;

#ifdef HAVE_765_H
/* The +3's drives */

typedef struct specplus3_drive_t {
  int fd;			/* Our file descriptor for this disk; note
				   that lib765 will use a stdio stream
				   as well internally; if -1 => no disk
				   in drive */
  dev_t device;			/* The device and inode where this disk file */
  ino_t inode;			/* resides. Used to check for the same file
				   being put into both drives */
  FDRV_PTR drive;		/* The lib765 structure for this drive */
} specplus3_drive_t;

int specplus3_disk_insert( specplus3_drive_number which,
			   const char *filename );
int specplus3_disk_eject( specplus3_drive_number which );
#endif			/* #ifdef HAVE_765_H */

#endif			/* #ifndef FUSE_SPECPLUS3_H */
