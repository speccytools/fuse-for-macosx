/* fdd.c: Routines for emulating floppy disk drives
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

#include <config.h>

#include <stdlib.h>
#include <stdio.h>

#include <libspectrum.h>

#include "compat.h"
#include "disk.h"
#include "fdd.h"
#include "settings.h"

#define FDD_LOAD_FACT 2
#define FDD_HEAD_FACT 16
#define FDD_STEP_FACT 256

static const char *fdd_error[] = {
  "OK",
  "invalid disk geometry",
  "read only disk",

  "unknown error code"			/* will be the last */
};

const char *
fdd_strerror( int error )
{
  if( error >= FDD_LAST_ERROR )
    error = FDD_LAST_ERROR;
  return fdd_error[error];
}

/*
   disk->sides  1  2  1  2  1  2  1  2
   d->c_head    0  0  1  1  0  0  1  1
   d->upside    0  0  0  0  1  1  1  1
   
   UNREADABLE   0  0  1  0  1  0  0  0


*/
static void
fdd_set_data( fdd_t *d, int fact )
{
  int head = d->upsidedown ? 1 - d->c_head : d->c_head;
  
  if( !d->loaded )
    return;

  if( d->unreadable || ( d->disk->sides == 1 && head == 1 ) ) {
    d->disk->track = NULL;
    d->disk->clocks = NULL;
    return;
  }
  
  d->disk->track = d->disk->data + 
		( d->disk->sides * d->c_cylinder + head ) * d->disk->tlen;
  d->disk->clocks = d->disk->track + d->disk->bpt;
  d->disk->i += rand() % ( d->disk->bpt / fact );
  while( d->disk->i >= d->disk->bpt )
    d->disk->i -= d->disk->bpt;
}

/*
 *
 * initialize fdd
 *
 *
 */

int
fdd_init( fdd_t *d, int heads, int cyls )
{
  
  d->fdd_heads = d->fdd_cylinders = d->c_head = d->c_cylinder = d->wrprot = 0;
  d->upsidedown = d->unreadable = d->loaded = 0; d->index = d->tr00 = 1;
  d->disk = NULL;

  if( heads < 0 || heads > 2 || cyls < 0 || cyls > 83 )
    return d->status = FDD_GEOM;

  d->fdd_heads = heads;
  d->fdd_cylinders = cyls;
  
  return d->status = DISK_OK;
}

/*
 *
 * load a disk into fdd
 *
 *
 */

int
fdd_load( fdd_t *d, disk_t *disk, int upsidedown )
{
  
  if( disk->sides < 0 || disk->sides > 2 || disk->cylinders < 0 || disk->cylinders > 83 )
    return d->status = FDD_GEOM;
  if( d->fdd_heads == 0 )
    d->fdd_heads = disk->sides;
  if( d->fdd_cylinders == 0 )
    d->fdd_cylinders = disk->cylinders;
  if( disk->cylinders > d->fdd_cylinders )
    d->unreadable = 1;

  d->disk = disk;
  d->upsidedown = upsidedown > 0 ? 1 : 0;
  d->wrprot = d->disk->wrprot;		/* write protect */
  d->loaded = 1;

  fdd_set_data( d, FDD_LOAD_FACT );
  return d->status = DISK_OK;
}

void
fdd_unload( fdd_t *d )
{
  fdd_init( d, d->fdd_heads, d->fdd_cylinders );
}

/* change current head */
void
fdd_set_head( fdd_t *d, int head )
{
  if( d->fdd_heads == 1 )
    return;

  if( head > 0 )
    d->c_head = 1;
  else if( head < 1 )
    d->c_head = 0;

  fdd_set_data( d, FDD_HEAD_FACT );
}

/* change current track dir = 1 / -1 */
void
fdd_step( fdd_t *d )
{
  if( d->direction < 1 ) {	/* STEP_OUT */
    if( d->c_cylinder > 0 )
      d->c_cylinder--;
  } else if( d->direction > 0 ) {	/* STEP_IN */
    if( d->c_cylinder < d->fdd_cylinders - 1 )
      d->c_cylinder++;
  }
  if( d->c_cylinder == 0 )
    d->tr00 = 1;
  else
    d->tr00 = 0;

  fdd_set_data( d, FDD_STEP_FACT );
}

/*
 *
 * read/write next byte from/to sector
 *
 */
int
fdd_read_write_data( fdd_t *d, fdd_write_t write )
{

  if( !d->loaded )
    return d->status = FDD_OK;
  
  if( d->disk->i >= d->disk->bpt ) {		/* next data byte */
    d->disk->i = 0;
  }
  if( d->disk->track == NULL ) {
    if( !write )
      d->data = 0x100;				/* no data */
    d->disk->i++;
    d->index = d->disk->i >= d->disk->bpt ? 1 : 0;
    return d->status = FDD_OK;
  }

  if( write ) {
    if( d->disk->wrprot ) {
      d->disk->i++;
      d->index = d->disk->i >= d->disk->bpt ? 1 : 0;
      return d->status = FDD_RDONLY;
    }
    d->disk->track[d->disk->i] = d->data & 0x00ff;
    if( d->data & 0xff00 )
      d->disk->clocks[d->disk->i / 8 ] |= 1 << ( d->disk->i % 8 );
    else
      d->disk->clocks[d->disk->i / 8 ] &= ~( 1 << ( d->disk->i % 8 ) );
    d->disk->dirty = 1;
  } else {	/* read */
    d->data = d->disk->track[d->disk->i];
    if( d->disk->clocks[d->disk->i / 8 ] & ( 1 << ( d->disk->i % 8 ) ) )
      d->data |= 0xff00;
  }
  d->disk->i++;
  d->index = d->disk->i >= d->disk->bpt ? 1 : 0;

  return d->status = FDD_OK;
}

void
fdd_wait_index_hole( fdd_t *d )
{
  if( !d->loaded )
    return;

  d->disk->i = 0;
  d->index = 1;
}
