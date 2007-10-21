/* disk.c: Routines for handling disk images
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libspectrum.h>

#include "bitmap.h"
#include "crc.h"
#include "disk.h"

/* The ordering of these strings must match the order of the
 * the disk_error_t enumeration in disk.h */

static const char *disk_error[] = {
  "OK",				/* DISK_OK */
  "Feature not implemented",	/* DISK_IMPL */
  "Out of memory",		/* DISK_MEM */
  "Invalid disk geometry",	/* DISK_GEOM */
  "Cannot open disk image",	/* DISK_OPEN */
  "Unsupported file feature",	/* DISK_UNSUP */
  "Read only disk",		/* DISK_RDONLY */
  "Cannot close file",		/* DISK_CLOSE */
  "Cannot write disk image",	/* DISK_WRFILE */
  "Partially written file",	/* DISK_WRPART */
  
  "Unknown error code"		/* DISK_LAST_ERROR */
};

static const int disk_bpt[] = {
  6250,				/* AUTO assumes DD */
  5208,				/* 8" SD */
  10416,			/* 8" DD */
  3125,				/* SD */
  6250,				/* DD */
  12500,			/* HD */
};

static unsigned char head[256];

typedef struct disk_gap_t {
  int gap;			/* gap byte */
  int sync;			/* sync byte */
  int sync_len;
  int len[4];
} disk_gap_t;

disk_gap_t gaps[] = {
  { 0x4e, 0x00, 12, {  0, 60, 22, 24 } },			/* MGT MFM */
  { 0x4e, 0x00, 12, {  0, 10, 22, 60 } },			/* TRD MFM */
  { 0xff, 0x00,  6, { 40, 26, 11, 27 } },			/* IBM3740 FM */
  { 0x4e, 0x00, 12, { 80, 50, 22, 54 } },			/* IBM34 MFM */
  { 0xff, 0x00,  6, {  0, 16, 11, 10 } },			/* MINIMAL FM */
  { 0x4e, 0x00, 12, {  0, 32, 22, 24 } },			/* MINIMAL MFM */
};

#define GAP_MGT_PLUSD	0
#define GAP_TRDOS	1
#define GAP_IBM3740	2
#define GAP_IBM34	3
#define GAP_MINIMAL_FM  4
#define GAP_MINIMAL_MFM 5

const char *
disk_strerror( int error )
{
  if( error < 0 || error > DISK_LAST_ERROR )
    error = DISK_LAST_ERROR;
  return disk_error[ error ];
}

static int
id_read( disk_t *d, int *head, int *track, int *sector, int *length )
{
  int a1mark = 0;
  
  while( d->i < d->bpt ) {
    if( d->track[ d->i ] == 0xa1 && 
	bitmap_test( d->clocks, d->i ) ) {		/* 0xa1 with clock */
      a1mark = 1;
    } else if( d->track[ d->i ] == 0xfe && 
	( bitmap_test( d->clocks, d->i ) ||		/* 0xfe with clock */
	  a1mark ) ) {						/* or 0xfe with 0xa1 */
      d->i++;
      *track  = d->track[ d->i++ ];
      *head   = d->track[ d->i++ ];
      *sector = d->track[ d->i++ ];
      *length = d->track[ d->i++ ];
      d->i += 2;	/* skip CRC */
      return 1;
    } else {
      a1mark = 0;
    }
    d->i++;
  }
  return 0;
}

static int
datamark_read( disk_t *d, int *deleted )
{
  int a1mark = 0;

  while( d->i < d->bpt ) {
    if( d->track[ d->i ] == 0xa1 && 
	bitmap_test( d->clocks, d->i ) ) { /* 0xa1 with clock */
      a1mark = 1;
    } else if( d->track[ d->i ] >= 0xf8 && d->track[ d->i ] <= 0xfe &&
	       ( bitmap_test( d->clocks, d->i ) || a1mark ) ) {
      /* 0xfe with clock or 0xfe after 0xa1 mark */
      *deleted = d->track[ d->i ] == 0xf8 ? 1 : 0;
      d->i++;
      return 1;
    } else {
      a1mark = 0;
    }
    d->i++;
  }
  return 0;
}


static int
id_seek( disk_t *d, int sector )
{
  int h, t, s, b;
  d->i = 0;	/* start of the track */
  while( id_read( d, &h, &t, &s, &b ) ) {
    if( s == sector )
      return 1;
  }
  return 0;
}

/* seclen 00 -> 128, 01 -> 256 ... byte*/
static int
data_write_file( disk_t *d, FILE *file, int seclen )
{
  int len = 0x80 << seclen;
  if( fwrite( &d->track[ d->i ], len, 1, file ) != 1 )
    return 1;
  return 0;
}

static int
savetrack( disk_t *d, FILE *file, int head, int track, 
	    int sector_base, int sectors, int seclen )
{
  int s;
  int del;
  
  d->track = d->data + ( ( d->sides * track + head ) * d->tlen );
  d->clocks = d->track + d->bpt;
  d->i = 0;
  for( s = sector_base; s < sector_base + sectors; s++ ) {
    if( id_seek( d, s ) ) {
      if( datamark_read( d, &del ) ) {		/* write data if we have data */
	if( data_write_file( d, file, seclen ) )
	  return 1;
      }
    } else {
      return 1;
    }
  }
  return 0;
}

static int
saverawtrack( disk_t *d, FILE *file, int head, int track  )
{
  int h, t, s, seclen;
  int del;
  
  d->track = d->data + ( ( d->sides * track + head ) * d->tlen );
  d->clocks = d->track + d->bpt;
  d->i = 0;
  while( id_read( d, &h, &t, &s, &seclen ) ) {
    if( datamark_read( d, &del ) ) {		/* write data if we have data */
      if( data_write_file( d, file, seclen ) )
	return 1;
    }
  }
  return 0;
}

#define DISK_ID_NOTMATCH 1
#define DISK_SECLEN_VARI 2
#define DISK_SPT_VARI 4
#define DISK_SBASE_VARI 8
#define DISK_MFM_VARI 16
#define DISK_DDAM 32
#define DISK_CORRUPT_SECTOR 64

static int
guess_track_geom( disk_t *d, int head, int track, int *sector_base,
		    int *sectors, int *seclen, int *mfm )
{
  int r = 0;
  int h, t, s, sl;
  int del;
  *sector_base = -1;
  *sectors = 0;
  *seclen = -1;
  *mfm = -1;

  d->track = d->data + ( d->sides * track + head ) * d->tlen;
  d->clocks = d->track + d->bpt;
  d->i = 0;
  while( id_read( d, &h, &t, &s, &sl ) ) {
    if( *sector_base == -1 )
      *sector_base = s;
    if( *seclen == -1 )
      *seclen = sl;
    if( *mfm == -1 )
      *mfm = d->track[ d->i ] == 0x4e ? 1 : 0;	/* not so robust */
    if( !datamark_read( d, &del ) )
      r |= DISK_CORRUPT_SECTOR;
    if( /* h != head ||*/ t != track /* || s != *sectors */ )
      r |= DISK_ID_NOTMATCH;
    if( s < *sector_base )
      *sector_base = s;
    if( sl != *seclen ) {
      r |= DISK_SECLEN_VARI;
      if( sl > *seclen )
	*seclen = sl;
    }
    if( del )
      r |= DISK_DDAM;
    *sectors += 1;
  }
  return r;
}

static int
check_disk_geom( disk_t *d, int *sector_base, int *sectors,
		 int *seclen, int *mfm )
{
  int h, t, s, slen, sbase, m;
  int r = 0;

  d->track = d->data; d->clocks = d->track + d->bpt;
  d->i = 0;
  *sector_base = -1;
  *sectors = -1;
  *seclen = -1;
  *mfm = -1;
  for( t = 0; t < d->cylinders; t++ ) {
    for( h = 0; h < d->sides; h++ ) {
      r |= guess_track_geom( d, h, t, &sbase, &s, &slen, &m );
      if( *sector_base == -1 )
	*sector_base = sbase;
      if( *sectors == -1 )
	*sectors = s;
      if( *seclen == -1 )
	*seclen = slen;
      if( *mfm == -1 )
	*mfm = m;
      if( sbase != *sector_base ) {
	r |= DISK_SBASE_VARI;
	if( sbase < *sector_base )
	  *sector_base = sbase;
      }
      if( s != *sectors ) {
	r |= DISK_SPT_VARI;
	if( s > *sectors )
	  *sectors = s;
      }
      if( slen != *seclen ) {
	r |= DISK_SECLEN_VARI;
	if( slen > *seclen )
	  *seclen = slen;
      }
      if( m != *mfm ) {
	r |= DISK_MFM_VARI;
	*mfm = 1;
      }
    }
  }
  return r;
}

static int
gap_add( disk_t *d, int gap, int mark, int gaptype )
{
  disk_gap_t *g = &gaps[ gaptype ];
  if( d->i + g->sync_len + g->len[gap] + 
	( d->density != DISK_SD ? 3 : 0 ) >= d->bpt )  /* too many data bytes */
    return 1;
/*-------------------------------- given gap --------------------------------*/
  memset( d->track + d->i, g->gap,  g->len[gap] ); d->i += g->len[gap];
  memset( d->track + d->i, g->sync, g->sync_len ); d->i += g->sync_len;
  if( d->density != DISK_SD ) {
    memset( d->track + d->i , mark, 3 );
    bitmap_set( d->clocks, d->i ); d->i++;
    bitmap_set( d->clocks, d->i ); d->i++;
    bitmap_set( d->clocks, d->i ); d->i++;
  }
  return 0;
}

static int
preindex_add( disk_t *d, int gap )		/* preindex gap and index mark */
{
/*------------------------------ pre-index gap -------------------------------*/
  if( gap_add( d, 0, 0xc2, gap ) )
    return 1;
  if( d->i + 1 >= d->bpt )  /* too many data bytes */
    return 1;
  if( d->density == DISK_SD )
    bitmap_set( d->clocks, d->i ); 		/* set clock mark */
  d->track[ d->i++ ] = 0xfc;			/* index mark */
  return 0;
}

static int
postindex_add( disk_t *d, int gap )		/* preindex gap and index mark */
{
  return gap_add( d, 1, 0xa1, gap );
}

static int
gap4_add( disk_t *d, int gap )
{
  int len = d->bpt - d->i;
  disk_gap_t *g = &gaps[ gap ];

  if( len < 0 ) {
    return 1;
  }
/*------------------------------     GAP IV     ------------------------------*/
  memset( d->track + d->i, g->gap, len ); /* GAP IV fill until end of track */
  d->i = d->bpt;
  return 0;
}

#define SECLEN_128 0x00
#define SECLEN_256 0x01
#define SECLEN_512 0x02
#define SECLEN_1024 0x03
#define CRC_OK 0
#define CRC_ERROR 1

static int
id_add( disk_t *d, int h, int t, int s, int l, int gap, int crc_error )
{
  libspectrum_word crc = 0xffff;
  if( d->i + 7 >= d->bpt )		/* too many data bytes */
    return 1;
/*------------------------------     header     ------------------------------*/
  if( d->density == DISK_SD )
    bitmap_set( d->clocks, d->i ); 		/* set clock mark */
  d->track[ d->i++ ] = 0xfe;		/* ID mark */
  if( d->density != DISK_SD ) {
    crc = crc_fdc( crc, 0xa1 );
    crc = crc_fdc( crc, 0xa1 );
    crc = crc_fdc( crc, 0xa1 );
  }
  crc = crc_fdc( crc, 0xfe );
  d->track[ d->i++ ] = t; crc = crc_fdc( crc, t );
  d->track[ d->i++ ] = h; crc = crc_fdc( crc, h );
  d->track[ d->i++ ] = s; crc = crc_fdc( crc, s );
  d->track[ d->i++ ] = l; crc = crc_fdc( crc, l );
  d->track[ d->i++ ] = crc >> 8;
  if( crc_error ) {
    d->track[ d->i++ ] = (~crc) & 0xff;	/* record a CRC error */
  } else {
    d->track[ d->i++ ] = crc & 0xff;	/* CRC */
  }
/*------------------------------     GAP II     ------------------------------*/
  return gap_add( d, 2, 0xa1, gap );
}

static int
datamark_add( disk_t *d, int ddam )
{
  if( d->i + 1 >= d->bpt )  /* too many data bytes */
    return 1;
/*------------------------------      data      ------------------------------*/
  if( d->density == DISK_SD )
    bitmap_set( d->clocks, d->i ); 		/* set clock mark */
  d->track[ d->i++ ] = ddam ? 0xf8 : 0xfb;	/* DATA mark 0xf8 -> deleted data */
  return 0;
}
#define NO_DDAM 0
#define DDAM 1
#define NO_AUTOFILL -1
/* if 'file' == NULL, then copy data bytes from 'data' */  
static int
data_add( disk_t *d, FILE *file, unsigned char *data, int len, int ddam, int gap, int autofill )
{
  int length;
  libspectrum_word crc = 0xffff;

  if( datamark_add( d, ddam ) )
    return 1;
  if( d->density != DISK_SD ) {
    crc = crc_fdc( crc, 0xa1 );
    crc = crc_fdc( crc, 0xa1 );
    crc = crc_fdc( crc, 0xa1 );
  }
  crc = crc_fdc( crc, 0xfb );
  if( len < 0 )
    goto header_crc_error;		/* CRC error */
  if( d->i + len + 2 >= d->bpt )  /* too many data bytes */
    return 1;
/*------------------------------      data      ------------------------------*/
  if( file == NULL ) {
    memcpy( d->track + d->i, data, len );
    length = len;
  } else {
    length = fread( d->track + d->i, 1, len, file );
  }
  if( length != len ) {		/* autofill with 'autofill' */
    if( autofill < 0 )
      return 1;
    while( length < len ) {
      d->track[ d->i + length ] = autofill;
      length++;
    }
  }
  length = 0;
  while( length < len ) {	/* calculate CRC */
    crc = crc_fdc( crc, d->track[ d->i ] );
    d->i++;
    length++;
  }
  d->track[ d->i++ ] = crc >> 8; d->track[ d->i++ ] = crc & 0xff;    /* CRC */
/*------------------------------     GAP III    ------------------------------*/
header_crc_error:
  return ( gap_add( d, 3, 0xa1, gap ) );
}

static int
calc_sectorlen( disk_t *d, int sector_length, int gap )
{
  int len = 0;
  disk_gap_t *g = &gaps[ gap ];
  
/*------------------------------     ID        ------------------------------*/
  len += 7;
/*------------------------------     GAP II    ------------------------------*/
  len += g->len[2] + g->sync_len;
  if( d->density != DISK_SD )
    len += 3;
/*---------------------------------  data   ---------------------------------*/
  len += 1;		/* DAM */
  len += sector_length;
  len += 2;		/* CRC */
/*------------------------------    GAP III    ------------------------------*/
  len += g->len[3] + g->sync_len;
  if( d->density != DISK_SD )
    len += 3;
  return len;
}

#define SECTOR_BASE_0 0
#define SECTOR_BASE_1 1
#define NO_INTERLEAVE 1
#define INTERLEAVE_2 2
#define NO_PREINDEX 0
#define PREINDEX 1

static int
trackgen( disk_t *d, FILE *file, int head, int track, int sector_base,
	  int sectors, int sector_length, int preindex, int gap, int interleave,
	  int autofill )
{
  int i, s, pos;
  int slen = calc_sectorlen( d, sector_length, gap );
  int idx;
  
  d->i = 0;
  d->track = d->data + ( ( d->sides * track + head ) * d->tlen );
  d->clocks = d->track + d->bpt;
  if( preindex && preindex_add( d, gap ) )
    return 1;
  if( postindex_add( d, gap ) )
    return 1;

  idx = d->i;
  pos = i = 0;
  for( s = sector_base; s < sector_base + sectors; s++ ) {
    d->i = idx + ( pos + i ) * slen;
    if( id_add( d, head, track, s, sector_length >> 8, gap, CRC_OK ) )
      return 1;
    if( data_add( d, file, NULL, sector_length, NO_DDAM, gap, autofill ) )
      return 1;
    pos += interleave;
    if( pos >= sectors ) {
      pos = 0;
      i++;
    }
  }
  d->i = idx + sectors * slen;
  return gap4_add( d, gap );
}

/* close and destroy a disk structure and data */
void
disk_close( disk_t *d )
{
  if( d->filename != NULL ) {
    free( d->filename );
    d->filename = NULL;
  }
  if( d->data != NULL ) {
    free( d->data );
    d->data = NULL;
  }
  d->type = DISK_TYPE_NONE;
}

/*
 *  if d->density == DISK_DENS_AUTO => 
 *                            use d->tlen if d->bpt == 0
 *                             or use d->bpt to determine d->density
 *  or use d->density
 */
static int
disk_alloc( disk_t *d )
{
  size_t dlen;
  
  if( d->density != DISK_DENS_AUTO ) {
    d->bpt = disk_bpt[ d->density ];
  } else if( d->bpt > 12000 ) {
    return d->status = DISK_UNSUP;
  } else if( d->bpt > 6000 ) {
    d->bpt = disk_bpt[ DISK_HD ];
  } else if( d->bpt > 3000 ) {
    d->bpt = disk_bpt[ DISK_DD ];
  } else if( d->bpt > 0 ) {
    d->bpt = disk_bpt[ DISK_SD ];
  }

  d->tlen = d->bpt + d->bpt / 8 + ( d->bpt % 8 ? 1 : 0 );
  dlen = d->sides * d->cylinders * d->tlen;		/* track len with clock marks */

  if( ( d->data = calloc( 1, dlen ) ) == NULL )
    return d->status = DISK_MEM;

  return d->status = DISK_OK;
}

/* create a new unformatted disk  */
int
disk_new( disk_t *d, int sides, int cylinders,
	     disk_dens_t density, disk_type_t type )
{
  if( density < DISK_DENS_AUTO || density > DISK_HD ||	/* unknown density */
      type <= DISK_TYPE_NONE || type >= DISK_TYPE_LAST || /* unknown type */
      sides < 1 || sides > 2 ||				/* 1 or 2 side */
      cylinders < 35 || cylinders > 83 )		/* 35 .. 83 cylinder */
    return d->status = DISK_GEOM;

  d->type = type;
  d->density = density == DISK_DENS_AUTO ? DISK_DD : density;
  d->sides = sides;
  d->cylinders = cylinders;
  
  if( disk_alloc( d ) != DISK_OK )
    return d->status;

  d->filename = NULL;

  return d->status = DISK_OK;
}

static int
alloc_uncompress_buffer( unsigned char **buffer, int length )
{
  unsigned char *b;
  
  if( *buffer != NULL )				/* return if allocated */
    return 0;
  b = calloc( length, 1 );
  if( b == NULL )
    return 1;
  memset( b, 0, length );
  *buffer = b;
  return 0;
}

/* open a disk image */

static int
open_udi( FILE *file, disk_t *d )
{
  int i, bpt;

  d->sides = head[10] + 1;
  d->cylinders = head[9] + 1;
  d->density = DISK_DENS_AUTO;;
  fseek( file, 16, SEEK_SET );
  d->bpt = 0;

  /* scan file for the longest track */
  for( i = 0; i < d->sides * d->cylinders; i++ ) {
    if( fread( head, 3, 1, file ) != 1 )
      return d->status = DISK_OPEN;
    bpt = head[1] + 256 * head[2];		/* current track len... */
    if( head[0] != 0x00 )
      return d->status = DISK_UNSUP;
    if( bpt > d->bpt )
      d->bpt = bpt;
    if( fseek( file, bpt + bpt / 8 + ( bpt % 8 ? 1 : 0 ), SEEK_CUR ) == -1 )
      return d->status = DISK_OPEN;
  }

  if( disk_alloc( d ) != DISK_OK )
    return d->status;
  fseek( file, 16, SEEK_SET );
  d->track = d->data;

  for( i = 0; i < d->sides * d->cylinders; i++ ) {
    if( fread( head, 3, 1, file ) != 1 )
      return d->status = DISK_OPEN;
    bpt = head[1] + 256 * head[2];		/* current track len... */
    if( head[0] != 0x00 )
      return d->status = DISK_UNSUP;
					        /* read track + clocks */
    if( fread( d->track, bpt + bpt / 8 + ( bpt % 8 ? 1 : 0 ), 1, file ) != 1 )
      return d->status = DISK_OPEN;
    d->track += d->tlen;
  }

  return d->status = DISK_OK;
}

static int
open_mgt_img( FILE *file, disk_t *d, size_t file_length, disk_type_t type )
{
  int i, j, sectors, seclen;

  fseek( file, 0, SEEK_SET );

  /* guess geometry of disk:
   * 2*80*10*512, 1*80*10*512 or 1*40*10*512 */
  if( file_length == 2*80*10*512 ) {
    d->sides = 2; d->cylinders = 80; sectors = 10; seclen = 512;
  } else if( file_length == 1*80*10*512 ) {
    /* we cannot distinguish between a single sided 80 track image
     * and a double sided 40 track image (2*40*10*512) */
    d->sides = 1; d->cylinders = 80; sectors = 10; seclen = 512;
  } else if( file_length == 1*40*10*512 ) {
    d->sides = 1; d->cylinders = 40; sectors = 10; seclen = 512;
  } else {
    return d->status = DISK_GEOM;
  }

  /* create a DD disk */
  d->density = DISK_DD;
  if( disk_alloc( d ) != DISK_OK )
    return d->status;

  if( type == DISK_IMG ) {	/* IMG out-out */
    for( j = 0; j < d->sides; j++ ) {
      for( i = 0; i < d->cylinders; i++ ) {
	if( trackgen( d, file, j, i, SECTOR_BASE_1, sectors, seclen,
		      NO_PREINDEX, GAP_MGT_PLUSD, NO_INTERLEAVE, NO_AUTOFILL ) )
	  return d->status = DISK_GEOM;
      }
    }
  } else {			/* MGT alt */
    for( i = 0; i < d->sides * d->cylinders; i++ ) {
      if( trackgen( d, file, i % 2, i / 2, SECTOR_BASE_1, sectors, seclen,
		    NO_PREINDEX, GAP_MGT_PLUSD, NO_INTERLEAVE, NO_AUTOFILL ) )
	return d->status = DISK_GEOM;
    }
  }

  return d->status = DISK_OK;
}

static int
open_sad( FILE *file, disk_t *d, int preindex )
{
  int i, j, sectors, seclen;

  fseek( file, 22, SEEK_SET );
  d->sides = head[18];
  d->cylinders = head[19];
  sectors = head[20];
  seclen = head[21] * 64;

  /* create a DD disk */
  d->density = DISK_DD;
  if( disk_alloc( d ) != DISK_OK )
    return d->status;

  for( j = 0; j < d->sides; j++ ) {
    for( i = 0; i < d->cylinders; i++ ) {
      if( trackgen( d, file, j, i, SECTOR_BASE_1, sectors, seclen, preindex, 
		    GAP_MGT_PLUSD, NO_INTERLEAVE, NO_AUTOFILL ) )
	return d->status = DISK_GEOM;
    }
  }

  return d->status = DISK_OK;
}

static int
open_trd( FILE *file, disk_t *d )
{
  int i, j, sectors, seclen;

  fseek( file, 8*256, SEEK_SET );
  if( fread( head, 256, 1, file ) != 1 )
    return d->status = DISK_OPEN;

  if( head[231] != 0x10 || head[227] < 0x16 || head[227] > 0x19 )
    return d->status = DISK_OPEN;	/*?*/

  /* guess geometry of disk */
  d->sides =  head[227] & 0x08 ? 1 : 2;
  d->cylinders = head[227] & 0x01 ? 40 : 80;
  sectors = 16; seclen = 256;

  /* create a DD disk */
  d->density = DISK_DD;
  if( disk_alloc( d ) != DISK_OK )
    return d->status;

  fseek( file, 0, SEEK_SET );
  for( i = 0; i < d->cylinders; i++ ) {
    for( j = 0; j < d->sides; j++ ) {
      if( trackgen( d, file, j, i, SECTOR_BASE_1, sectors, seclen,
    	    NO_PREINDEX, GAP_TRDOS, INTERLEAVE_2, 0x00 ) )
	return d->status = DISK_GEOM;
    }
  }
  return d->status = DISK_OK;
}

static int
open_fdi( FILE *file, disk_t *d, int preindex )
{
  int i, j, h, bpt, gap;
  int data_offset, track_offset, head_offset, sector_offset;

  d->wrprot = head[0x03] == 1 ? 1 : 0;
  d->sides = head[0x06] + 256 * head[0x07];
  d->cylinders = head[0x04] + 256 * head[0x05];
  data_offset = head[0x0a] + 256 * head[0x0b];
  h = 0x0e + head[0x0c] + 256 * head[0x0d];		/* save head start */
  head_offset = h;

  /* first determine the longest track */
  d->bpt = 0;
  for( i = 0; i < d->cylinders * d->sides; i++ ) {	/* ALT */
    fseek( file, head_offset, SEEK_SET );
    if( fread( head, 7, 1, file ) != 1 )	/* 7 := track head  */
      return d->status = DISK_OPEN;
    bpt = 0;

    for( j = 0; j < head[0x06]; j++ ) {		/* calculate track len */
      if( j % 35 == 0 ) {				/* 35-sector header */
	if( fread( head + 7, 245, 1, file ) != 1 )	/* 7*35 := max 35 sector head */
	  return d->status = DISK_OPEN;
      }
      if( ( head[ 0x0b + 7 * ( j % 35 ) ] & 0x3f ) != 0 )
	bpt += 0x80 << head[ 0x0a + 7 * ( j % 35 ) ];
    }

    if( bpt > d->bpt )
      d->bpt = bpt;

    head_offset += 7 + 7 * head[ 0x06 ];
  }

  d->density = DISK_DENS_AUTO;		/* disk_alloc use d->bpt */
  if( disk_alloc( d ) != DISK_OK )
    return d->status;
    /* start reading the tracks */
  gap = d->bpt <= 3000 ? GAP_MINIMAL_FM : GAP_MINIMAL_MFM;
	
  head_offset = h;			/* restore head start */
  for( i = 0; i < d->cylinders * d->sides; i++ ) {	/* ALT */
    fseek( file, head_offset, SEEK_SET );
    if( fread( head, 7, 1, file ) != 1 ) /* 7 = track head */
      return d->status = DISK_OPEN;
    track_offset = head[0x00] + 256 * head[0x01] +
    			 65536 * head[0x02] + 16777216 * head[0x03];
    d->track = d->data + i * d->tlen; d->clocks = d->track + d->bpt;
    d->i = 0;
    if( preindex )
      preindex_add( d, gap );
    postindex_add( d, gap );
    bpt = 0;
    for( j = 0; j < head[0x06]; j++ ) {
      if( j % 35 == 0 ) {			/* if we have more than 35 sector in a track,
						   we have to seek back to the next sector
						   headers and read it ( max 35 sector header */
	fseek( file, head_offset + 7 *( j + 1 ), SEEK_SET );
	if( fread( head + 7, 245, 1, file ) != 1 )	/* 7*35 := max 35 sector head */
	  return d->status = DISK_OPEN;
      }
      id_add( d, head[ 0x08 + 7 * ( j % 35 ) ], head[ 0x07 + 7*( j % 35 ) ],
	      head[ 0x09 + 7*( j % 35 ) ], head[ 0x0a + 7*( j % 35 ) ], gap, 
	      ( head[ 0x0b + 7*( j % 35 ) ] & 0x3f ) ? CRC_OK : CRC_ERROR );
      sector_offset = head[ 0x0c + 7 * ( j % 35 ) ] +
		      256 * head[ 0x0d + 7 * ( j % 35 ) ];
      fseek( file, data_offset + track_offset + sector_offset, SEEK_SET );
      data_add( d, file, NULL, ( head[ 0x0b + 7 * ( j % 35 ) ] & 0x3f ) == 0 ? 
			       -1 : 0x80 << head[ 0x0a + 7 * ( j % 35 ) ],
		head[ 0x0b + 7 * ( j % 35 ) ] & 0x80 ? DDAM : NO_DDAM,
		gap, NO_AUTOFILL );
    }
    head_offset += 7 + 7 * head[0x06];
    gap4_add( d, gap );
  }
  return d->status = DISK_OK;
}

static int
open_cpc( FILE *file, disk_t *d, disk_type_t type, int preindex )
{
  int i, j, seclen, gap;

  d->sides = head[0x31];
  d->cylinders = head[0x30];
  if( type == DISK_CPC ) {
    d->bpt = head[0x32] + 256 * head[0x33] - 0x100;
  } else {
    d->bpt = 0;
    for( i = 0; i < d->sides * d->cylinders; i++ )
      if( head[ 0x34 + i ] > d->bpt )
	d->bpt = head[ 0x34 + i ];
    d->bpt <<= 8;
    d->bpt -= 0x100;
  }

  d->density = DISK_DENS_AUTO;			/* disk_alloc use d->bpt */
  if( disk_alloc( d ) != DISK_OK )
    return d->status;

  d->track = d->data; d->clocks = d->track + d->bpt;
  for( i = 0; i < d->sides*d->cylinders; i++ ) {
    if( fread( head, 256, 1, file ) != 1 ||
	    memcmp( head, "Track-Info\r\n", 12 ) ) /* read track header */
	return d->status = DISK_OPEN;
    gap = (unsigned char)head[0x16] == 0xff ? GAP_MINIMAL_FM :
    					    GAP_MINIMAL_MFM;
	
    d->track = d->data + i * d->tlen; d->clocks = d->track + d->bpt;
    d->i = 0;
    if( preindex)
      preindex_add( d, gap );
    postindex_add( d, gap );
    for( j = 0; j < head[0x15]; j++ ) {			/* each sector */
      seclen = d->type == DISK_ECPC ? head[ 0x1e + 8 * j ] +
				      256 * head[ 0x1f + 8 * j ]
				    : 0x80 << head[ 0x1b + 8 * j ];
      id_add( d, head[ 0x19 + 8 * j ], head[ 0x18 + 8 * j ],
		   head[ 0x1a + 8 * j ], seclen >> 8, gap, CRC_OK );
      data_add( d, file, NULL, seclen, NO_DDAM, gap, 0x00 );	/**FIXME deleted? */
    }
    gap4_add( d, gap );
  }
  return d->status = DISK_OK;
}

static int
open_scl( FILE *file, disk_t *d )
{
  int i, j, s, sectors, seclen;
  int scl_deleted, scl_files, scl_i;

  d->sides = 2;
  d->cylinders = 80;
  d->density = DISK_DD;
  if( disk_alloc( d ) != DISK_OK )
    return d->status;

/*
 TR-DOS:
- Root directory is 8 sectors long starting from track 0, sector 1
- Max root entries are 128
- Root entry dimension is 16 bytes (16*128 = 2048 => 8 sector
- Logical sector(start from 0) 8th (9th physical) holds disc info
- Logical sectors from 9 to 15 are unused
- Files are *NOT* fragmented, and start on track 1, sector 1...

*/

  if( ( scl_files = head[8] ) > 128 || scl_files < 1 ) 	/* number of files */
    return d->status = DISK_GEOM;	/* too many file */
    fseek( file, 9, SEEK_SET );		/* read SCL entries */

  d->track = d->data;			/* track 0 */
  d->clocks = d->track + d->bpt;
  d->i = 0;
  postindex_add( d, GAP_TRDOS );
  scl_i = d->i;			/* the position of first sector */
  s = 1;				/* start sector number */
  scl_deleted = 0;			/* deleted files */
  sectors = 0;			/* allocated sectors */
					/* we use 'head[]' to build
					   TR-DOS directory */
  j = 0;				/* index for head[] */
  memset( head, 0, 256 );
  seclen = calc_sectorlen( d, 256, GAP_TRDOS );	/* one sector raw length */
  for( i = 0; i < scl_files; i++ ) {	/* read all entry and build TR-DOS dir */
    if( fread( head + j, 14, 1, file ) != 1 )
      return d->status = DISK_OPEN;
    head[ j + 14 ] = sectors % 16; /* ( sectors + 16 ) % 16 := sectors % 16
    							 starting sector */
    head[ j + 15 ] = sectors / 16 + 1; /* ( sectors + 16 ) / 16 := sectors / 16 + 1
    							 starting track */
    sectors += head[ j + 13 ];
    if( d->data[j] == 0x01 )		/* deleted file */
      scl_deleted++;
    if( sectors > 16 * 159 ) 	/* too many sectors needed */
      return d->status = DISK_MEM;	/* or DISK_GEOM??? */

    j += 16;
    if( j == 256 ) {			/* one sector ready */
      d->i = scl_i + ( ( s - 1 ) % 8 * 2 + ( s - 1 ) / 8 ) * seclen;	/* 1 9 2 10 3 ... */
      id_add( d, 0, 0, s, SECLEN_256, GAP_TRDOS, CRC_OK );
      data_add( d, NULL, head, 256, NO_DDAM, GAP_TRDOS, NO_AUTOFILL );
      memset( head, 0, 256 );
      s++;
      j = 0;
    }
  }

  if( j != 0 ) {	/* we have to add this sector  */
    d->i = scl_i + ( ( s - 1 ) % 8 * 2 + ( s - 1 ) / 8 ) * seclen;	/* 1 9 2 10 3 ... */
    id_add( d, 0, 0, s, SECLEN_256, GAP_TRDOS, CRC_OK );
    data_add( d, NULL, head, 256, NO_DDAM, GAP_TRDOS, NO_AUTOFILL );
    s++;
  }
			/* and add empty sectors up to No. 16 */
  memset( head, 0, 256 );
  for( ; s <= 16; s++ ) {			/* finish the first track */
    d->i = scl_i + ( ( s - 1 ) % 8 * 2 + ( s - 1 ) / 8 ) * seclen;	/* 1 9 2 10 3 ... */
    id_add( d, 0, 0, s, SECLEN_256, GAP_TRDOS, CRC_OK );
    if( s == 9 ) {			/* TR-DOS descriptor */
      head[225] = sectors % 16;	/* first free sector */
      head[226] = sectors / 16 + 1;	/* first free track */
      head[227] = 0x16;		/* 80 track 2 side disk */
      head[228] = scl_files;		/* number of files */
      head[229] = ( 2544 - sectors ) % 256;	/* free sectors */
      head[230] = ( 2544 - sectors ) / 256;
      head[231] = 0x10;		/* TR-DOS ID byte */
      memset( head + 234, 32, 9 );
      head[244] = scl_deleted;	/* number of deleted files */
      memcpy( head + 245, "FUSE-SCL", 8 );
    }
    data_add( d, NULL, head, 256, NO_DDAM, GAP_TRDOS, NO_AUTOFILL );
    if( s == 9 )
      memset( head, 0, 256 );		/* clear sector data... */
  }
  gap4_add( d, GAP_TRDOS );
    
  /* now we continue with the data */
  for( i = 1; i < d->sides * d->cylinders; i++ ) {
    if( trackgen( d, file, i % 2, i / 2, SECTOR_BASE_1, 16, 256,
    		    NO_PREINDEX, GAP_TRDOS, INTERLEAVE_2, 0x00 ) )
      return d->status = DISK_OPEN;
  }
  return d->status = DISK_OK;
}

static int
open_td0( FILE *file, disk_t *d, int preindex )
{
  int i, j, s, sectors, seclen, bpt, gap, mfm;
  int data_offset, track_offset, sector_offset;
  unsigned char *buff;

  if( head[0] == 't' )		/* signature "td" -> advanced compression  */
    return d->status = DISK_IMPL;	/* Advenced compression not implemented  */
  buff = NULL;			/* we may use this buffer */
  mfm = head[5] & 0x80 ? 0 : 1;	/* td0notes say: may older teledisk
					   indicate the SD on high bit of
					   data rate */
  d->sides = head[9];			/* 1 or 2 */
  if( d->sides < 1 || d->sides > 2 )
    return d->status = DISK_GEOM;
					/* skip comment block if any */
  data_offset = track_offset = 12 + ( head[7] & 0x80 ? 
					10 + head[14] + 256 * head[15] : 0 );

  /* determine the greatest track length */
  d->bpt = 0;
  d->cylinders = 0;
  while( 1 ) {
    fseek( file, track_offset, SEEK_SET );
    if( fread( head, 1, 1, file ) != 1 )
      return d->status = DISK_OPEN;
    if( ( sectors = head[0] ) == 255 ) /* sector number 255 => end of tracks */
      break;
    if( fread( head + 1, 3, 1, file ) != 1 ) /* finish to read track header */
      return d->status = DISK_OPEN;
    if( head[1] + 1 > d->cylinders )	/* find the biggest cylinder number */
      d->cylinders = head[1] + 1;
    bpt = 0;
    sector_offset = track_offset + 4;
    for( s = 0; s < sectors; s++ ) {
      fseek( file, sector_offset, SEEK_SET );
      if( fread( head, 6, 1, file ) != 1 )
	return d->status = DISK_OPEN;
      if( !( head[4] & 0x30 ) ) {		/* only if we have data */
	if( fread( head + 6, 3, 1, file ) != 1 )	/* read data header */
	  return d->status = DISK_OPEN;

	bpt += 0x80 << head[3];
	if( head[3] > seclen )
	  seclen = head[3];			/* biggest sector */
	sector_offset += head[6] + 256 * head[7] - 1;
      }
      sector_offset += 9;
    }
    if( bpt > d->bpt )
      d->bpt = bpt;
    track_offset = sector_offset;
  }

  d->density = DISK_DENS_AUTO;
  if( disk_alloc( d ) != DISK_OK )
    return d->status;

  d->track = d->data; d->clocks = d->track + d->bpt;

  fseek( file, data_offset, SEEK_SET );	/* first track header */
  while( 1 ) {
    if( fread( head, 1, 1, file ) != 1 )
      return d->status = DISK_OPEN;
    if( ( sectors = head[0] ) == 255 ) /* sector number 255 => end of tracks */
      break;
    if( fread( head + 1, 3, 1, file ) != 1 ) { /* finish to read track header */
      if( buff )
	free( buff );
      fclose( file );
      return d->status = DISK_OPEN;
    }

    d->track = d->data + ( d->sides * head[1] + ( head[2] & 0x01 ) ) * d->tlen;
    d->clocks = d->track + d->bpt;
    d->i = 0;
    			/* later teledisk -> if head[2] & 0x80 -> FM track */
    gap = mfm == 0 || head[2] & 0x80 ? GAP_MINIMAL_FM : GAP_MINIMAL_MFM;
    postindex_add( d, gap );

    for( s = 0; s < sectors; s++ ) {
      if( fread( head, 6, 1, file ) != 1 ) {  /* sector head */
	if( buff )
	  free( buff );
	fclose( file );
	return d->status = DISK_OPEN;
      }
      if( !( head[4] & 0x40 ) )		/* if we have id we add */
	id_add( d, head[1], head[0], head[2], head[3], gap,
				   head[4] & 0x02 ? CRC_ERROR : CRC_OK );
      if( !( head[4] & 0x30 ) ) {		/* only if we have data */
	if( fread( head + 6, 3, 1, file ) != 1 ) {	/* read data header */
	  if( buff )
	    free( buff );
	  return d->status = DISK_OPEN;
	}
      }
      if( head[4] & 0x40 ) {		/* if we have _no_ id we drop data... */
	fseek( file, head[6] + 256 * head[7] - 1, SEEK_CUR );
	continue;		/* next sector */
      }
      seclen = 0x80 << head[3];

      switch( head[8] ) {
      case 0:				/* raw sector data */
	if( head[6] + 256 * head[7] - 1 != seclen ) {
	  if( buff )
	    free( buff );
	  return d->status = DISK_OPEN;
	}
	if( data_add( d, file, NULL, head[6] + 256 * head[7] - 1,
		      head[4] & 0x04 ? DDAM : NO_DDAM, gap, NO_AUTOFILL ) ) {
	  if( buff )
	    free( buff );
	  return d->status = DISK_OPEN;
	}
	break;
      case 1:				/* Repeated 2-byte pattern */
	if( buff == NULL && alloc_uncompress_buffer( &buff, 8192 ) )
	  return d->status = DISK_MEM;
	for( i = 0; i < seclen; ) {			/* fill buffer */
	  if( fread( head + 9, 4, 1, file ) != 1 ) { /* read block header */
	    free( buff );
	    return d->status = DISK_OPEN;
	  }
	  if( i + 2 * ( head[9] + 256*head[10] ) > seclen ) {
						  /* too many data bytes */
	    free( buff );
	    return d->status = DISK_OPEN;
	  }
	  /* ab ab ab ab ab ab ab ab ab ab ab ... */
	  for( j = 1; j < head[9] + 256 * head[10]; j++ )
	    memcpy( buff + i + j * 2, &head[11], 2 );
	  i += 2 * ( head[9] + 256 * head[10] );
	}
	if( data_add( d, NULL, buff, head[6] + 256 * head[7] - 1,
		      head[4] & 0x04 ? DDAM : NO_DDAM, gap, NO_AUTOFILL ) ) {
	  free( buff );
	  return d->status = DISK_OPEN;
	}
	break;
      case 2:				/* Run Length Encoded data */
	if( buff == NULL && alloc_uncompress_buffer( &buff, 8192 ) )
	  return d->status = DISK_MEM;
	for( i = 0; i < seclen; ) {			/* fill buffer */
	  if( fread( head + 9, 2, 1, file ) != 1 ) {/* read block header */
	    free( buff );
	    return d->status = DISK_OPEN;
	  }
	  if( head[9] == 0 ) {		/* raw bytes */
	    if( i + head[10] > seclen ||	/* too many data bytes */
		      fread( buff + i, head[10], 1, file ) != 1 ) {
	      free( buff );
	      return d->status = DISK_OPEN;
	    }
	    i += head[10];
	  } else {				/* repeated samples */
	    if( i + 2 * head[9] * head[10] > seclen || /* too many data bytes */
		      fread( buff + i, 2 * head[9], 1, file ) != 1 ) {
	      free( buff );
	      return d->status = DISK_OPEN;
	    }
	      /*
		 abcdefgh abcdefg abcdefg abcdefg ...
		 \--v---/ 
		  2*head[9]
		 |        |       |       |           |
		 +- 0     +- 1    +- 2    +- 3    ... +- head[10]-1
	      */
	    for( j = 1; j < head[10]; j++ ) /* repeat 'n' times */
	      memcpy( buff + i + j * 2 * head[9], buff + i, 2 * head[9] );
	    i += 2 * head[9] * head[10];
	  }
	}
	if( data_add( d, NULL, buff, head[6] + 256 * head[7] - 1,
	      head[4] & 0x04 ? DDAM : NO_DDAM, gap, NO_AUTOFILL ) ) {
	  free( buff );
	  return d->status = DISK_OPEN;
	}
	break;
      default:
	if( buff )
	  free( buff );
	return d->status = DISK_OPEN;
	break;
      }
    }
  }

  gap4_add( d, gap );
  if( buff )
    free( buff );
  return d->status = DISK_OK;
}

/* open a disk image file, read and convert to our format
 * if preindex != 0 we generate preindex gap if needed
 */
int
disk_open( disk_t *d, const char *filename, int preindex )
{
  FILE *file;
  disk_type_t type;
  struct stat file_info;
  const char *ext;
  size_t namelen;

  if( filename == NULL || *filename == '\0' )
    return d->status = DISK_OPEN;

  if( ( file = fopen( filename, "rb" ) ) == NULL )
    return d->status = DISK_OPEN;

  if( stat( filename, &file_info) ) {
    fclose( file );
    return d->status = DISK_OPEN;
  }
  
  if( fread( head, 256, 1, file ) != 1 ) {
    fclose( file );
    return d->status = DISK_OPEN;
  }

  namelen = strlen( filename );
  if( namelen < 4 )
    ext = NULL;
  else
    ext = filename + namelen - 4;

  type = DISK_TYPE_NONE;

  if( memcmp( head, "UDI!", 4 ) == 0 )
    type = DISK_UDI;
  else if( memcmp( head, "EXTENDED CPC DSK File\r\nDisk-Info\r\n", 34 ) == 0 )
    type = DISK_ECPC;
  else if( memcmp( head, "MV - CPCEMU Disk-File\r\nDisk-Info\r\n", 34 ) == 0 )
    type = DISK_CPC;
  else if( memcmp( head, "FDI", 3 ) == 0 )
    type = DISK_FDI;
  else if( memcmp( head, "Aley's disk backup", 18 ) == 0 )
    type = DISK_SAD;
  else if( memcmp( head, "SINCLAIR", 8 ) == 0 )
    type = DISK_SCL;
  else if( memcmp( head, "TD", 2 ) == 0 || memcmp( head, "td", 2 ) == 0 )
    type = DISK_TD0;
  else {
    if( ext == NULL ) {
      fclose( file );
      return d->status = DISK_OPEN;
    }
    if( !strcasecmp( ext, ".mgt" ) || !strcasecmp( ext, ".dsk" ) )
      type = DISK_MGT;					/* ALT side */
    else if( !strcasecmp( ext, ".img" ) )		/* out-out */
      type = DISK_IMG;
    else if( !strcasecmp( ext, ".trd" ) )		/* out-out */
      type = DISK_TRD;
  }

  if( access( filename, W_OK ) == -1 )		/* file read only */
    d->wrprot = 1;
  
  switch ( type ) {
  case DISK_UDI:
    open_udi( file, d );
    break;
  case DISK_MGT:
  case DISK_IMG:
    open_mgt_img( file, d, file_info.st_size, type );
    break;
  case DISK_SAD:
    open_sad( file, d, preindex );
    break;
  case DISK_TRD:
    open_trd( file, d );
    break;
  case DISK_FDI:
    open_fdi( file, d, preindex );
    break;
  case DISK_CPC:
  case DISK_ECPC:
    open_cpc( file, d, type, preindex );
    break;
  case DISK_SCL:
    open_scl( file, d );
    break;
  case DISK_TD0:
    open_td0( file, d, preindex );
    break;
  default:
    fclose( file );
    return d->status = DISK_OPEN;
  }
  if( d->status != DISK_OK ) {
    if( d->data != NULL )
      free( d->data );
    fclose( file );
    return d->status;
  }
  if( ( d->filename = malloc( strlen( filename ) + 1 ) ) == NULL ) {
    free( d->data );
    fclose( file );
    return d->status = DISK_MEM;
  }
  fclose( file );
  strcpy( d->filename, filename );
  d->type = type;
  return d->status = DISK_OK;
}

/*--------------------- start of write section ----------------*/

static int
write_udi( FILE * file, disk_t * d )
{
  int i, j;
  size_t len;
  libspectrum_dword crc;

  crc = ~( libspectrum_dword ) 0;
  len = 16 + ( d->tlen + 3 ) * d->sides * d->cylinders;
  head[0] = 'U';
  head[1] = 'D';
  head[2] = 'I';
  head[3] = '!';
  head[4] = len & 0xff;
  head[5] = ( len >> 8 ) & 0xff;
  head[6] = ( len >> 16 ) & 0xff;
  head[7] = ( len >> 24 ) & 0xff;
  head[8] = 0x00;
  head[9] = d->cylinders - 1;
  head[10] = d->sides - 1;
  head[11] = head[12] = head[13] = head[14] = head[15] = 0;
  if( fwrite( head, 16, 1, file ) != 1 )
    return d->status = DISK_WRPART;
  for( j = 0; j < 16; j++ )
    crc = crc_udi( crc, head[j] );
  d->track = d->data;
  d->clocks = d->track + d->bpt;
  for( i = 0; i < d->sides * d->cylinders; i++ ) {	/* write tracks */
    head[0] = 0;
    head[1] = d->bpt & 0xff;
    head[2] = d->bpt >> 8;
    if( fwrite( head, 3, 1, file ) != 1 )
      return d->status = DISK_WRPART;
    for( j = 0; j < 3; j++ )
      crc = crc_udi( crc, head[j] );
    if( fwrite( d->track, d->tlen, 1, file ) != 1 )
      return d->status = DISK_WRPART;
    for( j = d->tlen; j > 0; j-- ) {
      crc = crc_udi( crc, *d->track );
      d->track++;
    }
  }
  head[0] = crc & 0xff;
  head[1] = ( crc >> 8 ) & 0xff;
  head[2] = ( crc >> 16 ) & 0xff;
  head[3] = ( crc >> 24 ) & 0xff;
  if( fwrite( head, 4, 1, file ) != 1 )		/* CRC */
    fclose( file );
  return d->status = DISK_OK;
}

static int
write_img_mgt( FILE * file, disk_t * d, disk_type_t type )
{
  int i, j, sbase, sectors, seclen, mfm;

  if( check_disk_geom( d, &sbase, &sectors, &seclen, &mfm ) ||
      sbase != 1 || seclen != 2 || sectors != 10 )
    return d->status = DISK_GEOM;

  if( type == DISK_IMG ) {	/* out-out */
    for( j = 0; j < d->sides; j++ ) {
      for( i = 0; i < d->cylinders; i++ ) {
	if( savetrack( d, file, j, i, 1, sectors, seclen ) )
	  return d->status = DISK_GEOM;
      }
    }
  } else {			/* alt */
    for( i = 0; i < d->cylinders; i++ ) {	/* MGT */
      for( j = 0; j < d->sides; j++ ) {
	if( savetrack( d, file, j, i, 1, sectors, seclen ) )
	  return d->status = DISK_GEOM;
      }
    }
  }
  return d->status = DISK_OK;
}

static int
write_trd( FILE * file, disk_t * d )
{
  int i, j, sbase, sectors, seclen, mfm;

  if( check_disk_geom( d, &sbase, &sectors, &seclen, &mfm ) ||
      sbase != 1 || seclen != 1 || sectors != 16 )
    return d->status = DISK_GEOM;
  for( i = 0; i < d->cylinders; i++ ) {
    for( j = 0; j < d->sides; j++ ) {
      if( savetrack( d, file, j, i, 1, sectors, seclen ) )
	return d->status = DISK_GEOM;
    }
  }
  return d->status = DISK_OK;
}

static int
write_sad( FILE * file, disk_t * d )
{
  int i, j, sbase, sectors, seclen, mfm;

  if( check_disk_geom( d, &sbase, &sectors, &seclen, &mfm ) || sbase != 1 )
    return d->status = DISK_GEOM;

  memcpy( head, "Aley's disk backup", 18 );
  head[18] = d->sides;
  head[19] = d->cylinders;
  head[20] = sectors;
  head[21] = seclen * 4;
  if( fwrite( head, 22, 1, file ) != 1 )	/* SAD head */
    return d->status = DISK_WRPART;

  for( j = 0; j < d->sides; j++ ) {	/* OUT-OUT */
    for( i = 0; i < d->cylinders; i++ ) {
      if( savetrack( d, file, j, i, 1, sectors, seclen ) )
	return d->status = DISK_GEOM;
    }
  }
  return d->status = DISK_OK;
}

static int
write_fdi( FILE * file, disk_t * d )
{
  int i, j, k, sbase, sectors, seclen, mfm, del;
  int h, t, s, b;
  int toff, soff;

  memset( head, 0, 14 );
  memcpy( head, "FDI", 3 );
  head[0x03] = d->wrprot = 1;
  head[0x04] = d->cylinders & 0xff;
  head[0x05] = d->cylinders >> 8;
  head[0x06] = d->sides & 0xff;
  head[0x07] = d->sides >> 8;
  sectors = 0;
  for( j = 0; j < d->cylinders; j++ ) {	/* count sectors */
    for( i = 0; i < d->sides; i++ ) {
      guess_track_geom( d, i, j, &sbase, &s, &seclen, &mfm );
      sectors += s;
    }
  }
  h = ( sectors + d->cylinders * d->sides ) * 7;	/* track header len */
  head[0x08] = ( h + 0x0e ) & 0xff;	/* description offset */
  head[0x09] = ( h + 0x0e ) >> 8; /* "http://fuse-emulator.sourceforge.net" */
  head[0x0a] = ( h + 0x33 ) & 0xff;	/* data offset */
  head[0x0b] = ( h + 0x33 ) >> 8;
  if( fwrite( head, 14, 1, file ) != 1 )	/* FDI head */
    return d->status = DISK_WRPART;

  /* write track headers */
  toff = 0;			/* offset of track data */
  for( i = 0; i < d->cylinders; i++ ) {
    for( j = 0; j < d->sides; j++ ) {
      d->track = d->data + ( ( d->sides * i + j ) * d->tlen );
      d->clocks = d->track + d->bpt;
      d->i = 0;
      head[0x00] = toff & 0xff;
      head[0x01] = ( toff >> 8 ) & 0xff;	/* track offset */
      head[0x02] = ( toff >> 16 ) & 0xff;
      head[0x03] = ( toff >> 24 ) & 0xff;
      head[0x04] = 0;
      head[0x05] = 0;
      guess_track_geom( d, j, i, &sbase, &sectors, &seclen, &mfm );
      head[0x06] = sectors;
      if( fwrite( head, 7, 1, file ) != 1 )
	return d->status = DISK_WRPART;

      d->track = d->data + ( ( d->sides * i + j ) * d->tlen );
      d->clocks = d->track + d->bpt;
      d->i = 0;
      k = 0;
      soff = 0;
      while( sectors > 0 ) {
	while( k < 35 && id_read( d, &h, &t, &s, &b ) ) {
	  head[ 0x00 + k * 7 ] = t;
	  head[ 0x01 + k * 7 ] = h;
	  head[ 0x02 + k * 7 ] = s;
	  head[ 0x03 + k * 7 ] = b;
	  head[ 0x05 + k * 7 ] = soff & 0xff;
	  head[ 0x06 + k * 7 ] = ( soff >> 8 ) & 0xff;
	  if( !datamark_read( d, &del ) ) {
	    head[ 0x04 + k * 7 ] = 0;	/* corrupt sector data */
	  } else {
	    head[ 0x04 + k * 7 ] = 1 << b | ( del ? 0x80 : 0x00 );
	    soff += 0x80 << b;
	  }
	  k++;
	}			/* Track header */
	if( fwrite( head, 7 * k, 1, file ) != 1 )
	  return d->status = DISK_WRPART;

	sectors -= k;
	k = 0;
      }
      toff += soff;
    }
  }
  if( fwrite( "http://fuse-emulator.sourceforge.net", 37, 1, file ) != 1 )
    return d->status = DISK_WRPART;

  /* write data */
  for( i = 0; i < d->cylinders; i++ ) {
    for( j = 0; j < d->sides; j++ ) {
      if( saverawtrack( d, file, j, i ) )
	return d->status = DISK_WRPART;
    }
  }
  return d->status = DISK_OK;
}

static int
write_cpc( FILE * file, disk_t * d )
{
  int i, j, k, sbase, sectors, seclen, mfm;
  int h, t, s, b;
  size_t len;

  i = check_disk_geom( d, &sbase, &sectors, &seclen, &mfm );
  if( i & DISK_SECLEN_VARI || i & DISK_SPT_VARI )
    return d->status = DISK_GEOM;

  if( i & DISK_MFM_VARI )
    mfm = -1;
  memset( head, 0, 256 );
  memcpy( head, "MV - CPCEMU Disk-File\r\nDisk-Info\r\n", 34 );
  head[0x30] = d->cylinders;
  head[0x31] = d->sides;
  len = sectors * ( 0x80 << seclen ) + 256;
  head[0x32] = len & 0xff;
  head[0x33] = len >> 8;
  if( fwrite( head, 256, 1, file ) != 1 )	/* CPC head */
    return d->status = DISK_WRPART;

  memset( head, 0, 256 );
  memcpy( head, "Track-Info\r\n", 12 );
  for( i = 0; i < d->cylinders; i++ ) {
    for( j = 0; j < d->sides; j++ ) {
      d->track = d->data + ( ( d->sides * i + j ) * d->tlen );
      d->clocks = d->track + d->bpt;
      d->i = 0;
      head[0x10] = i;
      head[0x11] = j;
      head[0x14] = seclen;
      head[0x15] = sectors;
      if( mfm != -1 )
	head[0x16] = mfm ? 0x4e : 0xff;
      head[0x17] = 0xe5;
      k = 0;
      while( id_read( d, &h, &t, &s, &b ) ) {
	head[ 0x18 + k * 8 ] = t;
	head[ 0x19 + k * 8 ] = h;
	head[ 0x1a + k * 8 ] = s;
	head[ 0x1b + k * 8 ] = b;
	if( k == 0 && mfm == -1 ) {	/* if mixed MFM/FM tracks */
	  head[0x16] = d->track[ d->i ] == 0x4e ? 0x4e : 0xff;
	}
	k++;
      }
      if( fwrite( head, 256, 1, file ) != 1 )	/* Track head */
	return d->status = DISK_WRPART;

      if( saverawtrack( d, file, j, i ) )
	return d->status = DISK_WRPART;
    }
  }
  return d->status = DISK_OK;
}

static int
write_scl( FILE * file, disk_t * d )
{
  int i, j, k, l, t, s, sbase, sectors, seclen, mfm, del;
  int entries;
  libspectrum_dword sum = 597;		/* sum of "SINCLAIR"*/

  if( check_disk_geom( d, &sbase, &sectors, &seclen, &mfm ) ||
      sbase != 1 || seclen != 1 || sectors != 16 )
    return d->status = DISK_GEOM;

  d->track = d->data;
  d->clocks = d->track + d->bpt;

  /* TR-DOS descriptor */
  if( !id_seek( d, 9 ) || !datamark_read( d, &del ) )
    return d->status = DISK_GEOM;

  entries = head[8] = d->track[ d->i + 228 ];	/* number of files */

  if( entries > 128 || d->track[ d->i + 231 ] != 0x10 ||
      ( d->track[ d->i + 227 ] != 0x16 && d->track[ d->i + 227 ] != 0x17 &&
	d->track[ d->i + 227 ] != 0x18 && d->track[ d->i + 227 ] != 0x19 ) ||
      d->track[ d->i ] != 0 )
    return d->status = DISK_GEOM;

  memcpy( head, "SINCLAIR", 8 );
  sum += entries;
  if( fwrite( head, 9, 1, file ) != 1 )
    return d->status = DISK_WRPART;

  /* save SCL entries */
  j = 1;			/* sector number */
  k = 0;
  sectors = 0;
  for( i = 0; i < entries; i++ ) {	/* read TR-DOS dir */
    if( j > 8 )		/* TR-DOS dir max 8 sector len */
      return d->status = DISK_GEOM;

    if( k == 0 && ( !id_seek( d, j ) || !datamark_read( d, &del ) ) )
      return d->status = DISK_GEOM;

    if( fwrite( d->track + d->i + k, 14, 1, file ) != 1 )
      return d->status = DISK_WRPART;
    sectors += d->track[ d->i + k + 13 ];	/* file length in sectors */
    for( s = 0; s < 14; s++ ) {
      sum += d->track[ d->i + k ];
      k++;
    }
    k += 2;
    if( k >= 256 ) {		/* end of sector */
      j++;
      k = 0;
    }
  }
  /* save data */
  /* we have to 'defragment' the disk :) */
  j = 1;			/* sector number */
  k = 0;			/* byte offset */
  for( i = 0; i < entries; i++ ) {	/* read TR-DOS dir */
    d->track = d->data;
    d->clocks = d->track + d->bpt;	/* set track 0 */
    if( k == 0 ) {
      if ( !id_seek( d, j ) || !datamark_read( d, &del ) )
	return d->status = DISK_GEOM;
      memcpy( head, d->track + d->i, 256 );
    }

    s = head[ k + 14 ];	/* starting sector */
    t = head[ k + 15 ];	/* starting track */
    sectors = head[ k + 13 ] + s;	/* last sector */
    k += 16;
    if( k == 256 ) {
      k = 0;
      j++;
    }
    if( t >= d->sides * d->cylinders )
      return d->status = DISK_GEOM;

    if( s % 16 == 0 )
      t--;
    d->track = d->data + t * d->tlen;
    d->clocks = d->track + d->bpt;

    for( ; s < sectors; s++ ) {		/* save all sectors */
      if( s % 16 == 0 ) {
	t++;
	if( t >= d->sides * d->cylinders )
	  return d->status = DISK_GEOM;
	d->track = d->data + t * d->tlen;
	d->clocks = d->track + d->bpt;
      }
      if( id_seek( d, s % 16 + 1 ) ) {
	if( datamark_read( d, &del ) ) {	/* write data if we have data */
	  if( data_write_file( d, file, 1 ) ) {
	    return d->status = DISK_GEOM;
	  } else {
	    for( l = 0; l < 256; l++ )
	      sum += d->track[ d->i + l ];
	  }
	}
      } else {
	return DISK_GEOM;
      }
    }
  }
  head[0] = sum & 0xff;
  head[1] = ( sum >> 8 ) & 0xff;
  head[2] = ( sum >> 16 ) & 0xff;
  head[3] = ( sum >> 24 ) & 0xff;
  
  if( fwrite( head, 4, 1, file ) != 1 )
    return d->status = DISK_WRPART;
  
  return d->status = DISK_OK;
}

static int
write_log( FILE * file, disk_t * d )
{
  int i, j, k, del;
  int h, t, s, b;

  fprintf( file, "DISK tracks log!\n" );
  fprintf( file, "Sides: %d, cylinders: %d\n", d->sides, d->cylinders );
  for( j = 0; j < d->cylinders; j++ ) {	/* ALT :) */
    for( i = 0; i < d->sides; i++ ) {
      d->track = d->data + ( ( d->sides * j + i ) * d->tlen );
      d->clocks = d->track + d->bpt;
      d->i = 0;
      fprintf( file, "*********\nSide: %d, cylinder: %d\n", i, j );
      while( id_read( d, &h, &t, &s, &b ) ) {
	fprintf( file, "  h:%d t:%d s:%d l:%d\n", h, t, s, 0x80 << b );
      }
    }
  }
  fprintf( file, "\n***************************\nSector Data Dump:\n" );
  for( j = 0; j < d->cylinders; j++ ) {	/* ALT :) */
    for( i = 0; i < d->sides; i++ ) {
      d->track = d->data + ( ( d->sides * j + i ) * d->tlen );
      d->clocks = d->track + d->bpt;
      d->i = 0;
      fprintf( file, "*********\nSide: %d, cylinder: %d\n", i, j );
      k = 0;
      while( id_read( d, &h, &t, &s, &b ) ) {
	b = 0x80 << b;
	if( datamark_read( d, &del ) )
	  fprintf( file, "  h:%d t:%d s:%d l:%d (%s)\n", h, t, s, b,
		   del ? "deleted" : "normal" );
	else
	  fprintf( file, "  h:%d t:%d s:%d l:%d (missing data)\n", h,
		   t, s, b );
	k = 0;
	while( k < b ) {
	  if( !( k % 16 ) )
	    fprintf( file, "0x%08x:", d->i );
	  fprintf( file, " 0x%02x", d->track[ d->i ] );
	  k++;
	  if( !( k % 16 ) )
	    fprintf( file, "\n" );
	  d->i++;
	}
      }
    }
  }
  fprintf( file, "\n***************************\n**Full Dump:\n" );
  for( j = 0; j < d->cylinders; j++ ) {	/* ALT :) */
    for( i = 0; i < d->sides; i++ ) {
      d->track = d->data + ( ( d->sides * j + i ) * d->tlen );
      d->clocks = d->track + d->bpt;
      d->i = 0;
      fprintf( file, "*********\nSide: %d, cylinder: %d\n", i, j );
      k = 0;
      while( d->i < d->bpt ) {
	if( !( k % 8 ) )
	  fprintf( file, "0x%08x:", d->i );
	fprintf( file, " 0x%04x", d->track[ d->i ] |
		 ( bitmap_test( d->clocks, d->i ) ? 0xff00 : 0x0000 ) );
	k++;
	if( !( k % 8 ) )
	  fprintf( file, "\n" );
	d->i++;
      }
    }
  }
  return d->status = DISK_OK;
}

int
disk_write( disk_t *d, const char *filename )
{
  FILE *file;
  const char *ext;
  size_t namelen;

  if( ( file = fopen( filename, "wb" ) ) == NULL )
    return d->status = DISK_WRFILE;
  
  namelen = strlen( filename );
  if( namelen < 4 )
    ext = NULL;
  else
    ext = filename + namelen - 4;

  if( d->type == DISK_TYPE_NONE ) {
    if( !strcasecmp( ext, ".udi" ) )
      d->type = DISK_UDI;				/* ALT side */
    else if( !strcasecmp( ext, ".dsk" ) )
      d->type = DISK_CPC;				/* ALT side */
    else if( !strcasecmp( ext, ".mgt" ) )
      d->type = DISK_MGT;				/* ALT side */
    else if( !strcasecmp( ext, ".img" ) )		/* out-out */
      d->type = DISK_IMG;
    else if( !strcasecmp( ext, ".trd" ) )		/* ALT */
      d->type = DISK_TRD;
    else if( !strcasecmp( ext, ".sad" ) )		/* ALT */
      d->type = DISK_SAD;
    else if( !strcasecmp( ext, ".fdi" ) )		/* ALT */
      d->type = DISK_FDI;
    else if( !strcasecmp( ext, ".scl" ) )		/* not really a disk image */
      d->type = DISK_SCL;
    else if( !strcasecmp( ext, ".log" ) )		/* ALT */
      d->type = DISK_LOG;
    else
      d->type = DISK_UDI;				/* ALT side */
  }

  switch( d->type ) {
  case DISK_UDI:
    write_udi( file, d );
    break;
  case DISK_IMG:
  case DISK_MGT:
    write_img_mgt( file, d, d->type );
    break;
  case DISK_TRD:
    write_trd( file,  d );
    break;
  case DISK_SAD:
    write_sad( file,  d );
    break;
  case DISK_FDI:
    write_fdi( file,  d );
    break;
  case DISK_SCL:
    write_scl( file,  d );
    break;
  case DISK_CPC:
    write_cpc( file,  d );
    break;
  case DISK_LOG:
    write_log( file,  d );
    break;
  default:
    return d->status = DISK_WRFILE;
    break;
  }

  if( d->status != DISK_OK ) {
    fclose( file );
    return d->status;
  }

  if( fclose( file ) == -1 )
    return d->status = DISK_WRFILE;
  
  return d->status = DISK_OK;
}
