/* rzx.c: .rzx files
   Copyright (c) 2002 Philip Kendall

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

#include "event.h"
#include "fuse.h"
#include "keyboard.h"
#include "machine.h"
#include "rzx.h"
#include "settings.h"
#include "snapshot.h"
#include "types.h"
#include "ui/ui.h"
#include "utils.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

/* The offset used to get the count of instructions from the R register;
   (instruction count) = R + rzx_instructions_offset */
size_t rzx_instructions_offset;

/* The number of bytes read via IN during the current frame */
size_t rzx_in_count;

/* And the values of those bytes */
libspectrum_byte *rzx_in_bytes;

/* How big is the above array? */
size_t rzx_in_allocated;

/* Are we currently recording a .rzx file? */
int rzx_recording;

/* The filename we'll save this recording into */
static char *rzx_filename;

/* The .z80 format snapshot taken when we started the recording */
static libspectrum_byte *rzx_snap;
static size_t rzx_snap_length;

/* Are we currently playing back a .rzx file? */
int rzx_playback;

/* The .rzx frame we're currently playing */
size_t rzx_current_frame;

/* And the RZX frame we're getting IN data from */
size_t rzx_data_frame;

/* The current RZX data */
libspectrum_rzx *rzx;

/* The information to be used as the creator in the RZX file */
static const char *rzx_creator = "Fuse";
static libspectrum_word rzx_major_version, rzx_minor_version;

static int recording_frame( void );
static int playback_frame( void );
static int counter_reset( void );

int rzx_init( void )
{
  unsigned version[4] = { 0, 0, 0, 0 };
  int i;
  libspectrum_error error;

  error = libspectrum_rzx_alloc( &rzx );
  if( error ) return 1;

  rzx_recording = rzx_playback = 0;

  rzx_in_bytes = NULL;
  rzx_in_allocated = 0;
  rzx_snap = 0;

  sscanf( VERSION, "%u.%u.%u.%u",
	  &version[0], &version[1], &version[2], &version[3] );

  for( i=0; i<4; i++ ) if( version[i] > 0xff ) version[i] = 0xff;

  rzx_major_version = version[0] * 0x100 + version[1];
  rzx_minor_version = version[2] * 0x100 + version[3];

  return 0;
}

int rzx_start_recording( const char *filename, int embed_snapshot )
{
  if( rzx_playback ) return 1;

  /* Note that we're recording */
  rzx_recording = 1;

  if( rzx_snap ) free( rzx_snap ); rzx_snap = 0;

  /* If we're embedding a snapshot, create it now */
  if( embed_snapshot ) {
    
    libspectrum_snap *snap;
    int error;
    libspectrum_error libspec_error;

    libspec_error = libspectrum_snap_alloc( &snap );
    if( libspec_error != LIBSPECTRUM_ERROR_NONE ) return 1;

    error = snapshot_copy_to( snap ); if( error ) return 1;

    libspec_error = libspectrum_z80_write( &rzx_snap, &rzx_snap_length, snap );
    if( libspec_error != LIBSPECTRUM_ERROR_NONE ) {
      ui_error( UI_ERROR_ERROR, "Error creating RZX embedded snapshot: %s",
		libspectrum_error_message( libspec_error ) );
      libspectrum_snap_free( snap ); rzx_snap = 0;
      return 1;
    }
    
    libspec_error = libspectrum_snap_free( snap );
    if( libspec_error != LIBSPECTRUM_ERROR_NONE ) {
      ui_error( UI_ERROR_ERROR, "Error from libspectrum_snap_destroy: %s",
		libspectrum_error_message( libspec_error ) );
      free( rzx_snap ); rzx_snap = 0;
      return 1;
    }
  }

  /* Start the count of instruction fetches here */
  counter_reset();
  rzx->tstates = tstates;

  /* Store the filename */
  rzx_filename = strdup( filename );
  if( rzx_filename == NULL ) {
    ui_error( UI_ERROR_ERROR, "out of memory in rzx_start_recording" );
    return 1;
  }

  return 0;
}

int rzx_stop_recording( void )
{
  libspectrum_byte *buffer; size_t length;
  libspectrum_error libspec_error; int error;

  /* Stop recording data */
  rzx_recording = 0;

  length = 0;
  libspec_error = libspectrum_rzx_write( rzx, &buffer, &length,
					 rzx_snap, rzx_snap_length,
					 rzx_creator, rzx_major_version,
					 rzx_minor_version,
					 settings_current.rzx_compression );
  if( libspec_error != LIBSPECTRUM_ERROR_NONE ) {
    ui_error( UI_ERROR_ERROR, "error during libspectrum_rzx_write: %s",
	      libspectrum_error_message( libspec_error ) );
    libspectrum_rzx_free( rzx );
    return libspec_error;
  }

  error = utils_write_file( rzx_filename, buffer, length );
  free( rzx_filename );
  if( error ) { free( buffer ); libspectrum_rzx_free( rzx ); return error; }

  free( buffer );

  libspec_error = libspectrum_rzx_free( rzx );
  if( libspec_error != LIBSPECTRUM_ERROR_NONE ) {
    ui_error( UI_ERROR_ERROR, "error during libspectrum_rzx_free: %s",
	      libspectrum_error_message( libspec_error ) );
    return libspec_error;
  }

  return 0;
}

int rzx_start_playback( const char *filename, int (*load_snap)(void) )
{
  libspectrum_byte *buffer; size_t length;
  libspectrum_error libspec_error; int error;
  libspectrum_snap *snap = 0;

  if( rzx_recording) return 1;

  error = utils_read_file( filename, &buffer, &length );
  if( error ) return error;

  libspec_error = libspectrum_rzx_read( rzx, buffer, length, &snap );
  if( libspec_error != LIBSPECTRUM_ERROR_NONE ) {
    ui_error( UI_ERROR_ERROR, "error during libspectrum_rzx_read: %s",
	      libspectrum_error_message( libspec_error ) );
    munmap( buffer, length );
    return libspec_error;
  }

  if( munmap( buffer, length ) == -1 ) {
    ui_error( UI_ERROR_ERROR, "Couldn't munmap '%s': %s", filename,
	      strerror( errno ) );
    libspectrum_rzx_free( rzx );
    if( snap ) libspectrum_snap_free( snap );
    return 1;
  }

  if( snap ) {			/* The RZX file contained a snapshot */
    error = snapshot_copy_from( snap );
    if( error ) {
      libspectrum_rzx_free( rzx );
      libspectrum_snap_free( snap );
      return error;
    }

    error = libspectrum_snap_free( snap );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      ui_error( UI_ERROR_ERROR, "Error from libspectrum_snap_destroy: %s",
		libspectrum_error_message( error ) );
      free( snap );
      return 1;
    }
  } else {			/* The RZX file had no snapshot */

    if( load_snap ) {
      error = load_snap();
      if( error ) {
	ui_error( UI_ERROR_ERROR, "Error reading snapshot: %s",
		  libspectrum_error_message( error ) );
	libspectrum_rzx_free( rzx );
	return 1;
      }
    }
  }

  /* Interrupts will now be generated by the RZX code */
  error = event_remove_type( EVENT_TYPE_INTERRUPT );
  if( error ) { libspectrum_rzx_free( rzx ); return error; }

  /* We're now playing this RZX file */
  tstates = rzx->tstates;
  rzx_playback = 1;
  rzx_current_frame = rzx_data_frame = 0;
  counter_reset();

  return 0;
}

int rzx_stop_playback( int add_interrupt )
{
  libspectrum_error libspec_error; int error;

  rzx_playback = 0;

  /* We've now finished with the RZX file, so add an interrupt event
     back in if we've been requested to do so; we don't if we just run
     out of frames, as this occurs just before a normal interrupt will
     do this for us */
  if( add_interrupt ) {
    error = event_add( machine_current->timings.cycles_per_frame,
		       EVENT_TYPE_INTERRUPT );
    if( error ) return error;
  }

  libspec_error = libspectrum_rzx_free( rzx );
  if( libspec_error != LIBSPECTRUM_ERROR_NONE ) {
    ui_error( UI_ERROR_ERROR, "libspec_error during libspectrum_rzx_free: %s",
	      libspectrum_error_message( libspec_error ) );
    return libspec_error;
  }

  return 0;
}  

int rzx_frame( void )
{
  if( rzx_recording ) return recording_frame();
  if( rzx_playback  ) return playback_frame();
  return 0;
}

static int recording_frame( void )
{
  libspectrum_error error;

  error = libspectrum_rzx_frame( rzx, R + rzx_instructions_offset,
				 rzx_in_count, rzx_in_bytes );
  if( error ) return error;

  /* Reset the instruction counter */
  counter_reset();

  return 0;
}

static int playback_frame( void )
{
  /* Check we read the correct number of INs during this frame */
  if( rzx_in_count != rzx->frames[ rzx_data_frame ].count ) {
    ui_error( UI_ERROR_ERROR,
	      "Not enough INs during frame %d: expected %d, got %d",
	      rzx_current_frame, rzx->frames[ rzx_data_frame ].count,
	      rzx_in_count );
    return rzx_stop_playback( 0 );
  }

  /* Increment the frame count and see if we've finished with this file */
  if( ++rzx_current_frame >= rzx->count ) {
    ui_error( UI_ERROR_INFO, "Finished RZX playback" );
    return rzx_stop_playback( 0 );
  }

  /* Move the data frame pointer along, unless we're supposed to be
     repeating the last frame */
  if( !rzx->frames[ rzx_current_frame ].repeat_last )
    rzx_data_frame = rzx_current_frame;

  /* If we've got more frame to do, just reset the count and continue */
  counter_reset();

  return 0;
}

/* Reset the RZX counters; also, take this opportunity to normalise the
   R register */
static int counter_reset( void )
{
  R &= 0x7f;		/* Clear all but the 7 lowest bits of the R register */
  rzx_instructions_offset = -R; /* Gives us a zero count */

  rzx_in_count = 0;

  return 0;
}

int rzx_store_byte( libspectrum_byte value )
{
  /* Get more space if we need it; allocate twice as much as we currently
     have, with a minimum of 50 */
  if( rzx_in_count == rzx_in_allocated ) {

    libspectrum_byte *ptr; size_t new_allocated;

    new_allocated = rzx_in_allocated >= 25 ? 2 * rzx_in_allocated : 50;
    ptr =
      (libspectrum_byte*)realloc(
        rzx_in_bytes, new_allocated * sizeof(libspectrum_byte)
      );
    if( ptr == NULL ) {
      ui_error( UI_ERROR_ERROR, "Out of memory in rzx_store_byte\n" );
      return 1;
    }

    rzx_in_bytes = ptr;
    rzx_in_allocated = new_allocated;
  }

  rzx_in_bytes[ rzx_in_count++ ] = value;

  return 0;
}

int rzx_end( void )
{
  if( rzx_recording ) return rzx_stop_recording();
  if( rzx_playback  ) return rzx_stop_playback( 0 );
  return 0;
}
