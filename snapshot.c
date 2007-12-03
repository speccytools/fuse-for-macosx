/* snapshot.c: snapshot handling routines
   Copyright (c) 1999-2004 Philip Kendall

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

#include <libspectrum.h>

#include "ay.h"
#include "disk/beta.h"
#include "disk/plusd.h"
#include "fuse.h"
#include "if2.h"
#include "joystick.h"
#include "machine.h"
#include "memory.h"
#include "module.h"
#include "scld.h"
#include "slt.h"
#include "snapshot.h"
#include "ui/ui.h"
#include "ula.h"
#include "utils.h"
#include "z80/z80.h"
#include "zxatasp.h"
#include "zxcf.h"

int snapshot_read( const char *filename )
{
  utils_file file;
  libspectrum_snap *snap;
  int error;

  error = libspectrum_snap_alloc( &snap ); if( error ) return error;

  error = utils_read_file( filename, &file );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  error = libspectrum_snap_read( snap, file.buffer, file.length,
				 LIBSPECTRUM_ID_UNKNOWN, filename );
  if( error ) {
    utils_close_file( &file ); libspectrum_snap_free( snap );
    return error;
  }

  if( utils_close_file( &file ) ) {
    libspectrum_snap_free( snap );
    return 1;
  }

  error = snapshot_copy_from( snap );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  error = libspectrum_snap_free( snap ); if( error ) return error;

  return 0;
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

static int
try_fallback_machine( libspectrum_machine machine )
{
  int error;

  /* If we failed on a +3 or +3e snapshot, try falling back to +2A (in
     case we were compiled without lib765) */
  if( machine == LIBSPECTRUM_MACHINE_PLUS3  ||
      machine == LIBSPECTRUM_MACHINE_PLUS3E    ) {

    error = machine_select( LIBSPECTRUM_MACHINE_PLUS2A );
    if( error ) {
      ui_error( UI_ERROR_ERROR,
		"Loading a %s snapshot, but neither that nor %s available",
		libspectrum_machine_name( machine ),
		libspectrum_machine_name( LIBSPECTRUM_MACHINE_PLUS2A )    );
      return 1;
    } else {
      ui_error( UI_ERROR_WARNING,
		"Loading a %s snapshot, but that's not available. "
		"Using %s instead",
		libspectrum_machine_name( machine ),
		libspectrum_machine_name( LIBSPECTRUM_MACHINE_PLUS2A )  );
    }

  } else {			/* Not trying a +3 or +3e snapshot */
    ui_error( UI_ERROR_ERROR,
	      "Loading a %s snapshot, but that's not available",
	      libspectrum_machine_name( machine ) );
  }
  
  return 0;
}

int
snapshot_copy_from( libspectrum_snap *snap )
{
  int capabilities, error;
  libspectrum_machine machine;

  module_snapshot_enabled( snap );

  machine = libspectrum_snap_machine( snap );

  if( machine != machine_current->machine ) {
    error = machine_select( machine );
    if( error ) {
      error = try_fallback_machine( machine ); if( error ) return error;
    }
  } else {
    machine_reset( 0 );
  }

  capabilities = machine_current->capabilities;

  module_snapshot_from( snap );

  return 0;
}

int snapshot_write( const char *filename )
{
  libspectrum_id_t type;
  libspectrum_class_t class;
  libspectrum_snap *snap;
  unsigned char *buffer; size_t length;
  int flags;

  int error;

  /* Work out what sort of file we want from the filename; default to
     .szx if we couldn't guess */
  error = libspectrum_identify_file_with_class( &type, &class, filename, NULL,
						0 );
  if( error ) return error;

  if( class != LIBSPECTRUM_CLASS_SNAPSHOT || type == LIBSPECTRUM_ID_UNKNOWN )
    type = LIBSPECTRUM_ID_SNAPSHOT_SZX;

  error = libspectrum_snap_alloc( &snap ); if( error ) return error;

  error = snapshot_copy_to( snap );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  flags = 0;
  length = 0;
  error = libspectrum_snap_write( &buffer, &length, &flags, snap, type,
				  fuse_creator, 0 );
  if( error ) { libspectrum_snap_free( snap ); return error; }

  if( flags & LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS ) {
    ui_error(
      UI_ERROR_WARNING,
      "A large amount of information has been lost in conversion; the snapshot probably won't work"
    );
  } else if( flags & LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS ) {
    ui_error(
      UI_ERROR_WARNING,
      "Some information has been lost in conversion; the snapshot may not work"
    );
  }

  error = libspectrum_snap_free( snap );
  if( error ) { free( buffer ); return 1; }

  error = utils_write_file( filename, buffer, length );
  if( error ) { free( buffer ); return error; }

  free( buffer );

  return 0;

}

int
snapshot_copy_to( libspectrum_snap *snap )
{
  libspectrum_snap_set_machine( snap, machine_current->machine );

  module_snapshot_to( snap );

  return 0;
}
