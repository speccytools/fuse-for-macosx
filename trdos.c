/* trdos.c: Routines for handling the Betadisk interface
   Copyright (c) 2002-2004 Dmitry Sanarin, Fredrick Meunier, Philip Kendall

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
#ifdef HAVE_STRINGS_H
#include <strings.h>		/* Needed for strncasecmp() on QNX6 */
#endif				/* #ifdef HAVE_STRINGS_H */
#include <limits.h>
#include <sys/stat.h>

#include <libspectrum.h>

#include "compat.h"
#include "event.h"
#include "machine.h"
#include "memory.h"
#include "module.h"
#include "settings.h"
#include "spectrum.h"
#include "trdos.h"
#include "ui/ui.h"
#include "utils.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

#define TRDOS_DISC_SIZE 655360

typedef struct 
{
  int disc_ready;		/* Is this disk present? */

  char filename[PATH_MAX];	/* The filename we used to open this disk */
  int fd;			/* The fd will we use to access this disk */

  int ro;			/* True if we have read-only access to this
				   disk */

  int dirty;			/* Has this disk been written to? */

  libspectrum_byte trk;

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
  unsigned b3  : 1;  /* If this is set, an error is found in one or more ID
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
  unsigned b3  : 1;  /* If this is set, an error is found in one or more ID
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

# define CurrentDiscNum ( trdos_system_register & 3 )
# define CurrentDisk discs[CurrentDiscNum]

static int busy = 0;
static int index_impulse = 0;

static int towrite = 0;
static int towriteaddr;

static int pointer;
static int side;

static libspectrum_byte track[ TRDOS_DISC_SIZE ];
static libspectrum_byte *toread;
static unsigned int toread_num = 0;
static unsigned int toread_position = 0;
static libspectrum_byte six_bytes[6];

static int vg_spin;
static int trdos_direction; /* 0 - spindlewards 1 - rimwards */
static libspectrum_byte trdos_status_register; /* Betadisk status register */
static libspectrum_byte trdos_track_register;  /* FDC track register */
static libspectrum_byte trdos_sector_register; /* FDC sector register */
static libspectrum_byte trdos_data_register;   /* FDC data register */
static libspectrum_byte vg_portFF_in;
static libspectrum_byte trdos_system_register; /* FDC system register */

union
{
  libspectrum_byte byte;
  rs_type bit;
} vg_rs; 

static discs_type discs[4];

int trdos_available = 0;
int trdos_active=0;

/* The template used for naming the results of the SCL->TRD conversion */
#define SCL_TMP_FILE_TEMPLATE "fuse.scl.XXXXXX"

/* The template used for the temporary copy of a .trd file */
static const char *trd_template = "fuse.trd.XXXXXX";

static int Scl2Trd( const char *oldname, int TRD );
static int insert_scl( trdos_drive_number which, const char *filename );
static int insert_trd( trdos_drive_number which, const char *filename );
static libspectrum_dword lsb2dw( libspectrum_byte *mem );
static void dw2lsb( libspectrum_byte *mem, libspectrum_dword value );

static void trdos_memory_map( void );
static void trdos_from_snapshot( libspectrum_snap *snap );
static void trdos_to_snapshot( libspectrum_snap *snap );

static module_info_t trdos_module_info = {

  NULL,
  trdos_memory_map,
  trdos_from_snapshot,
  trdos_to_snapshot,

};

int
trdos_init( void )
{
  discs[0].disc_ready = 0;
  discs[1].disc_ready = 0;
  discs[2].disc_ready = 0;
  discs[3].disc_ready = 0;

  module_register( &trdos_module_info );

  return 0;
}

void
trdos_reset( void )
{
  trdos_active = 0;
  busy = 0;

  trdos_event_index( 0 );

  /* We can eject disks only if they are currently present */
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_TRDOS_A_EJECT,
		    discs[ TRDOS_DRIVE_A ].disc_ready );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_TRDOS_B_EJECT,
		    discs[ TRDOS_DRIVE_B ].disc_ready );

  ui_statusbar_update( UI_STATUSBAR_ITEM_DISK, UI_STATUSBAR_STATE_INACTIVE );
}

void
trdos_end( void )
{
  trdos_available = 0;
}

void
trdos_page( void )
{
  trdos_active = 1;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();
}

void
trdos_unpage( void )
{
  trdos_active = 0;
  machine_current->ram.romcs = 0;
  machine_current->memory_map();
}

static void
trdos_memory_map( void )
{
  if( !( machine_current->capabilities &
	 LIBSPECTRUM_MACHINE_CAPABILITY_TRDOS_DISK ) ) return;

  memory_map_read[0] = memory_map_write[0] = memory_map_romcs[0];
  memory_map_read[1] = memory_map_write[1] = memory_map_romcs[1];
}
  
static void
trdos_from_snapshot( libspectrum_snap *snap )
{
  if( !( machine_current->capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TRDOS_DISK ) )
    return;

  trdos_active = libspectrum_snap_beta_paged( snap );
  
  if( trdos_active ) {
    trdos_page();
  } else {
    trdos_unpage();
  }

  trdos_direction = libspectrum_snap_beta_direction( snap );

  trdos_cr_write ( 0x001f, libspectrum_snap_beta_status( snap ) );
  trdos_tr_write ( 0x003f, libspectrum_snap_beta_track ( snap ) );
  trdos_sec_write( 0x005f, libspectrum_snap_beta_sector( snap ) );
  trdos_dr_write ( 0x007f, libspectrum_snap_beta_data  ( snap ) );
  trdos_sp_write ( 0x00ff, libspectrum_snap_beta_system( snap ) );
}

static void
trdos_to_snapshot( libspectrum_snap *snap )
{
  libspectrum_snap_set_beta_paged( snap, trdos_active );
  libspectrum_snap_set_beta_direction( snap, trdos_direction );
  libspectrum_snap_set_beta_status( snap, trdos_status_register );
  libspectrum_snap_set_beta_track ( snap, trdos_system_register );
  libspectrum_snap_set_beta_sector( snap, trdos_track_register );
  libspectrum_snap_set_beta_data  ( snap, trdos_sector_register );
  libspectrum_snap_set_beta_system( snap, trdos_data_register );
}

static
void trdos_update_index_impulse( void )
{
  if ( vg_spin ) {
    vg_rs.bit.b1 = 0;
    if ( CurrentDisk.disc_ready && index_impulse ) {
      vg_rs.bit.b1 = 1;
    }
  }

  ui_statusbar_update( UI_STATUSBAR_ITEM_DISK,
                       busy ? UI_STATUSBAR_STATE_ACTIVE :
                              UI_STATUSBAR_STATE_INACTIVE );
}

libspectrum_byte
trdos_sr_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !trdos_active ) return 0xff;

  *attached = 1;

  trdos_update_index_impulse();

  if ( !CurrentDisk.disc_ready ) {
    return( 0x80 ); /* No disk in drive */
  }

  return( vg_rs.byte );
}

libspectrum_byte
trdos_tr_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !trdos_active ) return 0xff;

  *attached = 1;

  trdos_update_index_impulse();

  return trdos_track_register;
}

void
trdos_tr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !trdos_active ) return;

  trdos_track_register = b;
}

libspectrum_byte
trdos_sec_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !trdos_active ) return 0xff;

  *attached = 1;

  trdos_update_index_impulse();

  return trdos_sector_register;
}

void
trdos_sec_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !trdos_active ) return;

  trdos_sector_register = b;
}

libspectrum_byte
trdos_dr_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !trdos_active ) return 0xff;

  *attached = 1;

  trdos_update_index_impulse();

  if( toread_position >= toread_num ) return trdos_data_register;

  trdos_data_register = toread[toread_position];

  if ( ( ( toread_position & 0x00ff ) == 0 ) && toread_position != 0 )
    trdos_sector_register++;

  if( trdos_sector_register > 16 ) trdos_sector_register = 1;

  toread_position++;

  if ( toread_position == toread_num ) {
    vg_portFF_in = 0x80;

    vg_rs.byte = 0;
  } else {
    vg_portFF_in = 0x40;

    vg_rs.bit.b0 = 1; /*  Busy */
    vg_rs.bit.b1 = 1; /*  DRQ copy */
  }

  return trdos_data_register;
}

void
trdos_dr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !trdos_active ) return;

  trdos_data_register = b;

  if( towrite == 0 ) return;

  if( lseek( CurrentDisk.fd, towriteaddr, SEEK_SET ) == -1 ) {
    towrite = 0;

    ui_error( UI_ERROR_ERROR,
              "trdos_dr_write: seeking in '%s' failed: %s",
	      CurrentDisk.filename, strerror( errno ) );
    return;
  }

  write( CurrentDisk.fd, &b, 1 );
  CurrentDisk.dirty = 1;

  towrite--;
  towriteaddr++;

  if( towrite == 0 ) {
    vg_portFF_in = 0x80;
    vg_rs.byte = 0;
  } else {
    vg_portFF_in = 0x40;
  }
}

libspectrum_byte
trdos_sp_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !trdos_active ) return 0xff;

  *attached = 1;

  trdos_update_index_impulse();

  if ( busy == 1 ) {
    return( vg_portFF_in &~ 0x40 );
  }

  return( vg_portFF_in );
}

void
trdos_sp_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !trdos_active ) return;

  trdos_system_register = b;
}

static int
insert_trd( trdos_drive_number which, const char *filename )
{
  int fd, error;
  char tempfilename[ PATH_MAX ];

  /* Make a temporary copy of the disk */
  error = utils_make_temp_file( &fd, tempfilename, filename, trd_template );
  if( error ) return error;

  unlink( tempfilename );

  discs[which].ro = 0;
  discs[which].fd = fd;
  strcpy( discs[which].filename, filename );
  discs[which].disc_ready = 1;

  return 0;
}

static int
insert_scl( trdos_drive_number which, const char *filename )
{
  const char *temp_path; size_t length;
  char* tempfilename;
  int ret;

  temp_path = utils_get_temp_path();

  /* +2 is for the slash between the path and the template and for the
     null at the end */
  length = strlen( temp_path ) + strlen( SCL_TMP_FILE_TEMPLATE ) + 2;

  tempfilename = malloc( length );
  if( !tempfilename ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  snprintf( tempfilename, length, "%s" FUSE_DIR_SEP_STR "%s", temp_path,
            SCL_TMP_FILE_TEMPLATE );

  discs[ which ].disc_ready = 0;

  discs[ which ].fd = mkstemp( tempfilename );
  if( discs[ which ].fd == -1 ) {
    ui_error( UI_ERROR_ERROR, "couldn't get a temporary filename: %s",
	      strerror( errno ) );
    free( tempfilename );
    return 1;
  }

  /* Unlink the file so it will be removed when the fd is closed */
  unlink( tempfilename );

  if( ( ret = Scl2Trd( filename, discs[ which ].fd ) ) ) {
    close( discs[ which ].fd );
    free( tempfilename );
    return ret;
  }

  strcpy( discs[which].filename, tempfilename );
  discs[which].disc_ready = 1;
  discs[which].ro = 1;

  free( tempfilename );

  return 0;
}

int
trdos_disk_insert_default_autoload( trdos_drive_number which,
                                    const char *filename )
{
  return trdos_disk_insert( which, filename, settings_current.auto_load );
}

int
trdos_disk_insert( trdos_drive_number which, const char *filename,
                   int autoload )
{
  int error;
  utils_file file;
  libspectrum_id_t type;
  libspectrum_class_t class;

  if( discs[ which ].disc_ready ) {
    if( trdos_disk_eject( which, 0 ) ) return 1;
  }

  if( utils_read_file( filename, &file ) ) return 1;

  if( libspectrum_identify_file_with_class( &type, &class, filename,
					    file.buffer, file.length ) ) {
    utils_close_file( &file );
    return 1;
  }

  if( class != LIBSPECTRUM_CLASS_DISK_TRDOS ) {
    ui_error( UI_ERROR_ERROR,
              "trdos_disk_insert: file `%s' is not a TR-DOS disk", filename );
    return 1;
  }

  if( utils_close_file( &file ) ) return 1;

  switch( type ) {
  case LIBSPECTRUM_ID_DISK_SCL:
    error = insert_scl( which, filename );
    break;
  case LIBSPECTRUM_ID_DISK_TRD:
    error = insert_trd( which, filename );
    break;
  default:
    ui_error( UI_ERROR_ERROR,
              "trdos_disk_insert: file `%s' is an unsupported TR-DOS disk", filename );
    error = 1;
  }

  discs[ which ].dirty = 0;

  if( error ) return error;

  /* Set the `eject' item active */
  ui_menu_activate(
    which == TRDOS_DRIVE_A ? UI_MENU_ITEM_MEDIA_DISK_TRDOS_A_EJECT :
			     UI_MENU_ITEM_MEDIA_DISK_TRDOS_B_EJECT  ,
    1
  );

  if( autoload ) {
    PC = 0;
    machine_current->ram.last_byte |= 0x10;   /* Select ROM 1 */
    trdos_page();
  }

  return 0;
}

int
trdos_disk_eject( trdos_drive_number which, int write )
{
  int error;

  if( write ) {

    if( ui_trdos_disk_write( which ) ) return 1;

  } else {

    if( discs[ which ].dirty ) {
    
      ui_confirm_save_t confirm = ui_confirm_save(
        "Disk has been modified.\nDo you want to save it?"
      );
  
      switch( confirm ) {

      case UI_CONFIRM_SAVE_SAVE:
	if( ui_trdos_disk_write( which ) ) return 1;
	break;

      case UI_CONFIRM_SAVE_DONTSAVE:
	error = close( discs[ which ].fd );
	if( error ) {
	  ui_error( UI_ERROR_ERROR, "Error closing '%s': %s",
		    discs[which].filename, strerror( errno ) );
	  return 1;
	}
	discs[ which ].fd = -1;
	break;

      case UI_CONFIRM_SAVE_CANCEL: return 1;

      }
    }

  }

  discs[which].disc_ready = 0;

  /* Set the `eject' item inactive */
  ui_menu_activate(
    which == TRDOS_DRIVE_A ? UI_MENU_ITEM_MEDIA_DISK_TRDOS_A_EJECT :
			     UI_MENU_ITEM_MEDIA_DISK_TRDOS_B_EJECT  ,
    0
  );

  return 0;
}

int
trdos_disk_write( trdos_drive_number which, const char *filename )
{
  FILE *f;
  utils_file file;
  int error;
  ssize_t bytes_written;

  f = fopen( filename, "wb" );
  if( !f ) {
    ui_error( UI_ERROR_ERROR, "couldn't open '%s' for writing: %s", filename,
	      strerror( errno ) );
  }

  error = utils_read_fd( discs[ which ].fd, discs[ which ].filename, &file );
  if( error ) { fclose( f ); return error; }

  bytes_written = fwrite( file.buffer, 1, file.length, f );
  if( bytes_written != file.length ) {
    ui_error( UI_ERROR_ERROR, "could write only %lu of %lu bytes to '%s'",
	      (unsigned long)bytes_written, (unsigned long)file.length,
	      filename );
    utils_close_file( &file ); fclose( f );
  }

  error = utils_close_file( &file ); if( error ) { fclose( f ); return error; }

  if( fclose( f ) ) {
    ui_error( UI_ERROR_ERROR, "error closing '%s': %s", filename,
	      strerror( errno ) );
    return 1;
  }

  return 0;
}

int
trdos_event_cmd_done( libspectrum_dword last_tstates GCC_UNUSED )
{
  busy = 0;

  return 0;
}

int
trdos_event_index( libspectrum_dword last_tstates )
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
vg_seek_delay( libspectrum_byte dst_track ) 
{
  int error;

  if( ( dst_track - CurrentDisk.trk ) > 0 ) trdos_direction = 1;
  if( ( dst_track - CurrentDisk.trk ) < 0 ) trdos_direction = 0;

  busy = 1;

  /* FIXME: is this code really what was meant? */
  if ( !CurrentDisk.disc_ready ) vg_portFF_in = 0x80;
  else vg_portFF_in = 0x80;

  CurrentDisk.trk = dst_track;
  trdos_track_register = dst_track;

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
trdos_cr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  int error;

  if( !trdos_active ) return;

  trdos_status_register = b;

  side = trdos_system_register & 0x10 ? 0 : 1;

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
    vg_seek_delay( trdos_data_register );
    vg_rs.bit.b5 = b & 8 ? 1 : 0;
    vg_setFlagsSeeks();
    if ( b & 8 ) vg_spin = 1;

    return;
  }

  if ( (b & 0xE0) == 0x40 ) { /*  fwd */
    trdos_direction = 1;
    b = 0x20; /*  step */
  }
  if ( (b & 0xE0) == 0x60 ) { /*  back */
    trdos_direction = 0;
    b = 0x20; /*  step */
  }
  if ( (b & 0xE0) == 0x20 ) { /*  step */
    if ( trdos_direction )
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
      return;
    }

    if( ( trdos_sector_register == 0 ) || ( trdos_sector_register > 16 ) ) {
      vg_rs.byte |= 0x10; /*  sector not found */
      vg_portFF_in = 0x80;

      ui_error( UI_ERROR_ERROR,
		"Attempt to read TRDOS sector with sector register 0x%02x",
		trdos_sector_register );

      return;
    }

    pointer = ( CurrentDisk.trk * 2 + side ) * 256 * 16 +
              ( trdos_sector_register - 1 ) * 256;

    if( lseek( CurrentDisk.fd, pointer, SEEK_SET) == -1 ) {
      ui_error( UI_ERROR_ERROR,
		"trdos_cr_write: seeking in '%s' failed: %s",
		CurrentDisk.filename, strerror( errno ) );

      vg_rs.byte |= 0x10; /*  sector not found */
      vg_portFF_in = 0x80;

      return;
    }

    if ( b & 0x10 ) {
      ui_error( UI_ERROR_ERROR,
                "Unimplemented TRDOS multisector read: sector register = %d",
                trdos_sector_register );

      read( CurrentDisk.fd, track, 256 * ( 17 - trdos_sector_register ) );

      lseek( CurrentDisk.fd,

	     /* This line used to say:
	        -256 * ( ( 17 - vg_reg_sec ) + ( vg_reg_sec - 1 ) ),
		which is fairly nonsensical */
             -256 * 16,

             SEEK_CUR );

      read( CurrentDisk.fd, track, 256 * ( trdos_sector_register - 1 ) );
 
      toread = track;
      toread_num = 256 * ( 16 );
      toread_position =  0;

      /* vg_portFF_in=0x80; */
      /* todo : Eto proverit' !!! */
    } else {
      if( read( CurrentDisk.fd, track, 256 ) == 256 ) {

        toread = track;
        toread_num = 256;
        toread_position = 0;

        /* schedule event */
        busy = 1;
        error = event_add( tstates + machine_current->timings.processor_speed
                                     / 1000 * 30,
                           EVENT_TYPE_TRDOS_CMD_DONE );
      } else {

        vg_rs.byte |= 0x10; /*  sector not found */
        vg_portFF_in = 0x80;
        
        ui_error( UI_ERROR_ERROR, "Error reading from '%s'",
		  CurrentDisk.filename );
      }
    }

    return;
  }

  if ( (b & 0xFB) == 0xC0 ) { /*  read adr */
    vg_rs.byte = 0x81;
    vg_portFF_in = 0x40;

    six_bytes[0] = CurrentDisk.trk;
    six_bytes[1] = 0; 
    six_bytes[2] = trdos_sector_register;
    six_bytes[3] = 1;
    six_bytes[4] = 0; /*  todo : crc !!! */
    six_bytes[5] = 0; /*  todo : crc !!! */

    toread = six_bytes;
    toread_num = 6;
    toread_position = 0;

    /* schedule event */
    busy = 1;
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

    if ( ( trdos_sector_register == 0 ) || ( trdos_sector_register > 16 ) ) {
      vg_rs.byte |= 0x10; /*  sector not found */
      vg_portFF_in = 0x80;

      ui_error( UI_ERROR_ERROR,
		"Attempt to write TRDOS sector with sector register 0x%02x",
		trdos_sector_register );

      return;
    }

    towriteaddr = ( CurrentDisk.trk * 2 + side ) * 256 * 16
                    + ( trdos_sector_register - 1 ) * 256;
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
  struct { libspectrum_byte b3, b2, b1, b0; } b;
#else
  struct { libspectrum_byte b0, b1, b2, b3; } b;
#endif
  libspectrum_dword dword;
} lsb_dword;

static libspectrum_dword 
lsb2dw( libspectrum_byte *mem )
{
  return ( mem[0] + ( mem[1] * 256 ) + ( mem[2] * 256 * 256 )
          + ( mem[3] * 256 * 256 * 256 ) );
}

static void
dw2lsb( libspectrum_byte *mem, libspectrum_dword value )
{
  lsb_dword ret;

  ret.dword = value;

  mem[0] = ret.b.b0;
  mem[1] = ret.b.b1;
  mem[2] = ret.b.b2;
  mem[3] = ret.b.b3;
}

static int
Scl2Trd( const char *oldname, int TRD )
{
  int SCL, i;

  libspectrum_byte *TRDh;

  libspectrum_byte *trd_free;
  libspectrum_byte *trd_fsec;
  libspectrum_byte *trd_ftrk;
  libspectrum_byte *trd_files;
  libspectrum_byte size;

  char signature[8];
  libspectrum_byte blocks;
  libspectrum_byte headers[256][14];
  void *tmpscl;
  libspectrum_dword left;
  libspectrum_dword fptr;
  size_t x;

  ssize_t written;

  libspectrum_byte template[34] = { 
    0x01, 0x16, 0x00, 0xF0,
    0x09, 0x10, 0x00, 0x00,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x00, 0x00, 0x64,
    0x69, 0x73, 0x6B, 0x6E,
    0x61, 0x6D, 0x65, 0x00,
    0x00, 0x00, 0x46, 0x55
  };

  libspectrum_byte *mem;

  mem = malloc( BLOCKSIZE );
  if( !mem ) {
    ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  memset( mem, 0, BLOCKSIZE );

  memcpy( &mem[TRD_DIRSTART], template, TRD_DIRLEN );
  memcpy( &mem[TRD_NAMEOFFSET], "Fuse", 5 );

  written = write( TRD, mem, BLOCKSIZE );
  if( written != BLOCKSIZE ) {
    ui_error( UI_ERROR_ERROR, "Error writing to temporary TRD file" );
    free( mem );
    return 1;
  }

  memset( mem, 0, BLOCKSIZE );

  for( i = 0; i < 63; i++ ) {

    written = write( TRD, mem, BLOCKSIZE );
    if( written != BLOCKSIZE ) {
      ui_error( UI_ERROR_ERROR, "Error writing to temporary TRD file" );
      free( mem );
      return 1;
    }
  }

  free( mem );

  if( lseek( TRD, 0, SEEK_SET ) == -1 ) {
    ui_error( UI_ERROR_ERROR, "Error seeking in temporary TRD file" );
    return 1;
  }
  
  TRDh = malloc( 4096 );
  if( !TRDh ) {
    ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  if( read( TRD, TRDh, 4096 ) != 4096 ) {
    ui_error( UI_ERROR_ERROR, "Error reading from temporary TRD file" );
    free( TRDh );
    return 1;
  }

  trd_free = TRDh + 0x8E5;
  trd_files = TRDh + 0x8E4;
  trd_fsec = TRDh + 0x8E1;
  trd_ftrk = TRDh + 0x8E2;

  if( ( SCL = open( oldname, O_RDONLY | O_BINARY ) ) == -1 ) {
    ui_error( UI_ERROR_ERROR, "Can't open SCL file '%s': %s", oldname,
	      strerror( errno ) );
    free( TRDh );
    return 1;
  }

  if( read( SCL, &signature, 8 ) != 8 ) {
    ui_error( UI_ERROR_ERROR, "Error reading from '%s'", oldname );
    close( SCL ); free( TRDh );
    return 1;
  }

  if( strncasecmp( signature, "SINCLAIR", 8 ) ) {
    ui_error( UI_ERROR_ERROR, "SCL file '%s' has the wrong signature",
	      oldname );
    close( SCL ); free( TRDh );
    return 1;
  }

  if( read( SCL, &blocks, 1 ) != 1 ) {
    ui_error( UI_ERROR_ERROR, "Error reading from '%s'", oldname );
    close( SCL ); free( TRDh );
    return 1;
  }

  for( x = 0; x < blocks; x++ ) {
    if( read( SCL, &(headers[x][0]), 14 ) != 14 ) {
      ui_error( UI_ERROR_ERROR, "Error reading from '%s'", oldname );
      close( SCL ); free( TRDh );
      return 1;
    }
  }

  for( x = 0; x < blocks; x++ ) {
    size = headers[x][13];
    if( lsb2dw(trd_free) < size ) {
      ui_error( UI_ERROR_ERROR,
                "File is too long to fit in the image *trd_free=%u < size=%u",
                lsb2dw( trd_free ), size );
      goto Finish;
    }

    if( *trd_files > 127 ) {
      ui_error( UI_ERROR_ERROR, "Image is full" );
      goto Finish;
    }

    memcpy( TRDh + *trd_files * 16, headers[x], 14 );

    memcpy( TRDh + *trd_files * 16 + 0x0E, trd_fsec, 2 );

    tmpscl = malloc( 32000 );
    if( !tmpscl ) {
      ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
      goto Finish;
    }

    left = ( headers[x][13] ) * (libspectrum_dword)256;
    fptr = (*trd_ftrk) * (libspectrum_dword)4096 +
           (*trd_fsec) * (libspectrum_dword)256;
    
    if( lseek( TRD, fptr, SEEK_SET ) == -1 ) {
      ui_error( UI_ERROR_ERROR, "Error seeking in temporary TRD file: %s",
		strerror( errno ) );
      goto Finish;
    }

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

Finish:
  close( SCL );
  lseek( TRD, 0L, SEEK_SET );
  write( TRD, TRDh, 4096 );
  free( TRDh );

  return 0;
}
