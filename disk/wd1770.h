/* wd1770.h: Routines for handling the WD1770 floppy disk controller
   Copyright (c) 2003-2007 Stuart Brady, Fredrick Meunier, Philip Kendall,
   Gergely Szasz

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

#ifndef FUSE_WD1770_H
#define FUSE_WD1770_H

#include <libdsk.h>

#include <libspectrum.h>

#include "../fuse.h"

static const int WD1770_SR_MOTORON = 1<<7; /* Motor on */
static const int WD1770_SR_WRPROT  = 1<<6; /* Write-protect */
static const int WD1770_SR_SPINUP  = 1<<5; /* Record type / Spin-up complete */
static const int WD1770_SR_RNF     = 1<<4; /* Record Not Found */
static const int WD1770_SR_CRCERR  = 1<<3; /* CRC error */
static const int WD1770_SR_LOST    = 1<<2; /* Lost data */
static const int WD1770_SR_IDX_DRQ = 1<<1; /* Index pulse / Data request */
static const int WD1770_SR_BUSY    = 1<<0; /* Busy (command under execution) */

extern int wd1770_index_pulse;
extern int wd1770_index_interrupt;

typedef struct wd1770_drive {
  DSK_PDRIVER disk;
  DSK_GEOMETRY geom;
  char filename[ PATH_MAX ];

  int index_pulse;
  int index_interrupt;

  int track;
  int side;
} wd1770_drive;

typedef struct wd1770_fdc {
  wd1770_drive *current_drive;

  int rates[ 4 ];
  int spin_cycles;
  int direction;                /* 0 = spindlewards, 1 = rimwards */

  enum wd1770_state {
    WD1770_STATE_NONE = 0,
    WD1770_STATE_SEEK,
    WD1770_STATE_READ,
    WD1770_STATE_WRITE,
    WD1770_STATE_READTRACK,
    WD1770_STATE_WRITETRACK,
    WD1770_STATE_READID,
  } state;

  enum wd1770_status_type {
    WD1770_STATUS_TYPE1,
    WD1770_STATUS_TYPE2,
  } status_type;

  /* state during transfer */
  int data_track;
  int data_sector;
  int data_side;
  int data_multisector;
  int data_offset;

  libspectrum_byte status_register;     /* status register */
  libspectrum_byte track_register;      /* track register */
  libspectrum_byte sector_register;     /* sector register */
  libspectrum_byte data_register;       /* data register */

  void ( *set_cmdint ) ( struct wd1770_fdc *f );
  void ( *reset_cmdint ) ( struct wd1770_fdc *f );
  void ( *set_datarq ) ( struct wd1770_fdc *f );
  void ( *reset_datarq ) ( struct wd1770_fdc *f );
  void *iface;

  DSK_GEOMETRY fgeom;		/* geom for write track */
  int fill;			/* fill byte for format */
  int idx;
  enum wd1770_trcmd_state {
    WD1770_TRACK_NONE = 0,
    WD1770_TRACK_GAP1,
    WD1770_TRACK_GAP2,
    WD1770_TRACK_SYNC,
    WD1770_TRACK_IMRK,

    WD1770_TRACK_ID_TRACK,
    WD1770_TRACK_ID_HEAD,
    WD1770_TRACK_ID_SECTOR,
    WD1770_TRACK_ID_SIZE,

    WD1770_TRACK_CRC,
    WD1770_TRACK_GAP3,
    WD1770_TRACK_DMRK,
    WD1770_TRACK_DATA,
    WD1770_TRACK_GAP4,
    WD1770_TRACK_FILL,
  } trcmd_state;
} wd1770_fdc;

libspectrum_byte wd1770_sr_read( wd1770_fdc *f );
void wd1770_cr_write( wd1770_fdc *f, libspectrum_byte b );

libspectrum_byte wd1770_tr_read( wd1770_fdc *f );
void wd1770_tr_write( wd1770_fdc *f, libspectrum_byte b );

libspectrum_byte wd1770_sec_read( wd1770_fdc *f );
void wd1770_sec_write( wd1770_fdc *f, libspectrum_byte b );

libspectrum_byte wd1770_dr_read( wd1770_fdc *f );
void wd1770_dr_write( wd1770_fdc *f, libspectrum_byte b );

void wd1770_set_cmdint( wd1770_fdc *f );
void wd1770_reset_cmdint( wd1770_fdc *f );
void wd1770_set_datarq( wd1770_fdc *f );
void wd1770_reset_datarq( wd1770_fdc *f );

#endif                  /* #ifndef FUSE_WD1770_H */
