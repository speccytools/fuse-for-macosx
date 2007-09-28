/* fdd.h: Header for handling raw disk images
   Copyright (c) 2007 Gergely Szasz

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

   Philip: philip-fuse@shadowmagic.org.uk

*/

#ifndef FUSE_DISK_H
#define FUSE_DISK_H

#include <config.h>

typedef enum disk_error_t {
  DISK_OK = 0,
  DISK_IMPL,
  DISK_MEM,
  DISK_GEOM,
  DISK_OPEN,
  DISK_UNSUP,
  DISK_RDONLY,
  DISK_CLOSE,
  DISK_WRFILE,
  DISK_WRPART,
  
  DISK_LAST_ERROR,
} disk_error_t;

typedef enum disk_type_t {
  DISK_TYPE_NONE = 0,
  DISK_UDI,		/* raw track disk image (our format :-) */
  DISK_FDI,		/* Full Disk Image ALT */
  DISK_TD0,

  DISK_SDF,		/* SAM Disk Format (deprecated) */

  /* DISCiPLE / +D / SAM Coupe */
  DISK_MGT,		/* ALT */
  DISK_IMG,		/* OUT-OUT */
  DISK_SAD,		/* OUT-OUT with geometry header */

  DISK_CPC,
  DISK_ECPC,

  /* Betadisk (TR-DOS) */
  DISK_TRD,
  DISK_SCL,

  /* Opus Discovery */
  DISK_OPD,

  /* Log disk structure (.log) */
  DISK_LOG,

  DISK_TYPE_LAST,
} disk_type_t;

typedef enum disk_dens_t {
  DISK_DENS_AUTO = 0,
  DISK_8_SD,		/* 8" SD floppy 5208 */
  DISK_8_DD,		/* 8" DD floppy 10416 */
  DISK_SD,		/* 3125 bpt */
  DISK_DD,		/* 6250 bpt */
  DISK_HD,		/* 12500 bpt*/
} disk_dens_t;

typedef struct disk_t {
  char *filename;
 
  int sides;		/* 1 or 2 */
  int cylinders;	/* tracks per side  */
  int bpt;		/* bytes per track */
  int wrprot;		/* disk write protect */
  int dirty;		/* disk changed */
  disk_error_t status;		/* last error code */
  libspectrum_byte *data;	/* disk data */
/* private part */
  int tlen;			/* length of a track with clock marks (bpt + 1/8bpt) */
  libspectrum_byte *track;	/* current track data bytes */
  libspectrum_byte *clocks;	/* clock marks bits */
  int i;			/* index for track and clocks */
  disk_type_t type;		/* DISK_UDI, ... */
  disk_dens_t density;		/* DISK_SD DISK_DD, or DISK_HD */
} disk_t;

const char *disk_strerror( int error );
/* create an unformatted disk sides -> (1/2) cylinders -> track/side,
   dens -> 'density' related to unformatted length of a track (SD = 3125,
   DD = 6250, HD = 12500, type -> if write this disk we want to convert
   into this type of image file (now only UDI implemented)
*/
int disk_new( disk_t *d, int sides, int cylinders, disk_dens_t dens, disk_type_t type );
/* open a disk image file. if preindex = 1 and the image file not UDI then
   pre-index gap generated with index mark (0xfc)
   this time only .mgt(.dsk)/.img/.udi and CPC/extended CPC file format
   supported
*/
int disk_open( disk_t *d, const char *filename, int preindex );
/* write a disk image file (from the disk buffer). the d->type
   gives the format of file. if it DISK_TYPE_AUTO, disk_write
   try to guess from the file name (extension). if fail save as
   UDI.
   This time only the UDI format implemented
*/
int disk_write( disk_t *d, const char *filename );
/* close a disk and free buffers
*/
void disk_close( disk_t *d );

#endif /* FUSE_DISK_H */
