/* snapshot.c: snapshot handling routines
   Copyright (c) 1999-2001 Philip Kendall

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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "display.h"
#include "fuse.h"
#include "libspectrum/libspectrum.h"
#include "machine.h"
#include "snapshot.h"
#include "spec128.h"
#include "spectrum.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

typedef enum snapshot_type {
  SNAPSHOT_TYPE_SNA,
  SNAPSHOT_TYPE_Z80,
} snapshot_type;

static snapshot_type snapshot_identify( const char *filename );

static int snapshot_copy_from( libspectrum_snap *snap );
static int snapshot_copy_to( libspectrum_snap *snap );

#define ERROR_MESSAGE_MAX_LENGTH 1024

int snapshot_read( char *filename )
{
  struct stat file_info; int fd; uchar *buffer;

  libspectrum_snap snap;

  int error; char error_message[ ERROR_MESSAGE_MAX_LENGTH ];

  libspectrum_snap_initalise( &snap );

  fd = open( filename, O_RDONLY );
  if( fd == -1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: couldn't open `%s'", fuse_progname, filename );
    perror( error_message );
    return 1;
  }

  if( fstat( fd, &file_info) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't stat `%s'", fuse_progname, filename );
    perror( error_message );
    close(fd);
    return 1;
  }

  buffer = mmap( 0, file_info.st_size, PROT_READ, MAP_SHARED, fd, 0 );
  if( buffer == (void*)-1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't mmap `%s'", fuse_progname, filename );
    perror( error_message );
    close(fd);
    return 1;
  }

  if( close(fd) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't close `%s'", fuse_progname, filename );
    perror( error_message );
    munmap( buffer, file_info.st_size );
    return 1;
  }

  switch( snapshot_identify( filename ) ) {

  case SNAPSHOT_TYPE_SNA:

    error = libspectrum_sna_read( buffer, file_info.st_size, &snap );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      fprintf(stderr, "%s: Error from libspectrum_sna_read: %s\n",
	      fuse_progname, libspectrum_error_message(error) );
      munmap( buffer, file_info.st_size );
      return 1;
    }
    break;

  case SNAPSHOT_TYPE_Z80:

    error = libspectrum_z80_read( buffer, file_info.st_size, &snap );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      fprintf(stderr, "%s: Error from libspectrum_z80_read: %s\n",
	      fuse_progname, libspectrum_error_message(error) );
      munmap( buffer, file_info.st_size );
      return 1;
    }
    break;

  default:

    fprintf(stderr, "%s: Unknown snapshot type\n", fuse_progname );
    return 1;

  }

  if( munmap( buffer, file_info.st_size ) == -1 ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't munmap `%s'", fuse_progname, filename );
    perror( error_message );
    return 1;
  }

  error = snapshot_copy_from( &snap );
  if( error ) return error;

  error = libspectrum_snap_destroy( &snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    fprintf(stderr, "%s: Error from libspectrum_snap_destroy: %s\n",
	    fuse_progname, libspectrum_error_message(error) );
    return 1;
  }

  return 0;

}

static snapshot_type snapshot_identify( const char *filename )
{
  if(    strlen( filename ) < 4
      || strncmp( &filename[ strlen(filename) - 4 ], ".sna", 4 ) ) {
    return SNAPSHOT_TYPE_Z80;
  } else {
    return SNAPSHOT_TYPE_SNA;
  }
}

static int snapshot_copy_from( libspectrum_snap *snap )
{
  int i; int error;

  switch( snap->machine ) {
  case LIBSPECTRUM_MACHINE_48:
    error = machine_select( SPECTRUM_MACHINE_48 );
    if( error ) {
      fprintf(stderr,"%s: 48K Spectrum unavailable\n", fuse_progname );
      return 1;
    }
    break;
  case LIBSPECTRUM_MACHINE_128:
    error = machine_select( SPECTRUM_MACHINE_128 );
    if( error ) {
      fprintf(stderr,"%s: 128K Spectrum unavailable\n", fuse_progname );
      return 1;
    }
    break;
  default:
    fprintf(stderr,"%s: Unknown machine type %d\n", fuse_progname,
	    snap->machine);
    return 1;
  }
  machine_current->reset();

  z80.halted = 0;

  A  = snap->a ; F  = snap->f ;
  A_ = snap->a_; F_ = snap->f_;

  BC  = snap->bc ; DE  = snap->de ; HL  = snap->hl ;
  BC_ = snap->bc_; DE_ = snap->de_; HL_ = snap->hl_;

  IX = snap->ix; IY = snap->iy; I = snap->i; R = snap->r;
  SP = snap->sp; PC = snap->pc;

  IFF1 = snap->iff1; IFF2 = snap->iff2; IM = snap->im;

  spectrum_ula_write( 0x00fe, snap->out_ula );

  if( machine_current->machine == SPECTRUM_MACHINE_128 ) {
    spec128_memoryport_write( 0x7ffd, snap->out_128_memoryport );
    ay_registerport_write( 0xfffd, snap->out_ay_registerport );
    for( i=0; i<15; i++ )
      machine_current->ay.registers[i] = snap->ay_registers[i];
  }

  tstates = snap->tstates;

  for( i=0; i<8; i++ ) {
    if( snap->pages[i] != NULL ) memcpy( RAM[i], snap->pages[i], 0x4000 );
  }

  return 0;
}

int snapshot_write( char *filename )
{
  libspectrum_snap snap;
  unsigned char *buffer; size_t length;
  FILE *f;

  int error; char error_message[ ERROR_MESSAGE_MAX_LENGTH ];

  libspectrum_snap_initalise( &snap );

  error = snapshot_copy_to( &snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  length = 0;
  error = libspectrum_z80_write( &buffer, &length, &snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    fprintf(stderr, "%s: error libspectrum_z80_write: %s\n", fuse_progname,
	    libspectrum_error_message(error) );
    return error;
  }

  f=fopen( filename, "wb" );
  if(!f) { 
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: error opening `%s'", fuse_progname, filename );
    perror( error_message );
    free( buffer );
    return 1;
  }
	    
  if( fwrite( buffer, 1, length, f ) != length ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: error writing to `%s'", fuse_progname, filename );
    perror( error_message );
    fclose(f);
    free( buffer );
    return 1;
  }

  free( buffer );

  if( fclose( f ) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: error closing `%s'", fuse_progname, filename );
    perror( error_message );
    return 1;
  }

  return 0;

}

static int snapshot_copy_to( libspectrum_snap *snap )
{
  int i;

  switch( machine_current->machine ) {
  case SPECTRUM_MACHINE_48:
    snap->machine = LIBSPECTRUM_MACHINE_48;
    break;
  case SPECTRUM_MACHINE_128:
    snap->machine = LIBSPECTRUM_MACHINE_128;
    break;
  default:
    fprintf(stderr,"%s: Can't handle machine type %d in snapshots\n",
	    fuse_progname, snap->machine);
    return 1;
  }

  snap->a  = A ; snap->f  = F ;
  snap->a_ = A_; snap->f_ = F_;

  snap->bc  = BC ; snap->de  = DE ; snap->hl  = HL ;
  snap->bc_ = BC_; snap->de_ = DE_; snap->hl_ = HL_;

  snap->ix = IX; snap->iy = IY; snap->i = I; snap->r = R;
  snap->sp = SP; snap->pc = PC;

  snap->iff1 = IFF1; snap->iff2 = IFF2; snap->im = IM;

  snap->out_ula = display_border; /* FIXME: need to do this properly */
  
  if( machine_current->machine == SPECTRUM_MACHINE_128 ) {
    snap->out_128_memoryport = machine_current->ram.last_byte;
    snap->out_ay_registerport = machine_current->ay.current_register;
    for( i=0; i<15; i++ )
      snap->ay_registers[i] = machine_current->ay.registers[i];
  }

  snap->tstates = tstates;

  /* FIXME: should copy to new memory */
  for( i=0; i<8; i++ ) snap->pages[i] = RAM[i];

  return 0;
}
