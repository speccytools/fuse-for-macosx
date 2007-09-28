/* wd1770.c: Routines for handling the WD1770 floppy disk controller
   Copyright (c) 2002-2007 Stuart Brady, Fredrick Meunier, Philip Kendall,
   Dmitry Sanarin, Gergely Szasz

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

   Stuart: sdbrady@ntlworld.com

*/

#include <config.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>            /* Needed for strncasecmp() on QNX6 */
#endif                          /* #ifdef HAVE_STRINGS_H */
#include <limits.h>
#include <sys/stat.h>

#include <libdsk.h>

#include <libspectrum.h>

#include "compat.h"
#include "event.h"
#include "machine.h"
#include "ui/ui.h"
#include "wd1770.h"
#include "z80/z80.h"

static void statusbar_update( int busy );
static void next_sector( wd1770_drive *d );
static int check_track( wd1770_fdc *f );
static int check_sector( wd1770_fdc *f );
static int read_sector( wd1770_fdc *f );
static int write_sector( wd1770_fdc *f );

static char secbuf[1024];

static void
statusbar_update( int busy )
{
  ui_statusbar_update( UI_STATUSBAR_ITEM_DISK,
		       busy ? UI_STATUSBAR_STATE_ACTIVE :
			      UI_STATUSBAR_STATE_INACTIVE );
}

static void
next_sector( wd1770_drive *d )
{
  int s = d->sector;

  s -= d->geom.dg_secbase;
  s++;
  s %= d->geom.dg_sectors;
  s += d->geom.dg_secbase;

  d->sector = s;
}

static int
check_track( wd1770_fdc *f )
{
  wd1770_drive *d = f->current_drive;

  if( !d->disk ||
      d->track >= d->geom.dg_cylinders ||
      f->data_side >= d->geom.dg_heads ) {
    fprintf( stderr, "access to non-existent track\n" );
    return 0;
  }

  return 1;
}

static int
check_sector( wd1770_fdc *f )
{
  wd1770_drive *d = f->current_drive;

  if( !check_track( f ) )
    return 0;

  if( d->sector < d->geom.dg_secbase ||
      d->sector >= d->geom.dg_secbase + d->geom.dg_sectors) {
    fprintf( stderr, "access to non-existent sector\n" );
    return 0;
  }

  if( d->geom.dg_secsize > sizeof( secbuf ) ) {
    fprintf( stderr, "sector too large for buffer\n" );
    return 0;
  }

  return 1;
}

static int
read_sector( wd1770_fdc *f )
{
  wd1770_drive *d = f->current_drive;

  if( !check_sector( f ) )
    return 0;

  if( dsk_pread( d->disk, &d->geom, secbuf, d->track,
		 f->data_side, d->sector ) != DSK_ERR_OK ) {
    return 0;
  }

  return 1;
}

static int
write_sector( wd1770_fdc *f )
{
  wd1770_drive *d = f->current_drive;

  if( !check_sector( f ) )
    return 0;

  if( dsk_pwrite( d->disk, &d->geom, secbuf, d->track,
		  f->data_side, d->sector ) != DSK_ERR_OK ) {
    return 0;
  }

  return 1;
}

void
wd1770_set_cmdint( wd1770_fdc *f )
{
  if( f->set_cmdint )
    f->set_cmdint( f );
}

void
wd1770_reset_cmdint( wd1770_fdc *f )
{
  if( f->reset_cmdint )
    f->reset_cmdint( f );
}

void
wd1770_set_datarq( wd1770_fdc *f )
{
  f->status_register |= WD1770_SR_IDX_DRQ;
  if( f->set_datarq )
    f->set_datarq( f );
}

void
wd1770_reset_datarq( wd1770_fdc *f )
{
  f->status_register &= ~WD1770_SR_IDX_DRQ;
  if( f->reset_datarq )
    f->reset_datarq( f );
}

static void
wd1770_seek( wd1770_fdc *f, int track, int update, int verify )
{
  wd1770_drive *d = f->current_drive;

  if( track < 0 )
    track = 0;
  if( track > 255 )
    track = 255;

  if( track < d->track ) {
    f->direction = -1;
    if( update ) {
      int trk = f->track_register;

      trk += track - d->track + 256;
      trk %= 256;
      f->track_register = trk;
    }
  } else if( track > d->track ) {
    f->direction = 1;
    if( update ) {
      int trk = f->track_register;

      trk += track - d->track;
      trk %= 256;
      f->track_register = trk;
    }
  }

  if( verify ) {
    if( track < 0 )
      f->status_register |= WD1770_SR_RNF;
    else if( track >= d->geom.dg_cylinders )
      f->status_register |= WD1770_SR_RNF;
    else
      f->status_register &= ~WD1770_SR_RNF;
  } else
    f->status_register &= ~WD1770_SR_RNF;

  if( track == 0 )
    f->status_register |= WD1770_SR_LOST;
  else
    f->status_register &= ~WD1770_SR_LOST;

  d->track = track;
}

libspectrum_byte
wd1770_sr_read( wd1770_fdc *f )
{
  wd1770_drive *d = f->current_drive;

  f->status_register &= ~( WD1770_SR_MOTORON | WD1770_SR_SPINUP |
			   WD1770_SR_WRPROT | WD1770_SR_CRCERR );
  if( f->status_type == WD1770_STATUS_TYPE1 ) {
    f->status_register &= ~WD1770_SR_IDX_DRQ;
    if( !d->disk || d->index_pulse )
      f->status_register |= WD1770_SR_IDX_DRQ;
  }
  return f->status_register;
}


void
wd1770_cr_write( wd1770_fdc *f, libspectrum_byte b )
{
  wd1770_drive *d = f->current_drive;

  /* command register: */
  if( ( b & 0xf0 ) == 0xd0 ||
      ( b & 0xf1 ) == 0xf1 ) {                  /* Type IV - Force Interrupt */
    statusbar_update(0);
    f->status_register &= ~WD1770_SR_BUSY;
    f->status_register &= ~WD1770_SR_WRPROT;
    f->status_register &= ~WD1770_SR_CRCERR;
    f->status_register &= ~WD1770_SR_IDX_DRQ;
    f->state = WD1770_STATE_NONE;
    f->status_type = WD1770_STATUS_TYPE1;
    if( d->track == 0 )
      f->status_register |= WD1770_SR_LOST;
    else
      f->status_register &= ~WD1770_SR_LOST;
    wd1770_reset_datarq( f );
    if( b & 0x08 )
      wd1770_set_cmdint( f );
    if( b & 0x04 )
      d->index_interrupt = 1;
    return;
  }

  if( f->status_register & WD1770_SR_BUSY )
    return;

  /*
  f->status_register |= WD1770_SR_BUSY;
  event_add( tstates +
    10 * machine_current->timings.processor_speed / 1000,
    EVENT_TYPE_PLUSD_CMD_DONE );
  */

  if( !( b & 0x08 ) ) {
    f->spin_cycles = 5;
    f->status_register |= WD1770_SR_MOTORON;
    f->status_register |= WD1770_SR_SPINUP;
  }

  if( !( b & 0x80 ) ) {                         /* Type I */
    int update = b & 0x10 ? 1 : 0;
    int verify = b & 0x04 ? 1 : 0;
/*  int rate   = b & 0x03 ? 1 : 0; */

    switch( ( b >> 5 ) & 0x03 ) {
    case 0x00:
      if( !( b & 0x10 ) ) {                             /* Restore */
	f->track_register = d->track;
	wd1770_seek( f, 0, 1, verify );
	f->data_register = 0;
      } else {                                          /* Seek */
	wd1770_seek( f, d->track + f->data_register - f->track_register,
		     1, verify );
	f->data_register = f->track_register;
      }
      f->direction = 1;
      break;
    case 0x01:                                          /* Step */
      wd1770_seek( f, d->track + f->direction, update, verify );
      break;
    case 0x02:                                          /* Step In */
      wd1770_seek( f, d->track + 1, update, verify );
      break;
    case 0x03:                                          /* Step Out */
      wd1770_seek( f, d->track - 1, update, verify );
      break;
    }
    f->status_type = WD1770_STATUS_TYPE1;
    wd1770_set_cmdint( f );
    wd1770_reset_datarq( f );
  } else if( !( b & 0x40 ) ) {                  /* Type II */
    int multisector = b & 0x10 ? 1 : 0;
/*  int delay       = b & 0x04 ? 1 : 0; */

    if( !( b & 0x20 ) ) {                               /* Read Sector */
      f->state = WD1770_STATE_READ;
    } else {                                            /* Write Sector */
/*    int dammark = b & 0x01; */

      f->state = WD1770_STATE_WRITE;
    }
    if( f->sector_register < d->geom.dg_secbase ||
	f->sector_register >= d->geom.dg_secbase + d->geom.dg_sectors ||
	d->track >= d->geom.dg_cylinders ||
	d->track < 0 ) {
      f->status_register |= WD1770_SR_RNF;
      wd1770_set_cmdint( f );
      wd1770_reset_datarq( f );
    } else {
      wd1770_set_datarq( f );
      statusbar_update(1);
      f->status_register |= WD1770_SR_BUSY;
      f->status_register &= ~( WD1770_SR_WRPROT | WD1770_SR_RNF |
			       WD1770_SR_CRCERR | WD1770_SR_LOST );
      f->status_type = WD1770_STATUS_TYPE2;
      d->sector = f->sector_register;
      f->data_side = d->side;
      f->data_offset = 0;
      f->data_multisector = multisector;
    }
  } else if( ( b & 0x30 ) != 0x10 ) {           /* Type III */
/*  int delay = b & 0x04; */

    switch( b & 0x30 ) {
    case 0x00:                                          /* Read Address */
      f->state = WD1770_STATE_READID;
      if( d->track >= d->geom.dg_cylinders ||
	  d->track < 0 ) {
	f->status_register |= WD1770_SR_RNF;
	wd1770_set_cmdint( f );
	wd1770_reset_datarq( f );
      } else {
	wd1770_set_datarq( f );
	statusbar_update(1);
	f->status_register |= WD1770_SR_BUSY;
	f->status_register &= ~( WD1770_SR_WRPROT | WD1770_SR_RNF |
				 WD1770_SR_CRCERR | WD1770_SR_LOST );
	f->data_side = d->side;
	f->data_offset = 0;
      }
      break;
    case 0x20:                                          /* Read Track */
      fprintf( stderr, "read track not yet implemented\n" );
      break;
    case 0x30:                                          /* Write Track */
      f->state = WD1770_STATE_WRITETRACK;
      f->trcmd_state = WD1770_TRACK_NONE;
      if( !check_track( f ) ) {
	f->status_register |= WD1770_SR_RNF;
	wd1770_set_cmdint( f );
	wd1770_reset_datarq( f );
      } else {
	wd1770_set_datarq( f );
	statusbar_update(1);
	f->status_register |= WD1770_SR_BUSY;
	f->status_register &= ~( WD1770_SR_WRPROT | WD1770_SR_RNF |
				 WD1770_SR_CRCERR | WD1770_SR_LOST );
	d->sector = d->geom.dg_secbase;
	f->data_side = d->side;
	f->data_offset = 0;
      }
      break;
    }
    f->status_type = WD1770_STATUS_TYPE2;
  }
}

libspectrum_byte
wd1770_tr_read( wd1770_fdc *f )
{
  return f->track_register;
}

void
wd1770_tr_write( wd1770_fdc *f, libspectrum_byte b )
{
  f->track_register = b;
}

libspectrum_byte
wd1770_sec_read( wd1770_fdc *f )
{
  return f->sector_register;
}

void
wd1770_sec_write( wd1770_fdc *f, libspectrum_byte b )
{
  f->sector_register = b;
}

libspectrum_byte
wd1770_dr_read( wd1770_fdc *f )
{
  wd1770_drive *d = f->current_drive;

  if( f->state == WD1770_STATE_READ ) {
    if( !read_sector( f ) ) {
      f->status_register |= WD1770_SR_RNF;
      statusbar_update(0);
      f->status_register &= ~WD1770_SR_BUSY;
      f->status_type = WD1770_STATUS_TYPE2;
      f->state = WD1770_STATE_NONE;
      wd1770_set_cmdint( f );
      wd1770_reset_datarq( f );
    } else if( f->data_offset < d->geom.dg_secsize ) {
      f->data_register = secbuf[ f->data_offset++ ];
      if( f->data_offset == d->geom.dg_secsize ) {
	next_sector( d );
	f->data_offset = 0;
	if( !f->data_multisector ||
	    d->sector >= d->geom.dg_secbase + d->geom.dg_sectors ) {
	  statusbar_update(0);
	  f->status_register &= ~WD1770_SR_BUSY;
	  f->status_type = WD1770_STATUS_TYPE2;
	  f->state = WD1770_STATE_NONE;
	  wd1770_set_cmdint( f );
	  wd1770_reset_datarq( f );
	}
      }
    }
  } else if( f->state == WD1770_STATE_READID ) {
    if( !check_track( f ) ) {
      f->status_register |= WD1770_SR_RNF;
      statusbar_update(0);
      f->status_register &= ~WD1770_SR_BUSY;
      f->status_type = WD1770_STATUS_TYPE2;
      f->state = WD1770_STATE_NONE;
      wd1770_set_cmdint( f );
      wd1770_reset_datarq( f );
    } else {
      switch( f->data_offset++ ) {
      case 0:
	f->data_register = d->track;
	break;
      case 1:
	f->data_register = f->data_side;
	break;
      case 2:
	f->data_register = d->sector;
	break;
      case 3:
	f->data_register = 0;
	if( d->geom.dg_secsize > 128 ) f->data_register++;
	if( d->geom.dg_secsize > 256 ) f->data_register++;
	if( d->geom.dg_secsize > 512 ) f->data_register++;
	break;
      case 4:
	f->data_register = 0; /* CRC */
	break;
      case 5:
	f->data_register = 0; /* CRC */

	/* finished */
	next_sector( d );
	statusbar_update(0);
	f->status_register &= ~WD1770_SR_BUSY;
	f->status_type = WD1770_STATUS_TYPE2;
	f->state = WD1770_STATE_NONE;
	wd1770_set_cmdint( f );
	wd1770_reset_datarq( f );
	break;
      }
    }
  }
  return f->data_register;
}

void
wd1770_dr_write( wd1770_fdc *f, libspectrum_byte b )
{
  wd1770_drive *d = f->current_drive;

  f->data_register = b;
  if( f->state == WD1770_STATE_WRITE ) {
    if( !check_sector( f ) ) {
      f->status_register |= WD1770_SR_RNF;
      statusbar_update(0);
      f->status_register &= ~WD1770_SR_BUSY;
      f->status_type = WD1770_STATUS_TYPE2;
      f->state = WD1770_STATE_NONE;
      wd1770_set_cmdint( f );
      wd1770_reset_datarq( f );
    } else if( f->data_offset < sizeof( secbuf ) ) {
      secbuf[ f->data_offset++ ] = b;
      if( f->data_offset == d->geom.dg_secsize ) {
	if( !write_sector( f ) ) {
	  f->status_register |= WD1770_SR_RNF;
	  statusbar_update(0);
	  f->status_register &= ~WD1770_SR_BUSY;
	  f->status_type = WD1770_STATUS_TYPE2;
	  f->state = WD1770_STATE_NONE;
	  wd1770_set_cmdint( f );
	  wd1770_reset_datarq( f );
	} else {
	  next_sector( d );
	  f->data_offset = 0;
	  if( !f->data_multisector ||
	      d->sector >= d->geom.dg_secbase + d->geom.dg_sectors ) {
	    statusbar_update(0);
	    f->status_register &= ~WD1770_SR_BUSY;
	    f->status_type = WD1770_STATUS_TYPE2;
	    f->state = WD1770_STATE_NONE;
	    wd1770_set_cmdint( f );
	    wd1770_reset_datarq( f );
	  }
	}
      }
    }
  } else if( f->state == WD1770_STATE_WRITETRACK ) {
    if( !d->disk || 
	d->track >= d->geom.dg_cylinders ||
	f->data_side >= d->geom.dg_heads ) {
      f->status_register |= WD1770_SR_RNF;
      fprintf( stderr, "writetrack to non-existent track\n" );
      return;
    }
    switch( f->trcmd_state ) {
    case WD1770_TRACK_NONE:
      statusbar_update(1);
      f->fgeom = d->geom;		/* set up formattng geometry */
      f->trcmd_state = WD1770_TRACK_GAP1;
      break;
    case WD1770_TRACK_GAP1:
      if( b == 0xfe )				/* received ID MARK */
	f->trcmd_state = WD1770_TRACK_IMRK;	/* side/sect/len */
      break;
    case WD1770_TRACK_IMRK:
      f->trcmd_state = WD1770_TRACK_ID_HEAD;	/* track number */
      break;
    case WD1770_TRACK_ID_HEAD:
      if( f->fgeom.dg_heads < b + 1 )		/* setup heads */
	f->fgeom.dg_heads = b + 1;
      f->trcmd_state = WD1770_TRACK_ID_SECTOR;	/* sector number */
      break;
    case WD1770_TRACK_ID_SECTOR:
      if( f->fgeom.dg_sectors < b )		/* setup sectors */
	f->fgeom.dg_sectors = b;
      f->trcmd_state = WD1770_TRACK_ID_SIZE;	/* size of sector */
      break;
    case WD1770_TRACK_ID_SIZE:
      if( f->fgeom.dg_secsize < ( 0x80 << b ) )
	f->fgeom.dg_secsize = ( 0x80 << b );
      f->trcmd_state = WD1770_TRACK_DMRK;	/* sector length */
      break;
    case WD1770_TRACK_DMRK:	/* waiting for DMRK */
      if( b == 0xfb || b == 0xf8 ) {
	f->trcmd_state = WD1770_TRACK_DATA;
	f->fill = 0xf7;		/* 0xf7 = CRC so never be a fill byte */
      }
      break;
    case WD1770_TRACK_DATA:	/* wait for next GAP */
      if( f->fill == 0xf7 )
	f->fill = b;		/* set up fill byte */
      if( b == 0x4e ) {
	f->trcmd_state = WD1770_TRACK_GAP4;
	f->idx = 128;		/* wait for max 64 GAP... */
      }
      break;
    case WD1770_TRACK_GAP4:
      if( b == 0x00 )		/* new sector PLL */
	f->trcmd_state = WD1770_TRACK_GAP1;
      if( !( f->idx-- ) )
	f->trcmd_state = WD1770_TRACK_FILL;
      break;
    case WD1770_TRACK_FILL:	/* now we actually format the track */
      if( f->fgeom.dg_heads > 2 || f->fgeom.dg_sectors > 21 ||
	    f->fgeom.dg_secsize > 1024 ) {
	fprintf( stderr, "problem during formatting track %d, wrong id data:\n", d->track );
	fprintf( stderr, "    t:%d h:%d spt:%d bps:%d\n", d->track,
				    f->fgeom.dg_heads, f->fgeom.dg_sectors,
				    (int)f->fgeom.dg_secsize );
      } else {
	if( dsk_apform( d->disk, &f->fgeom, d->track,
			f->data_side, f->fill ) != DSK_ERR_OK )
	fprintf( stderr, "problem during formatting track %d\n", d->track );
      }
      statusbar_update(0);

      f->status_register &= ~WD1770_SR_BUSY;
      f->status_type = WD1770_STATUS_TYPE2;
      f->state = WD1770_STATE_NONE;
      wd1770_set_cmdint( f );
      wd1770_reset_datarq( f );
      break;
    default:
      break;
    }
  }
}
