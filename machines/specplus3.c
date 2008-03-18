/* specplus3.c: Spectrum +2A/+3 specific routines
   Copyright (c) 1999-2007 Philip Kendall, Darren Salt

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <stdio.h>

#ifdef HAVE_765_H
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_LIBDSK_H
#include <libdsk.h>
#endif				/* #ifdef HAVE_LIBDSK_H */

#include <765.h>
#endif				/* #ifdef HAVE_765_H */

#include <libspectrum.h>

#include "ay.h"
#include "compat.h"
#include "fuse.h"
#include "joystick.h"
#include "machine.h"
#include "memory.h"
#include "periph.h"
#include "printer.h"
#include "settings.h"
#include "snapshot.h"
#include "spec128.h"
#include "specplus3.h"
#include "spectrum.h"
#include "ui/ui.h"
#include "ula.h"
#include "if1.h"
#include "utils.h"

static int normal_memory_map( int rom, int page );
static int special_memory_map( int which );
static int select_special_map( int page1, int page2, int page3, int page4 );

#ifdef HAVE_765_H
static libspectrum_byte specplus3_fdc_status( libspectrum_word port,
					      int *attached );
static libspectrum_byte specplus3_fdc_read( libspectrum_word port,
					    int *attached );
static void specplus3_fdc_write( libspectrum_word port,
				 libspectrum_byte data );

void specplus3_fdc_error( int debug, char *format, va_list ap );

/* The template used for naming the temporary files used for making
   a copy of the emulated disk */
static const char *dsk_template = "fuse.dsk.XXXXXX";

/* The filename used for the +3 disk autoload snap */
static const char *disk_autoload_snap = "disk_plus3.szx";

/* Has the FDC been initialised? */
static int fdc_initialised = 0;

#endif			/* #ifdef HAVE_765_H */

static int specplus3_reset( void );

const periph_t specplus3_peripherals[] = {
  { 0x0001, 0x0000, ula_read, ula_write },
  { 0x00e0, 0x0000, joystick_kempston_read, NULL },
  { 0xc002, 0xc000, ay_registerport_read, ay_registerport_write },
  { 0xc002, 0x8000, NULL, ay_dataport_write },
  { 0xc002, 0x4000, NULL, spec128_memoryport_write },

#ifdef HAVE_765_H
  { 0xf002, 0x3000, specplus3_fdc_read, specplus3_fdc_write },
  { 0xf002, 0x2000, specplus3_fdc_status, NULL },
#endif			/* #ifdef HAVE_765_H */

  { 0xf002, 0x1000, NULL, specplus3_memoryport2_write },
  { 0xf002, 0x0000, printer_parallel_read, printer_parallel_write },
};

const size_t specplus3_peripherals_count =
  sizeof( specplus3_peripherals ) / sizeof( periph_t );

#if HAVE_765_H
static FDC_PTR fdc;		/* The FDC */
static specplus3_drive_t drives[ SPECPLUS3_DRIVE_B+1 ]; /* Drives A: and B: */
static FDRV_PTR drive_null;	/* A null drive for drives 2 and 3 of the
				   FDC */
#endif				/* #ifdef HAVE_765_H */

int
specplus3_port_from_ula( libspectrum_word port GCC_UNUSED )
{
  /* No contended ports */
  return 0;
}

int specplus3_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_PLUS3;
  machine->id = "plus3";

  machine->reset = specplus3_reset;

  machine->timex = 0;
  machine->ram.port_from_ula	     = specplus3_port_from_ula;
  machine->ram.contend_delay	     = spectrum_contend_delay_76543210;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_none;

  machine->unattached_port = spectrum_unattached_port_none;

  specplus3_765_init();

  machine->shutdown = specplus3_shutdown;

  machine->memory_map = specplus3_memory_map;

  return 0;

}

void
specplus3_765_init( void )
{
#ifdef HAVE_765_H
  int i;

  if( fdc_initialised ) return;

  /* Register lib765 error callback */
  lib765_register_error_function( specplus3_fdc_error );

  /* Create the FDC */
  fdc = fdc_new();

  /* Populate the drives */
  for( i = SPECPLUS3_DRIVE_A; i <= SPECPLUS3_DRIVE_B; i++ ) {

#ifdef HAVE_LIBDSK_H
    drives[i].drive = fd_newldsk();
#else				/* #ifdef HAVE_LIBDSK_H */
    drives[i].drive = fd_newdsk();
#endif				/* #ifdef HAVE_LIBDSK_H */

    fd_settype( drives[i].drive, FD_30 );	/* FD_30 => 3" drive */
    fd_setheads( drives[i].drive, 1 );
    fd_setcyls( drives[i].drive, 40 );
    fd_setreadonly( drives[i].drive, 0 );
    drives[i].fd = -1;				/* Nothing inserted */

  }

  /* And a null drive to use for the other two drives lib765 supports */
  drive_null = fd_new();
  
  /* And reset the FDC */
  specplus3_fdc_reset();

  fdc_initialised = 1;

#endif				/* #ifdef HAVE_765_H */

}

static int
specplus3_reset( void )
{
  int error;

  error = machine_load_rom( 0, 0, settings_current.rom_plus3_0,
                            settings_default.rom_plus3_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 2, 1, settings_current.rom_plus3_1,
                            settings_default.rom_plus3_1, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 4, 2, settings_current.rom_plus3_2,
                            settings_default.rom_plus3_2, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 6, 3, settings_current.rom_plus3_3,
                            settings_default.rom_plus3_3, 0x4000 );
  if( error ) return error;

  error = specplus3_plus2a_common_reset();
  if( error ) return error;

  error = periph_setup( specplus3_peripherals, specplus3_peripherals_count );
  if( error ) return error;
  periph_setup_kempston( PERIPH_PRESENT_OPTIONAL );
  periph_update();

#ifdef HAVE_765_H

  specplus3_fdc_reset();
  specplus3_menu_items();

#endif				/* #ifdef HAVE_765_H */

  return 0;
}

int
specplus3_plus2a_common_reset( void )
{
  int error;
  size_t i;

  machine_current->ram.current_page=0; machine_current->ram.current_rom=0;
  machine_current->ram.locked=0;
  machine_current->ram.special=0;
  machine_current->ram.last_byte=0;
  machine_current->ram.last_byte2=0;

  memory_current_screen = 5;
  memory_screen_mask = 0xffff;

  /* All memory comes from the home bank */
  for( i = 0; i < 8; i++ )
    memory_map_read[i].bank = memory_map_write[i].bank = MEMORY_BANK_HOME;

  /* RAM pages 4, 5, 6 and 7 contended */
  for( i = 0; i < 8; i++ )
    memory_map_ram[ 2 * i     ].contended =
      memory_map_ram[ 2 * i + 1 ].contended = i & 4;

  /* Mark as present/writeable */
  for( i = 0; i < 16; i++ )
    memory_map_ram[i].writable = 1;

  error = normal_memory_map( 0, 0 ); if( error ) return error;

  return 0;
}

static int
normal_memory_map( int rom, int page )
{
  /* ROM as specified */
  memory_map_home[0] = &memory_map_rom[ 2 * rom     ];
  memory_map_home[1] = &memory_map_rom[ 2 * rom + 1 ];

  /* RAM 5 */
  memory_map_home[2] = &memory_map_ram[10];
  memory_map_home[3] = &memory_map_ram[11];

  /* RAM 2 */
  memory_map_home[4] = &memory_map_ram[ 4];
  memory_map_home[5] = &memory_map_ram[ 5];

  /* RAM as specified */
  memory_map_home[6] = &memory_map_ram[ 2 * page     ];
  memory_map_home[7] = &memory_map_ram[ 2 * page + 1 ];

  return 0;
}

static int
special_memory_map( int which )
{
  switch( which ) {
  case 0: return select_special_map( 0, 1, 2, 3 );
  case 1: return select_special_map( 4, 5, 6, 7 );
  case 2: return select_special_map( 4, 5, 6, 3 );
  case 3: return select_special_map( 4, 7, 6, 3 );

  default:
    ui_error( UI_ERROR_ERROR, "unknown +3 special configuration %d", which );
    return 1;
  }
}

static int
select_special_map( int page1, int page2, int page3, int page4 )
{
  memory_map_home[0] = &memory_map_ram[ 2 * page1     ];
  memory_map_home[1] = &memory_map_ram[ 2 * page1 + 1 ];

  memory_map_home[2] = &memory_map_ram[ 2 * page2     ];
  memory_map_home[3] = &memory_map_ram[ 2 * page2 + 1 ];

  memory_map_home[4] = &memory_map_ram[ 2 * page3     ];
  memory_map_home[5] = &memory_map_ram[ 2 * page3 + 1 ];

  memory_map_home[6] = &memory_map_ram[ 2 * page4     ];
  memory_map_home[7] = &memory_map_ram[ 2 * page4 + 1 ];

  return 0;
}

void
specplus3_memoryport2_write( libspectrum_word port GCC_UNUSED,
			     libspectrum_byte b )
{
  /* Let the parallel printer code know about the strobe bit */
  printer_parallel_strobe_write( b & 0x10 );

#ifdef HAVE_765_H
  /* If this was called by a machine which has a +3-style disk, set
     the state of both floppy drive motors */
  if( machine_current->capabilities &&
      LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_DISK ) {

    fdc_set_motor( fdc, ( b & 0x08 ) ? 3 : 0 );

    ui_statusbar_update( UI_STATUSBAR_ITEM_DISK,
			 b & 0x08 ? UI_STATUSBAR_STATE_ACTIVE :
			            UI_STATUSBAR_STATE_INACTIVE );
  }
#endif				/* #ifdef HAVE_765_H */

  /* Do nothing else if we've locked the RAM configuration */
  if( machine_current->ram.locked ) return;

  /* Store the last byte written in case we need it */
  machine_current->ram.last_byte2 = b;

  machine_current->memory_map();
}

int
specplus3_memory_map( void )
{
  int page, rom, screen;
  size_t i;

  page = machine_current->ram.last_byte & 0x07;
  screen = ( machine_current->ram.last_byte & 0x08 ) ? 7 : 5;
  rom =
    ( ( machine_current->ram.last_byte  & 0x10 ) >> 4 ) |
    ( ( machine_current->ram.last_byte2 & 0x04 ) >> 1 );

  /* If we changed the active screen, mark the entire display file as
     dirty so we redraw it on the next pass */
  if( memory_current_screen != screen ) {
    display_update_critical( 0, 0 );
    display_refresh_main_screen();
    memory_current_screen = screen;
  }

  /* Check whether we want a special RAM configuration */
  if( machine_current->ram.last_byte2 & 0x01 ) {

    /* If so, select it */
    machine_current->ram.special = 1;
    special_memory_map( ( machine_current->ram.last_byte2 & 0x06 ) >> 1 );

  } else {

    /* If not, we're selecting the high bit of the current ROM */
    machine_current->ram.special = 0;
    normal_memory_map( rom, page );

  }

  machine_current->ram.current_page = page;
  machine_current->ram.current_rom  = rom;

  for( i = 0; i < 8; i++ )
    memory_map_read[i] = memory_map_write[i] = *memory_map_home[i];

  memory_romcs_map();

  return 0;
}

#if HAVE_765_H

void
specplus3_fdc_reset( void )
{
  /* Reset the FDC and set up the four drives (of which only drives 0 and
     1 exist on the +3 */
  fdc_reset( fdc );
  fdc_setdrive( fdc, 0, drives[ SPECPLUS3_DRIVE_A ].drive );
  fdc_setdrive( fdc, 1, drives[ SPECPLUS3_DRIVE_B ].drive );
  fdc_setdrive( fdc, 2, drive_null );
  fdc_setdrive( fdc, 3, drive_null );
}

void
specplus3_menu_items( void )
{
  /* We can eject disks only if they are currently present */
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_PLUS3_A_EJECT,
		    drives[ SPECPLUS3_DRIVE_A ].fd != -1 );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_PLUS3_B_EJECT,
		    drives[ SPECPLUS3_DRIVE_B ].fd != -1 );
}

static libspectrum_byte
specplus3_fdc_status( libspectrum_word port GCC_UNUSED, int *attached )
{
  *attached = 1;

  return fdc_read_ctrl( fdc );
}

static libspectrum_byte
specplus3_fdc_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  *attached = 1;

  return fdc_read_data( fdc );
}

static void
specplus3_fdc_write( libspectrum_word port GCC_UNUSED, libspectrum_byte data )
{
  fdc_write_data( fdc, data );
}

/* FDC UI related functions */

/* Used as lib765's `print an error message' callback */
void
specplus3_fdc_error( int debug, char *format, va_list ap )
{
  /* Report only serious errors */
  if( debug != 0 ) return;

  ui_verror( UI_ERROR_ERROR, format, ap );
}

/* How we handle +3 disk files: we would like to keep with Fuse's
   model of having the current tape/disk as a 'virtual' tape/disk in
   memory which is written to (the emulating machine's) disk only when
   explicitly requested by the user. This doesn't mesh particularly
   well with lib765/libdsk's model of having a direct one-to-one
   mapping between the emulated disk and a disk file, so we copy the
   emulated disk to a temporary file when it is opened and give that
   to lib765/libdsk instead.

   The use of the temporary file is further complicated by the fact
   that lib765/libdsk doesn't immediately open the file so we can't
   just do the standard mkstemp/unlink pair, but have to unlink when
   the disk is ejected */

int
specplus3_disk_present( specplus3_drive_number which )
{
  return drives[ which ].fd != -1;
}

int
specplus3_disk_insert( specplus3_drive_number which, const char *filename,
                       int autoload )
{
  char tempfilename[ PATH_MAX ];
  int fd, error;

  if( which > SPECPLUS3_DRIVE_B ) {
    ui_error( UI_ERROR_ERROR, "specplus3_disk_insert: unknown drive %d",
	      which );
    fuse_abort();
  }

  /* Eject any disk already in the drive */
  if( drives[which].fd != -1 ) {
    /* Abort the insert if we want to keep the current disk */
    if( specplus3_disk_eject( which, 0 ) ) return 0;
  }

  /* Make a temporary copy of the disk file */
  error = utils_make_temp_file( &fd, tempfilename, filename, dsk_template );
  if( error ) return error;

  /* And now insert the disk */
  drives[ which ].fd = fd;
  strcpy( drives[ which ].filename, tempfilename );

#ifdef HAVE_LIBDSK_H
  fdl_settype( drives[which].drive, NULL ); /* Autodetect disk format */
  fdl_setfilename( drives[which].drive, tempfilename );
#else				/* #ifdef HAVE_LIBDSK_H */
  fdd_setfilename( drives[which].drive, tempfilename );
#endif				/* #ifdef HAVE_LIBDSK_H */

  /* And set the appropriate `eject' item active */
  ui_menu_activate(
    which == SPECPLUS3_DRIVE_A ? UI_MENU_ITEM_MEDIA_DISK_PLUS3_A_EJECT :
				 UI_MENU_ITEM_MEDIA_DISK_PLUS3_B_EJECT  ,
    1
  );

  if( autoload ) {
    int fd; utils_file snap;

    fd = utils_find_auxiliary_file( disk_autoload_snap, UTILS_AUXILIARY_LIB );
    if( fd == -1 ) {
      ui_error( UI_ERROR_ERROR, "Couldn't find +3 disk autoload snap" );
      return 1;
    }

    error = utils_read_fd( fd, disk_autoload_snap, &snap );
    if( error ) { utils_close_file( &snap ); return error; }

    error = snapshot_read_buffer( snap.buffer, snap.length,
                                  LIBSPECTRUM_ID_SNAPSHOT_SZX );
    if( error ) { utils_close_file( &snap ); return error; }

    if( utils_close_file( &snap ) ) {
      ui_error( UI_ERROR_ERROR, "Couldn't munmap '%s': %s", disk_autoload_snap,
                strerror( errno ) );
      return 1;
    }
  }

  return error;
}

int
specplus3_disk_eject( specplus3_drive_number which, int write )
{
  int error;

  if( which > SPECPLUS3_DRIVE_B ) {
    ui_error( UI_ERROR_ERROR, "specplus3_disk_eject: unknown drive %d",
	      which );
    fuse_abort();
  }

  if( drives[ which ].fd == -1 ) return 0;

#ifdef LIB765_EXPOSES_DIRTY

  if( write ) {

    if( ui_plus3_disk_write( which ) ) return 1;

  } else {
    int dirty = fd_dirty( drives[which].drive );
    if( dirty == FD_D_DIRTY ) {

      ui_confirm_save_t confirm = ui_confirm_save(
        "Disk in +3 drive %c: has been modified.\n"
	"Do you want to save it?",
	which == SPECPLUS3_DRIVE_A ? 'A' : 'B'
      );

      switch( confirm ) {

      case UI_CONFIRM_SAVE_SAVE:
	if( ui_plus3_disk_write( which ) ) return 1;
	break;

      case UI_CONFIRM_SAVE_DONTSAVE: fd_eject( drives[which].drive ); break;
      case UI_CONFIRM_SAVE_CANCEL: return 1;

      }
    } else if( dirty == FD_D_UNAVAILABLE ) {
      if( write && ui_plus3_disk_write( which ) ) return 1;
    }
  }
  
#else			/* #ifdef LIB765_EXPOSES_DIRTY */

  if( write ) {
    if( ui_plus3_disk_write( which ) ) return 1;
  }

#endif			/* #ifdef LIB765_EXPOSES_DIRTY */

  error = close( drives[which].fd );
  if( error == -1 ) {
    ui_error( UI_ERROR_ERROR, "Couldn't close temporary file '%s': %s",
	      drives[ which ].filename, strerror( errno ) );
    return 1;
  }
  drives[which].fd = -1;
  unlink( drives[ which ].filename );

  /* Set the appropriate `eject' item inactive */
  ui_menu_activate(
    which == SPECPLUS3_DRIVE_A ? UI_MENU_ITEM_MEDIA_DISK_PLUS3_A_EJECT :
				 UI_MENU_ITEM_MEDIA_DISK_PLUS3_B_EJECT  ,
    0
  );

  return 0;
}

int
specplus3_disk_write( specplus3_drive_number which, const char *filename )
{
  utils_file file;
  FILE *f;
  int error;
  size_t bytes_written;

  fd_eject( drives[ which ].drive );

  f = fopen( filename, "wb" );
  if( !f ) {
    ui_error( UI_ERROR_ERROR, "couldn't open '%s' for writing: %s", filename,
	      strerror( errno ) );
  }

  error = utils_read_file( drives[ which ].filename, &file );
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

#endif			/* #ifdef HAVE_765_H */

int
specplus3_shutdown( void )
{
#ifdef HAVE_765_H
  /* Eject any disks, thus causing the temporary files to be removed */
  specplus3_disk_eject( SPECPLUS3_DRIVE_A, 0 );
  specplus3_disk_eject( SPECPLUS3_DRIVE_B, 0 );

  fd_destroy( &drives[ SPECPLUS3_DRIVE_A ].drive );
  fd_destroy( &drives[ SPECPLUS3_DRIVE_B ].drive );
  fd_destroy( &drive_null );

  fdc_destroy( &fdc );
#endif			/* #ifdef HAVE_765_H */

  return 0;
}
