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

#include <libspectrum.h>

#include "fdd.h"
#include "fuse.h"

static const int WD1770_SR_MOTORON = 1<<7; /* Motor on */
static const int WD1770_SR_WRPROT  = 1<<6; /* Write-protect */
static const int WD1770_SR_SPINUP  = 1<<5; /* Record type / Spin-up complete */
static const int WD1770_SR_RNF     = 1<<4; /* Record Not Found */
static const int WD1770_SR_CRCERR  = 1<<3; /* CRC error */
static const int WD1770_SR_LOST    = 1<<2; /* Lost data */
static const int WD1770_SR_IDX_DRQ = 1<<1; /* Index pulse / Data request */
static const int WD1770_SR_BUSY    = 1<<0; /* Busy (command under execution) */

typedef enum wd_type_t {
  WD1773 = 0,		/* WD1773 */
  FD1793,
  WD1770,
  WD1772,
} wd_type_t;

extern int wd1770_index_pulse;
extern int wd1770_index_interrupt;

typedef struct wd1770_drive {
  fdd_t fdd;			/* floppy disk drive */
  disk_t disk;			/* the floppy disk itself */
  int index_pulse;
  int index_interrupt;

} wd1770_drive;

typedef struct wd1770_fdc {
  wd1770_drive *current_drive;

  wd_type_t type;		/* WD1770, WD1772, WD1773 */
  
  int rates[ 4 ];
  int spin_cycles;
  int direction;                /* 0 = spindlewards, 1 = rimwards */
  int dden;			/* SD/DD -> FM/MFM */
  int intrq;			/* INTRQ line status */
  int head_load;		/* WD1773/FD1793 */

  enum wd1770_state {
    WD1770_STATE_NONE = 0,
    WD1770_STATE_SEEK,
    WD1770_STATE_SEEK_DELAY,
    WD1770_STATE_VERIFY,
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

  enum wd1770_am_type {
    WD1770_AM_NONE = 0,
    WD1770_AM_INDEX,
    WD1770_AM_ID,
    WD1770_AM_DATA,
  } id_mark;

  int id_track;
  int id_head;
  int id_sector;
  int id_length;		/* sector length code 0, 1, 2, 3 */
  int sector_length;		/* sector length from length code */
  int ddam;			/* read a deleted data mark */
  int rev;			/* revolution counter */

  /* state during transfer */
  int data_check_head;		/* -1 no check, 0/1 wait side 0 or 1 */
  int data_multisector;
  int data_offset;

  libspectrum_byte command_register;    /* command register */
  libspectrum_byte status_register;     /* status register */
  libspectrum_byte track_register;      /* track register */
  libspectrum_byte sector_register;     /* sector register */
  libspectrum_byte data_register;       /* data register */

  libspectrum_word crc;			/* to hold crc */

  void ( *set_cmdint ) ( struct wd1770_fdc *f );
  void ( *reset_cmdint ) ( struct wd1770_fdc *f );
  void ( *set_datarq ) ( struct wd1770_fdc *f );
  void ( *reset_datarq ) ( struct wd1770_fdc *f );
  void *iface;

} wd1770_fdc;

/* allocate an fdc */
wd1770_fdc *wd1770_alloc_fdc( wd_type_t type );
void wd1770_master_reset( wd1770_fdc *f );

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
