/* trdos.c: Routines for handling the Betadisk interface
   Copyright (c) 2002-2003 Dmitry Sanarin, Fredrick Meunier, Philip Kendall

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

   Philip: pak21-fuse@srcf.ucam.org
     Post: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

   Dmitry: sdb386@hotmail.com
   Fred: fredm@spamcop.net
*/

#include <config.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "event.h"
#include "machine.h"
#include "spectrum.h"
#include "types.h"
#include "trdos.h"
#include "ui/ui.h"

#define TRDOS_DISC_SIZE 655360

typedef struct 
{
  char filename[PATH_MAX];
  int disc_ready;
  BYTE trk;
  int ro;
} discs_type;

#ifdef WORDS_BIGENDIAN

typedef struct
{
  unsigned b7  : 1;  /* This bit reflects the status of the Motor On output */
  unsigned b6  : 1;  /* disk is write-protected */
  unsigned b5  : 1;  /* When set, this bit indicates that the Motor Spin-Up
                        sequence has completed (6 revolutions) on type 1
                        commands. Type 2 & 3 commands, this bit indicates
                        record Type. 0 = Data Mark, 1 = Deleted Data Mark. */
  unsigned b4  : 1;  /* When set, it indicates that the desired track, sector,
                        or side were not found. This bit is reset when
                        updated. */
  unsigned b3  : 1;  /* If b4 is set, an error is found in one or more ID
                        fields; otherwise it indicates error in data field.
                        This bit is reset when updated. */
  unsigned b2  : 1;  /* When set, it indicates the computer did not respond to
                        DRQ in one byte time. This bit is reset to zero when
                        update. On type 1 commands, this bit reflects the
                        status of the TRACK 00 pin. */
  unsigned b1  : 1;  /* This bit is a copy of the DRQ output. When set, it
                        indicates the DR is full on a Read Operation or the DR
                        is empty on a write operation. This bit is reset to
                        zero when updated. On type 1 commands, this bit
                        indicates the status of the index pin. */
  unsigned b0  : 1;  /* When set, command is under execution. When reset, no
                        command is under execution. */
} rs_type;

#else				/* #ifdef WORDS_BIGENDIAN */

typedef struct
{
  unsigned b0  : 1;  /* When set, command is under execution. When reset, no
                        command is under execution. */
  unsigned b1  : 1;  /* This bit is a copy of the DRQ output. When set, it
                        indicates the DR is full on a Read Operation or the DR
                        is empty on a write operation. This bit is reset to
                        zero when updated. On type 1 commands, this bit
                        indicates the status of the index pin. */
  unsigned b2  : 1;  /* When set, it indicates the computer did not respond to
                        DRQ in one byte time. This bit is reset to zero when
                        update. On type 1 commands, this bit reflects the
                        status of the TRACK 00 pin. */
  unsigned b3  : 1;  /* If b4 is set, an error is found in one or more ID
                        fields; otherwise it indicates error in data field.
                        This bit is reset when updated. */
  unsigned b4  : 1;  /* When set, it indicates that the desired track, sector,
                        or side were not found. This bit is reset when
                        updated. */
  unsigned b5  : 1;  /* When set, this bit indicates that the Motor Spin-Up
                        sequence has completed (6 revolutions) on type 1
                        commands. Type 2 & 3 commands, this bit indicates
                        record Type. 0 = Data Mark, 1 = Deleted Data Mark. */
  unsigned b6  : 1;  /* disk is write-protected */
  unsigned b7  : 1;  /* This bit reflects the status of the Motor On output */
} rs_type;

#endif				/* #ifdef WORDS_BIGENDIAN */

# define CurrentDiscNum (last_vg93_system & 3)
# define CurrentDisk discs[CurrentDiscNum]

static int cmd_done = 0;
static int index_impulse = 0;

static int towrite = 0;
static int towriteaddr;

static int pointer;
static int side;

static BYTE track[TRDOS_DISC_SIZE];
static BYTE * toread;
static unsigned int toread_num = 0;
static unsigned int toread_position = 0;
static BYTE six_bytes[6];

static int vg_spin;
static int vg_direction; /* 0 - spindlewards 1 - rimwards */
static BYTE vg_reg_trk; /* The last byte sent to the VG93 track register  */
static BYTE vg_reg_sec; /* The last byte sent to the VG93 sector register */
static BYTE vg_reg_dat; /* The last byte sent to the VG93 data register   */
static BYTE vg_portFF_in;
static BYTE last_vg93_system = 0; /* The last byte sent to the VG93 system port */

union
{
  BYTE byte;
  rs_type bit;
} vg_rs; 

static discs_type discs[4];

int trdos_active=0;

/* The filenames (if any) used for the results of our SCL->TRD conversions */
static char *scl_files[2] = { NULL, NULL };

/* The file descriptors used mkstemp(3) returned for those temporary files */
static int scl_fds[2];

/* The template used for naming the results of the SCL->TRD conversion */
#define SCL_TMP_FILE_TEMPLATE "/tmp/fuse.scl.XXXXXX"

/* Remove any temporary file associated with drive 'which' */
static int remove_scl( trdos_drive_number which );

static int FileExists(const char * arg);
static int Scl2Trd( const char *oldname, const char *newname );
static int insert_trd( trdos_drive_number which, const char *filename );
static DWORD lsb2dw( BYTE *mem );
static void dw2lsb( BYTE *mem, DWORD value );

void
trdos_reset( void )
{
  trdos_active = 0;

  trdos_event_index( 0 );

  discs[0].disc_ready = 0;
  discs[1].disc_ready = 0;
  discs[2].disc_ready = 0;
  discs[3].disc_ready = 0;
}

void
trdos_end( void )
{
  int i;

  for( i=0; i<2; i++ ) remove_scl( i );
}

static
void trdos_update_index_impulse( void )
{
  if ( vg_spin ) 
  {
    vg_rs.bit.b1 = 0;
    if ( CurrentDisk.disc_ready && index_impulse ) {
      vg_rs.bit.b1 = 1;
    }
  }
}

BYTE
trdos_sr_read( WORD port GCC_UNUSED )
{
  trdos_update_index_impulse();

  if ( !CurrentDisk.disc_ready ) {
    return( 0x80 ); /* No disk in drive */
  }

  return( vg_rs.byte );
}

BYTE
trdos_tr_read( WORD port GCC_UNUSED )
{
  trdos_update_index_impulse();

  return( vg_reg_trk );
}

void
trdos_tr_write( WORD port GCC_UNUSED, BYTE b )
{
  vg_reg_trk = b;
}

BYTE
trdos_sec_read( WORD port GCC_UNUSED )
{
  trdos_update_index_impulse();

  return( vg_reg_sec );
}

void
trdos_sec_write( WORD port GCC_UNUSED, BYTE b )
{
  vg_reg_sec = b;
}

BYTE
trdos_dr_read( WORD port GCC_UNUSED )
{
  trdos_update_index_impulse();

  if ( toread_position >= toread_num )
    return ( vg_reg_dat );

  vg_reg_dat = toread[toread_position];

  if ( ( ( toread_position & 0x00ff ) == 0 ) && toread_position != 0 )
    vg_reg_sec++;

  if ( vg_reg_sec > 16 ) vg_reg_sec = 1;

  toread_position++;

  if ( toread_position == toread_num )
  {
    vg_portFF_in = 0x80;

    vg_rs.byte = 0;
  } else {
    vg_portFF_in = 0x40;

    vg_rs.bit.b0 = 1; /*  Busy */
    vg_rs.bit.b1 = 1; /*  DRQ copy */
  }

  return( vg_reg_dat );
}

void
trdos_dr_write( WORD port GCC_UNUSED, BYTE b )
{
  int trd_file;

  vg_reg_dat = b;

  if ( towrite == 0 )
    return;

  trd_file = open( CurrentDisk.filename, O_WRONLY );
  if ( trd_file == -1 )
    return;

  if ( lseek( trd_file, towriteaddr, SEEK_SET ) != towriteaddr )
  {
    towrite = 0;

    ui_error( UI_ERROR_ERROR,
              "trdos_dr_write : seek failed '%s'. pointer = %d",
              strerror(errno), towriteaddr );
    close( trd_file );
    return;
  }

  write( trd_file, &b, 1 );
  close( trd_file );

  towrite--;
  towriteaddr++;

  if ( towrite == 0 )
  {
    close( trd_file );
    vg_portFF_in = 0x80;
    vg_rs.byte = 0;
  } else
    vg_portFF_in = 0x40;
}

BYTE
trdos_sp_read( WORD port GCC_UNUSED )
{
  trdos_update_index_impulse();

  if ( cmd_done == 1 ) {
    return( vg_portFF_in &~ 0x40 );
  }

  return( vg_portFF_in );
}

void
trdos_sp_write( WORD port GCC_UNUSED, BYTE b )
{
  last_vg93_system = b;
}

static int
FileExists(const char * arg)
{
  struct stat buf;

  if ( stat( arg, &buf ) == -1 ) {
    return(0);
  }

  return(1);
}

static int
insert_trd( trdos_drive_number which, const char *filename )
{
  int fil;
  BYTE * temp;

  if ( !FileExists( filename ) ) {
    ui_error( UI_ERROR_INFO, "%s - No such file. Creating it", filename );

    fil = open( filename, O_CREAT|O_TRUNC|O_WRONLY, 00644 );

    if( fil == -1 ) {
      discs[which].disc_ready = 0;

      ui_error( UI_ERROR_ERROR,
              "Can't create %s. You must be able read and write this directory",
              filename );

      return 1;
    }

    temp = malloc( TRDOS_DISC_SIZE );
    memset( temp, TRDOS_DISC_SIZE, 0 );
    write( fil, temp, TRDOS_DISC_SIZE );
    free( temp );

    close( fil );
  }

  fil = open( filename, O_RDWR );

  if( fil == -1 ) {
    close( fil );
    fil = open( filename, O_RDONLY );

    if ( fil == -1 ) {	
      discs[which].disc_ready = 0;

      ui_error( UI_ERROR_INFO, "Can't open %s", filename );

      return 1;
    }

    discs[which].ro = 1;
  } else {
    discs[which].ro = 0;
  }	

  strcpy( discs[which].filename, filename );
  discs[which].disc_ready = 1;
  close( fil );

  return 0;
}

static int
remove_scl( trdos_drive_number which )
{
  int error = 0;

  if( scl_files[ which ] ) {

    if( close( scl_fds[ which ] ) ) {
      ui_error( UI_ERROR_ERROR, "couldn't close temporary file %s: %s",
		scl_files[ which ], strerror( errno ) );
    }

    error = unlink( scl_files[ which ] );
    if( error ) {
      ui_error( UI_ERROR_ERROR, "couldn't remove temporary .trd file %s: %s",
		scl_files[ which ], strerror( errno ) );
    }
    free( scl_files[ which ] ); scl_files[ which ] = 0;

  }

  return error;
}  

int
trdos_disk_insert_scl( trdos_drive_number which, const char *filename )
{
  char trd_template[] = SCL_TMP_FILE_TEMPLATE;
  int ret;

  ret = remove_scl( which ); if( ret ) return ret;

  scl_fds[ which ] = mkstemp( trd_template );
  if( scl_fds[ which ] == -1 ) {
    ui_error( UI_ERROR_ERROR, "couldn't get a temporary filename: %s",
	      strerror( errno ) );
    return ret;
  }

  scl_files[ which ] = malloc( strlen( trd_template ) + 1 );
  if( !scl_files[ which ] ) {
    unlink( trd_template );
    close( scl_fds[ which ] );
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }
  strcpy( scl_files[ which ], trd_template );

  if( ( ret = Scl2Trd( filename, trd_template ) ) ) {
    return ret;
  }

  strcpy( discs[which].filename, trd_template );
  discs[which].disc_ready = 1;
  discs[which].ro = 1;

  return 0;
}

int
trdos_disk_insert( trdos_drive_number which, const char *filename )
{
  char ext[4];

  sprintf(ext,"%s",&filename[strlen(filename)-3]);

  if ( strcasecmp( ext, "scl" ) == 0 ) {
    return trdos_disk_insert_scl( which, filename );
  }

  return insert_trd( which, filename );
}

int
trdos_disk_eject( trdos_drive_number which )
{
  remove_scl( which );

  discs[which].disc_ready = 0;

  return 0;
}

int
trdos_event_cmd_done( DWORD last_tstates GCC_UNUSED )
{
  cmd_done = 0;

  return 0;
}

int
trdos_event_index( DWORD last_tstates )
{
  int error;
  int next_tstates;
  static int num_calls = 0;

  if( num_calls == 0 ) {
    /* schedule next call in 20ms */
    next_tstates = 20 * machine_current->timings.processor_speed / 1000;
    index_impulse = 1;
    num_calls = 1;
  } else {
    /* schedule next call in 180ms */
    next_tstates = 180 * machine_current->timings.processor_speed / 1000;
    index_impulse = 0;
    num_calls = 0;
  }

  error = event_add( last_tstates + next_tstates, EVENT_TYPE_TRDOS_INDEX );
  if( error ) return error;

  return 0;
}

static void
vg_seek_delay(BYTE dst_track) 
{
  int error;

  if ( ( dst_track - CurrentDisk.trk ) > 0 )
    vg_direction = 1;
  if ( ( dst_track - CurrentDisk.trk ) < 0 )
    vg_direction = 0;

  cmd_done = 1;

  if ( !CurrentDisk.disc_ready ) vg_portFF_in = 0x80;
  else vg_portFF_in = 0x80;

  CurrentDisk.trk = dst_track;
  vg_reg_trk = dst_track;

  /* schedule event */
  error = event_add( tstates +
                     machine_current->timings.processor_speed / 1000 * 20 *
                     abs( dst_track - CurrentDisk.trk ),
                     EVENT_TYPE_TRDOS_CMD_DONE );
}

static void
vg_setFlagsSeeks( void )
{
  vg_rs.bit.b0 = 0;

  if ( CurrentDisk.trk == 0 )
    vg_rs.bit.b2 = 1;
  else
    vg_rs.bit.b2 = 0;

  vg_rs.bit.b3 = 0;
  vg_rs.bit.b4 = 0;
  vg_rs.bit.b6 = CurrentDisk.ro;
  vg_rs.bit.b7 = CurrentDisk.disc_ready;
}

void
trdos_cr_write( WORD port GCC_UNUSED, BYTE b )
{
  int trd_file, error;

  if ( last_vg93_system & 0x10 )
    side = 0;
  else
    side = 1;


  if ( (b & 0xF0) == 0xD0 ) { /*  interrupt */
    vg_portFF_in = 0x80;
    vg_setFlagsSeeks();

    return;
  }	

  if ( (b & 0xF0) == 0x00 ) { /*  seek trk0 AKA Initialisation */
    vg_seek_delay( 0 );
    vg_rs.bit.b5 = b & 8 ? 1 : 0;
    vg_setFlagsSeeks();
    if ( b & 8 ) vg_spin = 1;

    return;
  }

  if ( (b & 0xF0) == 0x10 ) { /*  seek track */
    vg_seek_delay( vg_reg_dat );
    vg_rs.bit.b5 = b & 8 ? 1 : 0;
    vg_setFlagsSeeks();
    if ( b & 8 ) vg_spin = 1;

    return;
  }

  if ( (b & 0xE0) == 0x40 ) { /*  fwd */
    vg_direction = 1;
    b = 0x20; /*  step */
  }
  if ( (b & 0xE0) == 0x60 ) { /*  back */
    vg_direction = 0;
    b = 0x20; /*  step */
  }
  if ( (b & 0xE0) == 0x20 ) { /*  step */
    if ( vg_direction )
      vg_seek_delay( ++CurrentDisk.trk );
    else
      vg_seek_delay( --CurrentDisk.trk );

    vg_rs.bit.b5 = 1;
    vg_setFlagsSeeks();
    vg_spin = 1;

    return;
  }

  if ( (b & 0xE0) == 0x80 ) { /*  readsec */
    vg_rs.byte = 0x81;
    vg_portFF_in = 0x40;
    vg_spin = 0;

    if ( !CurrentDisk.disc_ready ) {
      vg_rs.byte = 0x90;
      vg_portFF_in = 0x80; 

      ui_error( UI_ERROR_INFO, "No disk in drive %c", 'A' + CurrentDiscNum );

      return;
    }

    if ( ( vg_reg_sec == 0 ) || ( vg_reg_sec > 16 ) ) {
      vg_rs.byte |= 0x10; /*  sector not found */
      vg_portFF_in = 0x80;

      ui_error( UI_ERROR_INFO, "((vg_reg_sec==0)||(vg_reg_sec>16))" );

      return;
    }

    trd_file = open( CurrentDisk.filename, O_RDONLY );

    if ( trd_file == -1 ) {
      ui_error( UI_ERROR_ERROR,
                "trdos_cr_write : Can't open file %s last_vg93_system=%x",
                CurrentDisk.filename, last_vg93_system );

      vg_rs.byte = 0x90;
      vg_portFF_in = 0x80; 

      return;
    }

    pointer = ( CurrentDisk.trk * 2 + side ) * 256 * 16 + ( vg_reg_sec - 1 ) *
              256;

    if ( lseek( trd_file, pointer, SEEK_SET) != pointer ) {
      ui_error( UI_ERROR_ERROR,
                "trdos_cr_write : seek failed, pointer = %d", pointer );
      close( trd_file );

      vg_rs.byte |= 0x10; /*  sector not found */
      vg_portFF_in = 0x80;

      return;
    }

    if ( b & 0x10 ) {
      ui_error( UI_ERROR_ERROR,
                "Unimplemented multi sector read:read vg_reg_sec=%d",
                vg_reg_sec );

      read( trd_file, track, 256 * ( 17 - vg_reg_sec ) );

      lseek( trd_file, -256 * ( ( 17 - vg_reg_sec ) + ( vg_reg_sec - 1 ) ),
             SEEK_CUR );
      read( trd_file, track, 256 * ( vg_reg_sec - 1 ) );
 
      close( trd_file );

      toread = track;
      toread_num = 256 * ( 16 );
      toread_position =  0;

      /* vg_portFF_in=0x80; */
      /* todo : Eto proverit' !!! */
    } else {
      if ( read( trd_file, track, 256 ) == 256 ) {
        close( trd_file );

        toread = track;
        toread_num = 256;
        toread_position = 0;

        /* schedule event */
        cmd_done = 1;
        error = event_add( tstates + machine_current->timings.processor_speed
                                     / 1000 * 30,
                           EVENT_TYPE_TRDOS_CMD_DONE );
      } else {
        close( trd_file );

        vg_rs.byte |= 0x10; /*  sector not found */
        vg_portFF_in = 0x80;
        
        ui_error( UI_ERROR_ERROR, "read(trd_file, track, 256)!=256" );
      }
    }

    return;
  }

  if ( (b & 0xFB) == 0xC0 ) { /*  read adr */
    vg_rs.byte = 0x81;
    vg_portFF_in = 0x40;

    six_bytes[0] = CurrentDisk.trk;
    six_bytes[1] = 0; 
    six_bytes[2] = vg_reg_sec;
    six_bytes[3] = 1;
    six_bytes[4] = 0; /*  todo : crc !!! */
    six_bytes[5] = 0; /*  todo : crc !!! */

    toread = six_bytes;
    toread_num = 6;
    toread_position = 0;

    /* schedule event */
    cmd_done = 1;
    vg_spin = 0;

    error = event_add( tstates + machine_current->timings.processor_speed
                                 / 1000 * 30,
                       EVENT_TYPE_TRDOS_CMD_DONE );

    return;
  }	

  if ( (b & 0xE0) == 0xA0 ) { /*  writesec */
    vg_rs.byte = 0x81;
    vg_portFF_in = 0x40;
    vg_spin = 0;


    if ( CurrentDisk.ro ) {
      vg_rs.byte = 0x60;
      vg_portFF_in = 0x80;
      return;
    }

    if ( !CurrentDisk.disc_ready ) {
      vg_rs.byte = 0x90;
      vg_portFF_in = 0x80; 

      ui_error( UI_ERROR_ERROR, "No disk" );

      return;
    }

    if ( ( vg_reg_sec == 0 ) || ( vg_reg_sec > 16 ) ) {
      vg_rs.byte |= 0x10; /*  sector not found */
      vg_portFF_in = 0x80;

      ui_error( UI_ERROR_ERROR, "((vg_reg_sec==0)||(vg_reg_sec>16))" );

      return;
    }

    towriteaddr = ( CurrentDisk.trk * 2 + side ) * 256 * 16
                    + ( vg_reg_sec - 1 ) * 256;
    towrite = 256;

    vg_spin = 0;

    return;		
  }		
}

#define TRD_NAMEOFFSET 0x08F5
#define TRD_DIRSTART 0x08E2
#define TRD_DIRLEN 32
#define TRD_MAXNAMELENGTH 8
#define BLOCKSIZE 10240

typedef union {
#ifdef WORDS_BIGENDIAN
  struct { BYTE b3, b2, b1, b0; } b;
#else
  struct { BYTE b0, b1, b2, b3; } b;
#endif
  DWORD dword;
} lsb_dword;

static DWORD 
lsb2dw( BYTE *mem )
{
  return ( mem[0] + ( mem[1] * 256 ) + ( mem[2] * 256 * 256 )
          + ( mem[3] * 256 * 256 * 256 ) );
}

static void
dw2lsb( BYTE *mem, DWORD value )
{
  lsb_dword ret;

  ret.dword = value;

  mem[0] = ret.b.b0;
  mem[1] = ret.b.b1;
  mem[2] = ret.b.b2;
  mem[3] = ret.b.b3;
}

static int 
Scl2Trd( const char *oldname, const char *newname )
{
  int TRD, SCL, i;

  void *TRDh;

  BYTE *trd_free;
  BYTE *trd_fsec;
  BYTE *trd_ftrk;
  BYTE *trd_files;
  BYTE size;

  char signature[8];
  BYTE blocks;
  BYTE headers[256][14];
  void *tmpscl;
  DWORD left;
  DWORD fptr;
  size_t x;

  BYTE template[34] =
  { 0x01, 0x16, 0x00, 0xF0,
    0x09, 0x10, 0x00, 0x00,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x00, 0x00, 0x64,
    0x69, 0x73, 0x6B, 0x6E,
    0x61, 0x6D, 0x65, 0x00,
    0x00, 0x00, 0x46, 0x55
  };

  FILE *fh;
  BYTE *mem;

  fh = fopen( newname, "w" );
  if( fh ) {
    mem = (BYTE *) malloc( BLOCKSIZE );
    memset( mem, 0, BLOCKSIZE );

    if( mem ) {
      memcpy( &mem[TRD_DIRSTART], template, TRD_DIRLEN );
      strncpy( &mem[TRD_NAMEOFFSET], "Fuse", TRD_MAXNAMELENGTH );
      fwrite( (void *) mem, 1, BLOCKSIZE, fh );
      memset( mem, 0, BLOCKSIZE );

      for( i = 0; i < 63; i++ )
	fwrite( (void *)mem, 1, BLOCKSIZE, fh );

      free( mem );
      fclose( fh );
    }
  }

  if( ( TRD = open( newname, O_RDWR ) ) == -1 ) {
    ui_error( UI_ERROR_ERROR, "Error - cannot open TRD disk image %s",
              newname );
    return 1;
  }

  TRDh = malloc( 4096 );
  read( TRD, TRDh, 4096 );

  trd_free = (BYTE *) TRDh + 0x8E5;
  trd_files = (BYTE *) TRDh + 0x8E4;
  trd_fsec = (BYTE *) TRDh + 0x8E1;
  trd_ftrk = (BYTE *) TRDh + 0x8E2;

  if( ( SCL = open( oldname, O_RDONLY ) ) == -1 ) {
    ui_error( UI_ERROR_ERROR, "Can't open SCL file %s", oldname );
    close( TRD );
    close( SCL );
    return 1;
  }

  read( SCL, &signature, 8 );

  if( strncasecmp( signature, "SINCLAIR", 8 ) ) {
    ui_error( UI_ERROR_ERROR, "Wrong signature=%s", signature );
    close( TRD );
    close( SCL );
    return 1;
  }

  read( SCL, &blocks, 1 );

  for( x = 0; x < blocks; x++ )
    read( SCL, &(headers[x][0]), 14 );

  for( x = 0; x < blocks; x++ ) {
    size = headers[x][13];
    if( lsb2dw(trd_free) < size ) {
      ui_error( UI_ERROR_ERROR,
                "File is too long to fit in the image *trd_free=%u < size=%u",
                lsb2dw( trd_free ), size );
      close( SCL );
      goto Finish;
    }

    if( *trd_files > 127 ) {
      ui_error( UI_ERROR_ERROR, "Image is full" );
      close( SCL );
      goto Finish;
    }

    memcpy( (void *) ((BYTE *) TRDh + *trd_files * 16),
	    (void *) headers[x], 14 );

    memcpy( (void *) ((BYTE *) TRDh + *trd_files * 16 + 0x0E ),
	    (void *) trd_fsec, 2 );

    tmpscl = malloc( 32000 );

    left = (DWORD) ( headers[x][13] ) * 256L;
    fptr = (*trd_ftrk) * 4096L + (*trd_fsec) * 256L;
    lseek( TRD, fptr, SEEK_SET );

    while ( left > 32000 ) {
      read( SCL, tmpscl, 32000 );
      write( TRD, tmpscl, 32000 );
      left -= 32000;
    }

    read( SCL, tmpscl, left );
    write( TRD, tmpscl, left );

    free( tmpscl );

    (*trd_files)++;

    dw2lsb( trd_free, lsb2dw(trd_free) - size );

    while ( size > 15 ) {
      (*trd_ftrk)++;
      size -= 16;
    }

    (*trd_fsec) += size;
    while ( (*trd_fsec) > 15 ) {
      (*trd_fsec) -= 16;
      (*trd_ftrk)++;
    }
  }

  close( SCL );

Finish:
  lseek( TRD, 0L, SEEK_SET );
  write( TRD, TRDh, 4096 );
  close( TRD );
  free( TRDh );

  return 0;
}
