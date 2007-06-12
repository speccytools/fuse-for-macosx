/* wd1770.c: Routines for handling the WD1770 floppy disk controller
   Copyright (c) 2002-2007 Stuart Brady, Fredrick Meunier, Philip Kendall,
   Dmitry Sanarin

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

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

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

char secbuf[1024];

void
statusbar_update( int busy )
{
  ui_statusbar_update( UI_STATUSBAR_ITEM_DISK,
		       busy ? UI_STATUSBAR_STATE_ACTIVE :
			      UI_STATUSBAR_STATE_INACTIVE );
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

  if( track < d->track ) {
    f->direction = -1;
    if( f->track_register == 0 )
      f->track_register = 255;
    else if( update ) {
      int trk = f->track_register;

      trk += track - d->track + 256;
      trk %= 256;
      f->track_register = trk;
    }
  } else if( track > d->track ) {
    f->direction = 1;
    if( f->track_register == 255 )
      f->track_register = 0;
    else if( update ) {
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

  if( track < 0 )
    track = 0;
  if( track > 255 )
    track = 255;
  d->track = track;

  if( d->track == 0 )
    f->status_register |= WD1770_SR_LOST;
  else
    f->status_register &= ~WD1770_SR_LOST;
}

libspectrum_byte
wd1770_sr_read( wd1770_fdc *f )
{
  wd1770_drive *d = f->current_drive;

  f->status_register &= ~( WD1770_SR_MOTORON | WD1770_SR_SPINUP |
			   WD1770_SR_WRPROT | WD1770_SR_CRCERR );
  if( f->status_type == wd1770_status_type1 ) {
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
  if( ( b & 0xf0 ) == 0xd0 ) {                  /* Type IV - Force Interrupt */
    statusbar_update(0);
    f->status_register &= ~WD1770_SR_BUSY;
    f->status_register &= ~WD1770_SR_WRPROT;
    f->status_register &= ~WD1770_SR_CRCERR;
    f->status_register &= ~WD1770_SR_IDX_DRQ;
    f->state = wd1770_state_none;
    f->status_type = wd1770_status_type1;
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
    int rate   = b & 0x03 ? 1 : 0;

    switch( ( b >> 5 ) & 0x03 ) {
    case 0x00:
      if( ( b & 0x4 ) ) {                               /* Restore */
	f->track_register = d->track;
	wd1770_seek( f, 0, update, verify );
      } else {                                          /* Seek */
	wd1770_seek( f, f->data_register, 0, verify );
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
    f->status_type = wd1770_status_type1;
    wd1770_set_cmdint( f );
    wd1770_reset_datarq( f );
  } else if( !( b & 0x40 ) ) {                  /* Type II */
    int multisector = b & 0x10 ? 1 : 0;
    int delay       = b & 0x04 ? 1 : 0;

    if( !( b & 0x20 ) ) {                               /* Read Sector */
      f->state = wd1770_state_read;
    } else {                                            /* Write Sector */
      int dammark = b & 0x01;

      f->state = wd1770_state_write;
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
      f->status_type = wd1770_status_type2;
      f->data_track = d->track;
      f->data_sector = f->sector_register;
      f->data_side = d->side;
      f->data_offset = 0;
      f->data_multisector = multisector;
    }
  } else if( ( b & 0xf0 ) != 0xd0 ) {           /* Type III */
    int delay = b & 0x04;

    switch( b & 0xf0 ) {
    case 0x00:                                          /* Read Track */
      fprintf( stderr, "read track not yet implemented\n" );
      break;
    case 0x01:                                          /* Write Track */
      fprintf( stderr, "write track not yet implemented\n" );
      break;
    case 0x02:                                          /* Read Address */
      fprintf( stderr, "read address not yet implemented\n" );
      break;
    }
    f->status_type = wd1770_status_type2;
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

  if( f->state == wd1770_state_read ) {
    if( !d->disk ||
	f->data_sector >= d->geom.dg_secbase + d->geom.dg_sectors ||
	f->data_track >= d->geom.dg_cylinders ||
	f->data_side >= d->geom.dg_heads ) {
      f->status_register |= WD1770_SR_RNF;
      fprintf( stderr, "read from non-existent sector\n" );
      return f->data_register;
    }

    if( f->data_offset == 0 &&
	( d->geom.dg_secsize > sizeof( secbuf ) ||
	  dsk_pread( d->disk, &d->geom, secbuf, f->data_track,
		     f->data_side, f->data_sector ) != DSK_ERR_OK ) ) {
      f->status_register |= WD1770_SR_RNF;
      statusbar_update(0);
      f->status_register &= ~WD1770_SR_BUSY;
      f->status_type = wd1770_status_type2;
      f->state = wd1770_state_none;
      wd1770_set_cmdint( f );
      wd1770_reset_datarq( f );
    } else if( f->data_offset < d->geom.dg_secsize ) {
      f->data_register = secbuf[ f->data_offset++ ];
      if( f->data_offset == d->geom.dg_secsize ) {
	if( f->data_multisector &&
	    f->data_sector < d->geom.dg_secbase + d->geom.dg_sectors ) {
	  f->data_sector++;
	  f->data_offset = 0;
	}
	if( !f->data_multisector ||
	    f->data_sector >= d->geom.dg_secbase + d->geom.dg_sectors ) {
	  statusbar_update(0);
	  f->status_register &= ~WD1770_SR_BUSY;
	  f->status_type = wd1770_status_type2;
	  f->state = wd1770_state_none;
	  wd1770_set_cmdint( f );
	  wd1770_reset_datarq( f );
	}
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
  if( f->state == wd1770_state_write ) {
    if( !d->disk || f->data_sector >= d->geom.dg_secbase + d->geom.dg_sectors ||
	f->data_track >= d->geom.dg_cylinders ||
	f->data_side >= d->geom.dg_heads ) {
      f->status_register |= WD1770_SR_RNF;
      fprintf( stderr, "write to non-existent sector\n" );
      return;
    }

    if( f->data_offset < sizeof( secbuf ) )
      secbuf[ f->data_offset++ ] = b;
    if( f->data_offset == d->geom.dg_secsize ) {
      dsk_pwrite( d->disk, &d->geom, secbuf, f->data_track,
		  f->data_side, f->data_sector );
      if( f->data_multisector &&
	  f->data_sector < d->geom.dg_secbase + d->geom.dg_sectors ) {
	f->data_sector++;
	f->data_offset = 0;
      }
      if( !f->data_multisector ||
	  f->data_sector >= d->geom.dg_secbase + d->geom.dg_sectors ) {
	statusbar_update(0);
	f->status_register &= ~WD1770_SR_BUSY;
	f->status_type = wd1770_status_type2;
	f->state = wd1770_state_none;
	wd1770_set_cmdint( f );
	wd1770_reset_datarq( f );
      }
    }
  }
}
