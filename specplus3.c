/* specplus3.c: Spectrum +2A/+3 specific routines
   Copyright (c) 1999-2003 Philip Kendall, Darren Salt

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

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <stdio.h>

#ifdef HAVE_765_H
#include <errno.h>
#include <fcntl.h>
#include <limits.h>		/* Needed to get PATH_MAX */
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
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
#include "printer.h"
#include "settings.h"
#include "specplus3.h"
#include "spectrum.h"
#include "ui/ui.h"

static libspectrum_dword specplus3_contend_delay( void );

static int normal_memory_map( int rom, int page );
static int special_memory_map( int which );
static int select_special_map( int page1, int page2, int page3, int page4 );

#ifdef HAVE_765_H
static void specplus3_fdc_reset( void );
static libspectrum_byte specplus3_fdc_status( libspectrum_word port );
static libspectrum_byte specplus3_fdc_read( libspectrum_word port );
static void specplus3_fdc_write( libspectrum_word port,
				 libspectrum_byte data );

void specplus3_fdc_error( int debug, char *format, va_list ap );
#endif			/* #ifdef HAVE_765_H */

static int specplus3_shutdown( void );

spectrum_port_info specplus3_peripherals[] = {
  { 0x0001, 0x0000, spectrum_ula_read, spectrum_ula_write },
  { 0x00e0, 0x0000, joystick_kempston_read, spectrum_port_nowrite },
  { 0xc002, 0xc000, ay_registerport_read, ay_registerport_write },
  { 0xc002, 0x8000, spectrum_port_noread, ay_dataport_write },
  { 0xc002, 0x4000, spectrum_port_noread, specplus3_memoryport_write },

#ifdef HAVE_765_H
  { 0xf002, 0x3000, specplus3_fdc_read, specplus3_fdc_write },
  { 0xf002, 0x2000, specplus3_fdc_status, spectrum_port_nowrite },
#endif			/* #ifdef HAVE_765_H */

  { 0xf002, 0x1000, spectrum_port_noread, specplus3_memoryport2_write },
  { 0xf002, 0x0000, printer_parallel_read, printer_parallel_write },
  { 0, 0, NULL, NULL } /* End marker. DO NOT REMOVE */
};

#if HAVE_765_H
static FDC_PTR fdc;		/* The FDC */
static specplus3_drive_t drives[ SPECPLUS3_DRIVE_B+1 ]; /* Drives A: and B: */
static FDRV_PTR drive_null;	/* A null drive for drives 2 and 3 of the
				   FDC */
#endif				/* #ifdef HAVE_765_H */

libspectrum_byte
specplus3_unattached_port( void )
{
  return 0xff;
}

libspectrum_byte specplus3_read_screen_memory( libspectrum_word offset )
{
  return RAM[machine_current->ram.current_screen][offset];
}

libspectrum_dword specplus3_contend_memory( libspectrum_word address )
{
  if( memory_contended[ address >> 13 ] ) return specplus3_contend_delay();

  return 0;
}

libspectrum_dword
specplus3_contend_port( libspectrum_word port GCC_UNUSED )
{
  /* Contention does not occur for the ULA.
     FIXME: Unknown for other ports, so let's assume it doesn't for now */
  return 0;
}

static libspectrum_dword specplus3_contend_delay( void )
{
  libspectrum_word tstates_through_line;
  
  /* No contention in the upper border */
  if( tstates < machine_current->line_times[ DISPLAY_BORDER_HEIGHT ] )
    return 0;

  /* Or the lower border */
  if( tstates >= machine_current->line_times[ DISPLAY_BORDER_HEIGHT + 
                                              DISPLAY_HEIGHT          ] )
    return 0;

  /* Work out where we are in this line */
  tstates_through_line =
    ( tstates + machine_current->timings.left_border ) %
    machine_current->timings.tstates_per_line;

  /* No contention if we're in the left border */
  if( tstates_through_line < machine_current->timings.left_border - 3 ) 
    return 0;

  /* Or the right border or retrace */
  if( tstates_through_line >= machine_current->timings.left_border +
                              machine_current->timings.horizontal_screen - 3 )
    return 0;

  /* We now know the ULA is reading the screen, so put in the appropriate
     delay */
  switch( tstates_through_line % 8 ) {
    case 5: return 1; break;
    case 6: return 0; break;
    case 7: return 7; break;
    case 0: return 6; break;
    case 1: return 5; break;
    case 2: return 4; break;
    case 3: return 3; break;
    case 4: return 2; break;
  }

  return 0;	/* Shut gcc up */
}

int specplus3_init( fuse_machine_info *machine )
{
  int error;
#ifdef HAVE_765_H
  int i;
#endif			/* #ifdef HAVE_765_H */

  machine->machine = LIBSPECTRUM_MACHINE_PLUS3;
  machine->id = "plus3";

  machine->reset = specplus3_reset;

  error = machine_set_timings( machine ); if( error ) return error;

  machine->timex = 0;
  machine->ram.read_screen	     = specplus3_read_screen_memory;
  machine->ram.contend_memory	     = specplus3_contend_memory;
  machine->ram.contend_port	     = specplus3_contend_port;

  error = machine_allocate_roms( machine, 4 );
  if( error ) return error;
  machine->rom_length[0] = machine->rom_length[1] = 
    machine->rom_length[2] = machine->rom_length[3] = 0x4000;

  machine->peripherals=specplus3_peripherals;
  machine->unattached_port = specplus3_unattached_port;

  machine->ay.present=1;

#ifdef HAVE_765_H

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
#endif				/* #ifdef HAVE_765_H */

  machine->shutdown = specplus3_shutdown;

  return 0;

}

int specplus3_reset(void)
{
  int error;

  error = machine_load_rom( &ROM[0], settings_current.rom_plus3_0,
			    machine_current->rom_length[0] );
  if( error ) return error;
  error = machine_load_rom( &ROM[1], settings_current.rom_plus3_1,
			    machine_current->rom_length[1] );
  if( error ) return error;
  error = machine_load_rom( &ROM[2], settings_current.rom_plus3_2,
			    machine_current->rom_length[2] );
  if( error ) return error;
  error = machine_load_rom( &ROM[3], settings_current.rom_plus3_3,
			    machine_current->rom_length[3] );
  if( error ) return error;

#ifdef HAVE_765_H

  specplus3_fdc_reset();

  /* We can eject disks only if they are currently present */
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_A_EJECT,
		    drives[ SPECPLUS3_DRIVE_A ].fd != -1 );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_B_EJECT,
		    drives[ SPECPLUS3_DRIVE_B ].fd != -1 );

#endif				/* #ifdef HAVE_765_H */

  return specplus3_plus2a_common_reset();
}

int
specplus3_plus2a_common_reset()
{
  int error;
  size_t i;

  machine_current->ram.current_page=0; machine_current->ram.current_rom=0;
  machine_current->ram.current_screen=5;
  machine_current->ram.locked=0;
  machine_current->ram.special=0;

  error = normal_memory_map( 0, 0 ); if( error ) return error;
  for( i = 2; i < 8; i++ ) memory_writable[i] = 1;

  memory_screen_chunk1 = RAM[5];
  memory_screen_chunk2 = NULL;
  memory_screen_top = 0x1b00;

  return 0;
}

static int
normal_memory_map( int rom, int page )
{
  memory_map[0] = &ROM[  rom ][0x0000];
  memory_map[1] = &ROM[  rom ][0x2000];
  memory_map[2] = &RAM[    5 ][0x0000];
  memory_map[3] = &RAM[    5 ][0x2000];
  memory_map[4] = &RAM[    2 ][0x0000];
  memory_map[5] = &RAM[    2 ][0x2000];
  memory_map[6] = &RAM[ page ][0x0000];
  memory_map[7] = &RAM[ page ][0x2000];

  memory_writable[0] = memory_writable[1] = 0;

  memory_contended[0] = memory_contended[1] = 0;
  memory_contended[2] = memory_contended[3] = 1;
  memory_contended[4] = memory_contended[5] = 0;
  /* Pages 4, 5, 6 and 7 contended */
  memory_contended[6] = memory_contended[7] = page & 0x04;

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
  memory_map[0] = &RAM[ page1 ][0x0000];
  memory_map[1] = &RAM[ page1 ][0x0000];
  memory_map[2] = &RAM[ page2 ][0x0000];
  memory_map[3] = &RAM[ page2 ][0x0000];
  memory_map[4] = &RAM[ page3 ][0x0000];
  memory_map[5] = &RAM[ page3 ][0x0000];
  memory_map[6] = &RAM[ page4 ][0x0000];
  memory_map[7] = &RAM[ page4 ][0x0000];

  memory_writable[0] = memory_writable[1] = 1;

  /* Pages 4, 5, 6 and 7 contended */
  memory_contended[0] = memory_contended[1] = page1 & 0x04;
  memory_contended[2] = memory_contended[3] = page2 & 0x04;
  memory_contended[4] = memory_contended[5] = page3 & 0x04;
  memory_contended[5] = memory_contended[7] = page4 & 0x04;

  return 0;
}

void
specplus3_memoryport_write( libspectrum_word port GCC_UNUSED,
			    libspectrum_byte b )
{
  int page, rom, screen;

  if( machine_current->ram.locked ) return;

  page = b & 0x07;
  screen = ( b & 0x08 ) ? 7 : 5;
  rom = ( machine_current->ram.current_rom & 0x02 ) | ( ( b & 0x10 ) >> 4 );

  /* Change the memory map unless we're in a special RAM configuration */

  if( !machine_current->ram.special ) {

    memory_map[0] = &ROM[ rom ][0x0000];
    memory_map[1] = &ROM[ rom ][0x2000];

    memory_map[6] = &RAM[ page ][0x0000];
    memory_map[7] = &RAM[ page ][0x2000];

    /* Pages 4, 5, 6 and 7 are contended */
    memory_contended[6] = memory_contended[7] = page & 0x04;
  }

  memory_screen_chunk1 = RAM[ screen ];

  /* If we changed the active screen, mark the entire display file as
     dirty so we redraw it on the next pass */
  if( screen != machine_current->ram.current_screen )
    display_refresh_all();

  machine_current->ram.current_page = page;
  machine_current->ram.current_screen = screen;
  machine_current->ram.current_rom = rom;
  machine_current->ram.locked = ( b & 0x20 );

  machine_current->ram.last_byte = b;
}

void
specplus3_memoryport2_write( libspectrum_word port GCC_UNUSED,
			     libspectrum_byte b )
{
  /* Let the parallel printer code know about the strobe bit */
  printer_parallel_strobe_write( b & 0x10 );

#ifdef HAVE_765_H
  /* If this was called by a machine which has a +3-style disk (ie the +3
     as opposed to the +2A), set the state of both floppy drive motors */
  if( libspectrum_machine_capabilities( machine_current->machine ) &&
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

  if( b & 0x01 ) {	/* Check whether we want a special RAM configuration */

    /* If so, select it */
    machine_current->ram.special = 1;
    special_memory_map( ( b & 0x06 ) >> 1 );

  } else {

    /* If not, we're selecting the high bit of the current ROM */
    machine_current->ram.special = 0;
    machine_current->ram.current_rom = 
      ( machine_current->ram.current_rom & 0x01 ) | ( ( b & 0x04 ) >> 1 );

    normal_memory_map( machine_current->ram.current_rom,
		       machine_current->ram.current_page );
  }

}

#if HAVE_765_H

static void
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

static libspectrum_byte
specplus3_fdc_status( libspectrum_word port GCC_UNUSED )
{
  return fdc_read_ctrl( fdc );
}

static libspectrum_byte
specplus3_fdc_read( libspectrum_word port GCC_UNUSED )
{
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

int
specplus3_disk_insert( specplus3_drive_number which, const char *filename )
{
  struct stat buf;
  struct flock lock;
  int readonly;
  size_t i; int error;

  if( which > SPECPLUS3_DRIVE_B ) {
    ui_error( UI_ERROR_ERROR, "specplus3_disk_insert: unknown drive %d\n",
	      which );
    fuse_abort();
  }

  /* Eject any disk already in the drive */
  if( drives[which].fd != -1 ) specplus3_disk_eject( which );

  readonly = 0;
  /* Open the disk file */
  drives[which].fd = open( filename, O_RDWR | O_BINARY );
  if( drives[which].fd == -1 ) {

    /* If we couldn't open read-write, try read-only */
    if( errno == EACCES ) {
      readonly = 1;
      drives[which].fd = open( filename, O_RDONLY | O_BINARY );
    }

    /* If we got an error other than EACCES or the read-only open failed,
       give up */
    if( drives[which].fd == -1 ) {
      ui_error( UI_ERROR_ERROR, "Couldn't open '%s': %s", filename,
		strerror( errno ) );
      return 1;
    }
  }

  /* We now have to do two sorts of locking:

     1) stop the same disk being put into more than one drive on this
     copy of Fuse. Do this by looking at the st_dev (device) and
     st_ino (inode) results from fstat(2) and assuming that each file
     returns a persistent unique pair. This assumption isn't quite
     true (I can break it with smbfs, and nmm1@cam.ac.uk has pointed
     out some other cases), but it's right most of the time, and it's
     POSIX compliant.

     2) stop the same disk being accessed by other programs. Without
     mandatory locking (ugh), can't enforce this, so just lock the
     entire file with fcntl. Therefore two copies of Fuse (or anything
     else using fcntl) can't access the file whilst it's in a
     drive. Again, this is POSIX compliant.
  */

  error = fstat( drives[which].fd, &buf );
  if( error == -1 ) {
    ui_error( UI_ERROR_ERROR, "Couldn't fstat '%s': %s", filename,
	      strerror( errno ) );
    close( drives[which].fd ); drives[which].fd = -1;
    return 1;
  }

  drives[which].device = buf.st_dev;
  drives[which].inode  = buf.st_ino;

  for( i = SPECPLUS3_DRIVE_A; i <= SPECPLUS3_DRIVE_B; i++ ) {

    /* Don't compare this drive with itself */
    if( i == which ) continue;

    /* If there's no file in this drive, it can't clash */
    if( drives[i].fd == -1 ) continue;

    if( drives[i].device == drives[which].device &&
	drives[i].inode  == drives[which].inode ) {
      ui_error( UI_ERROR_ERROR, "'%s' is already in drive %c:", filename,
		(char)( 'A' + i ) );
      close( drives[which].fd ); drives[which].fd = -1;
      return 1;
    }
  }

  /* Lock the entire file: exclusively if we opened it read/write, or
     read lock if we opened it readonly */
  lock.l_type = readonly ? F_RDLCK : F_WRLCK;
  lock.l_start = 0;
  lock.l_whence = SEEK_SET;
  lock.l_len = 0;		/* Entire file */

  error = fcntl( drives[which].fd, F_SETLK, &lock );
  if( error == -1 ) {
    ui_error( UI_ERROR_ERROR, "Couldn't lock '%s': %s", filename,
	      strerror( errno ) );
    close( drives[which].fd ); drives[which].fd = -1;
    return 1;
  }

  /* And now insert the disk */
#ifdef HAVE_LIBDSK_H
  fdl_settype( drives[which].drive, NULL ); /* Autodetect disk format */
  fdl_setfilename( drives[which].drive, filename );
#else				/* #ifdef HAVE_LIBDSK_H */
  fdd_setfilename( drives[which].drive, filename );
#endif				/* #ifdef HAVE_LIBDSK_H */

  /* And set the appropriate `eject' item active */
  ui_menu_activate(
    which == SPECPLUS3_DRIVE_A ? UI_MENU_ITEM_MEDIA_DISK_A_EJECT :
				 UI_MENU_ITEM_MEDIA_DISK_B_EJECT  ,
    1
  );

  return 0;
}

int
specplus3_disk_eject( specplus3_drive_number which )
{
  int error;

  if( which > SPECPLUS3_DRIVE_B ) {
    ui_error( UI_ERROR_ERROR, "specplus3_disk_eject: unknown drive %d\n",
	      which );
    fuse_abort();
  }

  /* NB: the fclose() called here will cause the lock on the file to
     be released */
  fd_eject( drives[which].drive );

  if( drives[which].fd != -1 ) {
    error = close( drives[which].fd );
    if( error == -1 ) {
      ui_error( UI_ERROR_ERROR, "Couldn't close the disk: %s\n",
		strerror( errno ) );
      return 1;
    }
    drives[which].fd = -1;
  }

  /* Set the appropriate `eject' item inactive */
  /* And set the appropriate `eject' item active */
  ui_menu_activate(
    which == SPECPLUS3_DRIVE_A ? UI_MENU_ITEM_MEDIA_DISK_A_EJECT :
				 UI_MENU_ITEM_MEDIA_DISK_B_EJECT  ,
    0
  );

  return 0;
}

#endif			/* #ifdef HAVE_765_H */

static int
specplus3_shutdown( void )
{
#ifdef HAVE_765_H
  fd_destroy( &drives[ SPECPLUS3_DRIVE_A ].drive );
  fd_destroy( &drives[ SPECPLUS3_DRIVE_B ].drive );
  fd_destroy( &drive_null );

  fdc_destroy( &fdc );
#endif			/* #ifdef HAVE_765_H */

  return 0;
}
