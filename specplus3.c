/* specplus3.c: Spectrum +2A/+3 specific routines
   Copyright (c) 1999-2002 Philip Kendall, Darren Salt

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

#include "ay.h"
#include "display.h"
#include "fuse.h"
#include "joystick.h"
#include "keyboard.h"
#include "machine.h"
#include "printer.h"
#include "snapshot.h"
#include "sound.h"
#include "spec128.h"
#include "specplus3.h"
#include "spectrum.h"
#include "ui/ui.h"
#include "z80/z80.h"

static DWORD specplus3_contend_delay( void );

#ifdef HAVE_765_H
static void specplus3_fdc_reset( void );
static BYTE specplus3_fdc_status( WORD port );
static BYTE specplus3_fdc_read( WORD port );
static void specplus3_fdc_write( WORD port, BYTE data );

void specplus3_fdc_error( int debug, char *format, ... );
#endif			/* #ifdef HAVE_765_H */

static int specplus3_shutdown( void );

spectrum_port_info specplus3_peripherals[] = {
  { 0x0001, 0x0000, spectrum_ula_read, spectrum_ula_write },
  { 0x00e0, 0x0000, joystick_kempston_read, spectrum_port_nowrite },
  { 0xc002, 0xc000, ay_registerport_read, ay_registerport_write },
  { 0xc002, 0x8000, spectrum_port_noread, ay_dataport_write },
  { 0xc002, 0x4000, spectrum_port_noread, spec128_memoryport_write },

#ifdef HAVE_765_H
  { 0xf002, 0x3000, specplus3_fdc_read, specplus3_fdc_write },
  { 0xf002, 0x2000, specplus3_fdc_status, spectrum_port_nowrite },
#endif			/* #ifdef HAVE_765_H */

  { 0xf002, 0x1000, spectrum_port_noread, specplus3_memoryport_write },
  { 0xf002, 0x0000, printer_parallel_read, printer_parallel_write },
  { 0, 0, NULL, NULL } /* End marker. DO NOT REMOVE */
};

#if HAVE_765_H
static FDC_PTR fdc;		/* The FDC */
static specplus3_drive_t drives[ SPECPLUS3_DRIVE_B+1 ]; /* Drives A: and B: */
static FDRV_PTR drive_null;	/* A null drive for drives 2 and 3 of the
				   FDC */
#endif				/* #ifdef HAVE_765_H */

BYTE
specplus3_unattached_port( void )
{
  return 0xff;
}

BYTE specplus3_read_screen_memory(WORD offset)
{
  return RAM[machine_current->ram.current_screen][offset];
}

DWORD specplus3_contend_memory( WORD address )
{
  int bank;

  /* Contention occurs in pages 4 to 7. If we're not in a special
     RAM configuration, the logic is the same as for the 128K machine.
     If we are, just enumerate the cases */
  if( machine_current->ram.special ) {

    switch( machine_current->ram.specialcfg ) {

    case 0: /* Pages 0, 1, 2, 3 */
      return 0;
    case 1: /* Pages 4, 5, 6, 7 */
      return specplus3_contend_delay();
    case 2: /* Pages 4, 5, 6, 3 */
    case 3: /* Pages 4, 7, 6, 3 */
      bank = address / 0x4000;
      switch( bank ) {
      case 0: case 1: case 2:
	return specplus3_contend_delay();
      case 3:
	return 0;
      }
    }

  } else {

    if( ( address >= 0x4000 && address < 0x8000 ) ||
	( address >= 0xc000 && machine_current->ram.current_page >= 4 )
	)
      return specplus3_contend_delay();
  }

  return 0;
}

DWORD
specplus3_contend_port( WORD port GCC_UNUSED )
{
  /* Contention does not occur for the ULA.
     FIXME: Unknown for other ports, so let's assume it doesn't for now */
  return 0;
}

static DWORD specplus3_contend_delay( void )
{
  WORD tstates_through_line;
  
  /* No contention in the upper border */
  if( tstates < machine_current->line_times[ DISPLAY_BORDER_HEIGHT ] )
    return 0;

  /* Or the lower border */
  if( tstates >= machine_current->line_times[ DISPLAY_BORDER_HEIGHT + 
                                              DISPLAY_HEIGHT          ] )
    return 0;

  /* Work out where we are in this line */
  tstates_through_line =
    ( tstates + machine_current->timings.left_border_cycles ) %
    machine_current->timings.cycles_per_line;

  /* No contention if we're in the left border */
  if( tstates_through_line < machine_current->timings.left_border_cycles - 3 ) 
    return 0;

  /* Or the right border or retrace */
  if( tstates_through_line >= machine_current->timings.left_border_cycles +
                              machine_current->timings.screen_cycles - 3 )
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

  machine_set_timings( machine, 3.54690e6, 24, 128, 24, 52, 311, 8865 );

  machine->timex = 0;
  machine->ram.read_memory	     = specplus3_readbyte;
  machine->ram.read_memory_internal  = specplus3_readbyte_internal;
  machine->ram.read_screen	     = specplus3_read_screen_memory;
  machine->ram.write_memory          = specplus3_writebyte;
  machine->ram.write_memory_internal = specplus3_writebyte_internal;
  machine->ram.contend_memory	     = specplus3_contend_memory;
  machine->ram.contend_port	     = specplus3_contend_port;

  error = machine_allocate_roms( machine, 4 );
  if( error ) return error;
  error = machine_read_rom( machine, 0, "plus3-0.rom" );
  if( error ) return error;
  error = machine_read_rom( machine, 1, "plus3-1.rom" );
  if( error ) return error;
  error = machine_read_rom( machine, 2, "plus3-2.rom" );
  if( error ) return error;
  error = machine_read_rom( machine, 3, "plus3-3.rom" );
  if( error ) return error;

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
  machine_current->ram.current_page=0; machine_current->ram.current_rom=0;
  machine_current->ram.current_screen=5;
  machine_current->ram.locked=0;
  machine_current->ram.special=0; machine_current->ram.specialcfg=0;

  z80_reset();
  sound_ay_reset();
  snapshot_flush_slt();

#ifdef HAVE_765_H
  specplus3_fdc_reset();
#endif

  return 0;
}

void
specplus3_memoryport_write( WORD port GCC_UNUSED, BYTE b )
{
  /* Let the parallel printer code know about the strobe bit */
  printer_parallel_strobe_write( b & 0x10 );

  /* Do nothing else if we've locked the RAM configuration */
  if( machine_current->ram.locked ) return;

  /* Store the last byte written in case we need it */
  machine_current->ram.last_byte2=b;

#ifdef HAVE_765_H
  /* If this was called by a machine which has a +3-style disk (ie the +3
     as opposed to the +2A), set the state of both ( 3 = ( 1<<0 ) + ( 1<<1 ) )
     floppy drive motors */
  if( libspectrum_machine_capabilities( machine_current->machine ) &&
      LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_DISK )
    fdc_set_motor( fdc, ( b & 0x08 ) ? 3 : 0 );
#endif			/* #ifdef HAVE_765_H */

  if( b & 0x01) {	/* Check whether we want a special RAM configuration */

    /* If so, select it */
    machine_current->ram.special=1;
    machine_current->ram.specialcfg= ( b & 0x06 ) >> 1;

  } else {

    /* If not, we're selecting the high bit of the current ROM */
    machine_current->ram.special=0;
    machine_current->ram.current_rom=(machine_current->ram.current_rom & 0x01) |
      ( (b & 0x04) >> 1 );

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

static BYTE
specplus3_fdc_status( WORD port GCC_UNUSED )
{
  return fdc_read_ctrl( fdc );
}

static BYTE
specplus3_fdc_read( WORD port GCC_UNUSED )
{
  return fdc_read_data( fdc );
}

static void
specplus3_fdc_write( WORD port GCC_UNUSED, BYTE data )
{
  fdc_write_data( fdc, data );
}

/* FDC UI related functions */

/* Used as lib765's `print an error message' callback */
void
specplus3_fdc_error( int debug, char *format, ... )
{
  va_list ap;

  /* Report only serious errors */
  if( debug != 0 ) return;

  va_start( ap, format );
  ui_verror( UI_ERROR_ERROR, format, ap );
  va_end( ap );
}

int
specplus3_disk_insert( specplus3_drive_number which, const char *filename )
{
  struct stat buf;
  struct flock lock;
  size_t i; int error;

  if( which > SPECPLUS3_DRIVE_B ) {
    ui_error( UI_ERROR_ERROR, "specplus3_disk_insert: unknown drive %d\n",
	      which );
    fuse_abort();
  }

  /* Eject any disk already in the drive */
  if( drives[which].fd != -1 ) specplus3_disk_eject( which );

  /* Open the disk file */
  drives[which].fd = open( filename, O_RDWR );
  if( drives[which].fd == -1 ) {
    ui_error( UI_ERROR_ERROR, "Couldn't open '%s': %s", filename,
	      strerror( errno ) );
    return 1;
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

  /* Exclusively lock the entire file */
  lock.l_type = F_WRLCK;
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
