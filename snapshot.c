/* snapshot.c: snapshot handling routines
   Copyright (c) 1999-2002 Philip Kendall

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include <libspectrum.h>

#include "display.h"
#include "fuse.h"
#include "machine.h"
#include "scld.h"
#include "sound.h"
#include "snapshot.h"
#include "spec128.h"
#include "specplus2a.h"
#include "specplus3.h"
#include "spectrum.h"
#include "ui/ui.h"
#include "utils.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

int snapshot_read( const char *filename )
{
  unsigned char *buffer; size_t length;
  libspectrum_snap *snap;
  int error;

  error = libspectrum_snap_alloc( &snap ); if( error ) return error;

  error = utils_read_file( filename, &buffer, &length );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  error = libspectrum_snap_read( snap, buffer, length, LIBSPECTRUM_ID_UNKNOWN,
				 filename );
  if( error ) {
    munmap( buffer, length ); libspectrum_snap_free( snap );
    return error;
  }

  if( munmap( buffer, length ) == -1 ) {
    libspectrum_snap_free( snap );
    ui_error( UI_ERROR_ERROR, "Couldn't munmap '%s': %s", filename,
	      strerror( errno ) );
    return 1;
  }

  error = snapshot_copy_from( snap );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  error = libspectrum_snap_free( snap ); if( error ) return error;

  return 0;
}

void snapshot_flush_slt (void)
{
  int i;
  for ( i=0; i<256; i++ ) {
    if( slt[i] ) free( slt[i] );
    slt[i] = NULL;
    slt_length[i] = 0;
  }
  slt_screen = NULL;
}

int
snapshot_read_buffer( const unsigned char *buffer, size_t length,
		      libspectrum_id_t type )
{
  libspectrum_snap *snap; int error;

  error = libspectrum_snap_alloc( &snap ); if( error ) return error;

  error = libspectrum_snap_read( snap, buffer, length, type, NULL );
  if( error ) { libspectrum_snap_free( snap ); return error; }
    
  error = snapshot_copy_from( snap );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  error = libspectrum_snap_free( snap ); if( error ) return error;

  return 0;
}

int snapshot_copy_from( libspectrum_snap *snap )
{
  int i,j; int error;
  int capabilities;

  libspectrum_machine machine = snap->machine;

  if( machine == LIBSPECTRUM_MACHINE_PENT ) {
    ui_error( UI_ERROR_INFO, "%s not supported; trying %s instead",
	      libspectrum_machine_name( snap->machine ),
	      libspectrum_machine_name( LIBSPECTRUM_MACHINE_128 ) );
    machine = LIBSPECTRUM_MACHINE_128;
  }

  error = machine_select( machine );
  if( error ) {

    /* If we failed on a +3 snapshot, try falling back to +2A (in case
       we were compiled without lib765) */
    if( machine == LIBSPECTRUM_MACHINE_PLUS3 ) {
      error = machine_select( LIBSPECTRUM_MACHINE_PLUS2A );
      if( error ) {
	ui_error( UI_ERROR_ERROR,
		  "Loading a %s snapshot, but neither that nor %s available",
		  libspectrum_machine_name( machine ),
		  libspectrum_machine_name( LIBSPECTRUM_MACHINE_PLUS2A )    );
	return 1;
      } else {
	ui_error( UI_ERROR_INFO,
		  "Loading a %s snapshot, but that's not available. "
		  "Using %s instead",
		  libspectrum_machine_name( machine ),
		  libspectrum_machine_name( LIBSPECTRUM_MACHINE_PLUS2A )  );
      }
    } else {			/* Not trying a +3 snapshot */
      ui_error( UI_ERROR_ERROR,
		"Loading a %s snapshot, but that's not available",
		libspectrum_machine_name( machine ) );
    }
  }

  machine_reset();

  capabilities = libspectrum_machine_capabilities( machine_current->machine );

  z80.halted = 0;

  A  = snap->a ; F  = snap->f ;
  A_ = snap->a_; F_ = snap->f_;

  BC  = snap->bc ; DE  = snap->de ; HL  = snap->hl ;
  BC_ = snap->bc_; DE_ = snap->de_; HL_ = snap->hl_;

  IX = snap->ix; IY = snap->iy; I = snap->i; R = snap->r;
  SP = snap->sp; PC = snap->pc;

  IFF1 = snap->iff1; IFF2 = snap->iff2; IM = snap->im;

  spectrum_ula_write( 0x00fe, snap->out_ula );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_AY ) {
    ay_registerport_write( 0xfffd, snap->out_ay_registerport );
    for( i=0; i<16; i++ ) {
      machine_current->ay.registers[i] = snap->ay_registers[i];
      sound_ay_write( i, snap->ay_registers[i], 0 );
    }
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY )
    spec128_memoryport_write( 0x7ffd, snap->out_128_memoryport );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY )
    specplus3_memoryport_write( 0x1ffd, snap->out_plus3_memoryport );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY )
    scld_hsr_write( 0x00fd, snap->out_scld_hsr );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_VIDEO )
    scld_dec_write( 0x00ff, snap->out_scld_dec );

  tstates = snap->tstates;

  for( i=0; i<8; i++ ) {
    if( snap->pages[i] != NULL ) memcpy( RAM[i], snap->pages[i], 0x4000 );
  }

  memcpy( slt_length, snap->slt_length, sizeof(slt_length) );
  for( i=0; i<256; i++ ) {
    if( slt_length[i] ) {

      slt[i] = (BYTE*)malloc( slt_length[i] * sizeof( BYTE ) );
      if( slt[i] == NULL ) {
	for( j=0; j<i; j++ ) {
	  if( slt_length[j] ) { free( slt[j] ); slt_length[j] = 0; }
	  ui_error( UI_ERROR_ERROR, "Out of memory in snapshot_copy_from" );
	  return 1;
	}
      }

      memcpy( slt[i], snap->slt[i], slt_length[i] );
    }
  }

  if( snap->slt_screen ) {

    slt_screen = (BYTE*)malloc( 6912 * sizeof( BYTE ) );
    if( slt_screen == NULL ) {
      for( i=0; i<256; i++ ) {
	if( slt_length[i] ) { free( slt[i] ); slt_length[i] = 0; }
	ui_error( UI_ERROR_ERROR, "Out of memory in snapshot_copy_from" );
	return 1;
      }
    }

    memcpy( slt_screen, snap->slt_screen, 6912 );
    slt_screen_level = snap->slt_screen_level;
  }
	  
  return 0;
}

int snapshot_write( const char *filename )
{
  libspectrum_snap *snap;
  unsigned char *buffer; size_t length;

  int error;

  libspectrum_snap_alloc( &snap );

  error = snapshot_copy_to( snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  length = 0;
  error = libspectrum_z80_write( &buffer, &length, snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  error = libspectrum_snap_free( snap );
  if( error != LIBSPECTRUM_ERROR_NONE ) { free( buffer ); return 1; }

  error = utils_write_file( filename, buffer, length );
  if( error ) { free( buffer ); return error; }

  return 0;

}

int snapshot_copy_to( libspectrum_snap *snap )
{
  int i,j;

  snap->machine = machine_current->machine;

  snap->a  = A ; snap->f  = F ;
  snap->a_ = A_; snap->f_ = F_;

  snap->bc  = BC ; snap->de  = DE ; snap->hl  = HL ;
  snap->bc_ = BC_; snap->de_ = DE_; snap->hl_ = HL_;

  snap->ix = IX; snap->iy = IY; snap->i = I; snap->r = R;
  snap->sp = SP; snap->pc = PC;

  snap->iff1 = IFF1; snap->iff2 = IFF2; snap->im = IM;

  snap->out_ula = spectrum_last_ula;
  
  /* These won't necessarily be valid in some machine configurations, but
     this shouldn't cause anything to go wrong */
  snap->out_128_memoryport = machine_current->ram.last_byte;
  snap->out_ay_registerport = machine_current->ay.current_register;
  for( i=0; i<16; i++ )
    snap->ay_registers[i] = machine_current->ay.registers[i];
  snap->out_plus3_memoryport = machine_current->ram.last_byte2;
  snap->out_scld_hsr = scld_last_hsr; snap->out_scld_dec = scld_last_dec.byte;

  snap->tstates = tstates;

  for( i=0; i<8; i++ ) {
    if( RAM[i] != NULL ) {

      snap->pages[i] =
	(libspectrum_byte*)malloc( 0x4000 * sizeof( libspectrum_byte ) );
      if( snap->pages[i] == NULL ) {
	for( j=0; j<i; j++ )
	  if( snap->pages[j] ) { free(snap->pages[j]); snap->pages[j] = NULL; }
	ui_error( UI_ERROR_ERROR, "Out of memory in snapshot_copy_to" );
	return 1;
      }

      memcpy( snap->pages[i], RAM[i], 0x4000 );
    }
  }

  memcpy( snap->slt_length, slt_length, sizeof(snap->slt_length) );
  for( i=0; i<256; i++ ) {
    if( slt_length[i] ) {

      snap->slt[i] =
	(libspectrum_byte*)malloc( slt_length[i] * sizeof(libspectrum_byte) );
      if( snap->slt[i] == NULL ) {
	for( j=0; j<8; j++ )
	  if( snap->pages[j] ) { free(snap->pages[j]); snap->pages[j] = NULL; }
	for( j=0; j<i; j++ )
	  if( snap->slt[j] ) { free( snap->slt[j] ); snap->slt_length[j] = 0; }
	ui_error( UI_ERROR_ERROR, "Out of memory in snapshot_copy_to" );
	return 1;
      }

      memcpy( snap->slt[i], slt[i], slt_length[i] );
    }
  }

  if( slt_screen ) {
    snap->slt_screen =
      (libspectrum_byte*)malloc( 6912 * sizeof( libspectrum_byte ) );
    if( snap->slt_screen == NULL ) {
      for( i=0; i<8; i++ )
	if( snap->pages[i] ) { free( snap->pages[i] ); snap->pages[i] = NULL; }
      for( i=0; i<256; i++ )
	if( snap->slt[i] ) { free( snap->slt[i] ); snap->slt_length[i] = 0; }
      ui_error( UI_ERROR_ERROR, "Out of memory in snapshot_copy_to" );
      return 1;
    }

    memcpy( snap->slt_screen, slt_screen, 6912 );
    snap->slt_screen_level = slt_screen_level;
  }
    
  return 0;
}
