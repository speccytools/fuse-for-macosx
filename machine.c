/* machine.c: Routines for handling the various machine types
   Copyright (c) 1999-2003 Philip Kendall

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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <libgen.h>
#include <settings.h>
#include <string.h>
#include <sys/stat.h>

#include "event.h"
#include "fuse.h"
#include "machine.h"
#include "pentagon.h"
#include "printer.h"
#include "scld.h"
#include "snapshot.h"
#include "sound.h"
#include "spec16.h"
#include "spec48.h"
#include "spec128.h"
#include "specplus2.h"
#include "specplus2a.h"
#include "specplus3.h"
#include "tc2048.h"
#include "tc2068.h"
#include "tape.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "ui/scaler/scaler.h"
#include "utils.h"
#include "z80/z80.h"

#define PATHNAME_MAX_LENGTH 1024

fuse_machine_info **machine_types = NULL; /* Array of available machines */
int machine_count = 0;

fuse_machine_info *machine_current = NULL; /* The currently selected machine */
static int machine_location;	/* Where is the current machine in
				   machine_types[...]? */

static int machine_add_machine( int (*init_function)(fuse_machine_info *machine) );
static int machine_select_machine( fuse_machine_info *machine );

int machine_init_machines( void )
{
  int error;

  error = machine_add_machine( spec16_init    );
  if (error ) return error;
  error = machine_add_machine( spec48_init    );
  if (error ) return error;
  error = machine_add_machine( spec128_init   );
  if (error ) return error;
  error = machine_add_machine( specplus2_init );
  if (error ) return error;
  error = machine_add_machine( specplus2a_init );
  if( error ) return error;

#ifdef HAVE_765_H
  /* Add the +3 only if we have FDC support; otherwise the +2A and +3
     emulations are identical */
  error = machine_add_machine( specplus3_init );
  if (error ) return error;
#endif				/* #ifdef HAVE_765_H */

  error = machine_add_machine( tc2048_init );
  if (error ) return error;
  error = machine_add_machine( tc2068_init );
  if( error ) return error;
  error = machine_add_machine( pentagon_init );
  if (error ) return error;

  return 0;
}

static int machine_add_machine( int (*init_function)( fuse_machine_info *machine ) )
{
  int error;

  machine_count++;

  machine_types = 
    (fuse_machine_info**)realloc( machine_types, 
			     machine_count * sizeof( fuse_machine_info* ) );
  if( machine_types == NULL ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  machine_types[ machine_count - 1 ] =
    (fuse_machine_info*)malloc( sizeof( fuse_machine_info ) );
  if( machine_types[ machine_count - 1 ] == NULL ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  error = init_function( machine_types[ machine_count - 1 ] );
  if( error ) return error;

  return 0;
}

int machine_select( int type )
{
  int i;
  int error;

  for( i=0; i < machine_count; i++ ) {
    if( machine_types[i]->machine == type ) {
      machine_location = i;
      error = machine_select_machine( machine_types[i] );

      if( !error ) return 0;

      /* If we couldn't select the new machine type, try falling back
	 to plain old 48K */
      if( type != LIBSPECTRUM_MACHINE_48 ) 
	error = machine_select( LIBSPECTRUM_MACHINE_48 );
	
      /* If that still didn't work, give up */
      if( error ) {
	ui_error( UI_ERROR_ERROR, "can't select 48K machine. Giving up." );
	fuse_abort();
      } else {
	ui_error( UI_ERROR_INFO, "selecting 48K machine" );
	return 0;
      }
      
      return 0;
    }
  }

  ui_error( UI_ERROR_ERROR, "machine type %d unknown", type );
  return 1;
}

int machine_select_id( const char *id )
{
  int i, error;

  for( i=0; i < machine_count; i++ ) {
    if( ! strcmp( machine_types[i]->id, id ) ) {
      error = machine_select_machine( machine_types[i] );
      if( error ) return error;
      return 0;
    }
  }

  ui_error( UI_ERROR_ERROR, "Machine id '%s' unknown", id );
  return 1;
}

const char*
machine_get_id( libspectrum_machine type )
{
  int i;

  for( i=0; i<machine_count; i++ )
    if( machine_types[i]->machine == type ) return machine_types[i]->id;

  return NULL;
}

static int
machine_select_machine( fuse_machine_info *machine )
{
  int width, height;
  int capabilities;

  machine_current = machine;

  settings_set_string( &settings_current.start_machine, machine->id );
  
  tstates = 0;

  /* Reset the event stack */
  event_reset();
  if( event_add( machine->timings.tstates_per_frame, EVENT_TYPE_FRAME ) )
    return 1;
  if( event_add( machine->line_times[0], EVENT_TYPE_LINE) ) return 1;

  read_screen_memory = machine->ram.read_screen;
  contend_memory = machine->ram.contend_memory;
  contend_port = machine->ram.contend_port;
  
  if( uidisplay_end() ) return 1;

  capabilities = libspectrum_machine_capabilities( machine->machine );

  /* Set screen sizes here */
  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_VIDEO ) {
    width = DISPLAY_SCREEN_WIDTH;
    height = 2*DISPLAY_SCREEN_HEIGHT;
  } else {
    width = DISPLAY_ASPECT_WIDTH;
    height = DISPLAY_SCREEN_HEIGHT;
  }

  if( uidisplay_init( width, height ) ) return 1;

  if( machine_reset() ) return 1;

  /* Activate appropriate menu items and update the status bar */
  if( ( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_DISK ) ||
      ( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TRDOS_DISK )    ) {
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK, 1 );
    ui_statusbar_update( UI_STATUSBAR_ITEM_DISK, UI_STATUSBAR_STATE_INACTIVE );
  } else {
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK, 0 );
    ui_statusbar_update( UI_STATUSBAR_ITEM_DISK,
			 UI_STATUSBAR_STATE_NOT_AVAILABLE );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_DOCK ) {
    ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE, 1 );
    ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE_EJECT, 0 );
  } else {
    ui_menu_activate( UI_MENU_ITEM_MEDIA_CARTRIDGE, 0 );
  };

  return 0;
}

int
machine_load_rom( libspectrum_byte **data, char *filename,
		  size_t expected_length )
{
  int fd, error;
  utils_file rom;

  fd = utils_find_auxiliary_file( filename, UTILS_AUXILIARY_ROM );
  if( fd == -1 ) {
    ui_error( UI_ERROR_ERROR, "couldn't find ROM '%s'", filename );
    return 1;
  }
  
  error = utils_read_fd( fd, filename, &rom );
  if( error ) return error;
  
  if( rom.length != expected_length ) {
    ui_error( UI_ERROR_ERROR,
	      "ROM '%s' is %ld bytes long; expected %ld bytes",
	      filename, (unsigned long)rom.length,
	      (unsigned long)expected_length );
    return 1;
  }

  /* Take a copy of the ROM in case we want to write to it later */
  *data = malloc( rom.length * sizeof( libspectrum_byte ) );
  if( !(*data) ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  memcpy( *data, rom.buffer, rom.length );

  if( utils_close_file( &rom ) ) return 1;

  return 0;
}

int
machine_reset( void )
{
  size_t i;
  int error;

  /* These things should happen on all resets */
  z80_reset();

  /* sound_ay_reset() *absolutely must* be called before either
     sound_frame() or sound_ay_write() */
  sound_ay_reset();

  snapshot_flush_slt();
  printer_zxp_reset();
  scld_reset();
  tape_stop();

  /* Load in the ROMs: remove any ROMs we've got in memory at the moment */
  for( i = 0; i < spectrum_rom_count; i++ ) {
    if( ROM[i] ) { free( ROM[i] ); ROM[i] = 0; }
  }
    
  /* Make sure we have enough space for the new ROMs */
  if( spectrum_rom_count < machine_current->rom_count ) {

    libspectrum_byte **new_ROM =
      realloc( ROM, machine_current->rom_count * sizeof( libspectrum_byte* ) );
    if( !new_ROM ) {
      ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
      return 1;
    }

    ROM = new_ROM;
  }
  spectrum_rom_count = machine_current->rom_count;

  /* Do the machine-specific bits, including loading the ROMs */
  error = machine_current->reset(); if( error ) return error;

  return 0;
}

int
machine_set_timings( fuse_machine_info *machine )
{
  size_t y;

  /* Pull timings we use repeatedly out of libspectrum and store them
     for ourself */
  machine->timings.processor_speed =
    libspectrum_timings_processor_speed( machine->machine );
  machine->timings.left_border =
    libspectrum_timings_left_border( machine->machine );
  machine->timings.horizontal_screen =
    libspectrum_timings_horizontal_screen( machine->machine );
  machine->timings.right_border =
    libspectrum_timings_right_border( machine->machine );
  machine->timings.tstates_per_line =
    libspectrum_timings_tstates_per_line( machine->machine );
  machine->timings.tstates_per_frame =
    libspectrum_timings_tstates_per_frame( machine->machine );

  /* Magic number alert

     libspectrum_timings_top_left_pixel gives us the number of tstates
     after the interrupt at which the top-left pixel of the screen is
     displayed. However, what's more useful for Fuse is when the first
     pixel of the top line of the border is displayed.

     Fuse is currently hard-wired to show 24 lines of top and bottom border,
     hence we subtract 24 times the line length. We also subtract the
     appropriate number of left border cycles; the difference between this
     and the actually displayed width is accounted for in
     display.c:display_border_column()
  */

  machine->line_times[0]=
    libspectrum_timings_top_left_pixel( machine->machine ) -
    24 * machine->timings.tstates_per_line -
    machine->timings.left_border;

  for( y=1; y<DISPLAY_SCREEN_HEIGHT+1; y++ ) {
    machine->line_times[y] = machine->line_times[y-1] + 
                             machine->timings.tstates_per_line;
  }

  return 0;
}

int machine_allocate_roms( fuse_machine_info *machine, size_t count )
{
  machine->rom_count = count;

  machine->rom_length = malloc( count * sizeof(size_t) );
  if( machine->rom_length == NULL ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    free( machine->rom_length );
    return 1;
  }

  return 0;
}

int machine_end( void )
{
  int i;

  for( i=0; i<machine_count; i++ ) {
    if( machine_types[i]->shutdown ) machine_types[i]->shutdown();
    free( machine_types[i] );
  }

  free( machine_types );

  return 0;
}
