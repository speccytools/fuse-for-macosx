/* machine.c: Routines for handling the various machine types
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

#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "event.h"
#include "fuse.h"
#include "machine.h"
#include "spec48.h"
#include "spec128.h"
#include "specplus2.h"
#include "specplus3.h"
#include "tape.h"
#include "z80/z80.h"

#define ERROR_MESSAGE_MAX_LENGTH 1024
#define PATHNAME_MAX_LENGTH 1024

machine_info **machine_types = NULL; /* Array of available machines */
int machine_count = 0;

machine_info *machine_current;  /* The currently selected machine */
static int machine_location;	/* Where is the current machine in
				   machine_types[...]? */

static int machine_add_machine( int (*init_function)(machine_info *machine) );
static int machine_select_machine( machine_info *machine );
static int machine_free_machine( machine_info *machine );

int machine_init_machines( void )
{
  int error;

  error = machine_add_machine( spec48_init    );
  if (error ) return error;
  error = machine_add_machine( spec128_init   );
  if (error ) return error;
  error = machine_add_machine( specplus2_init );
  if (error ) return error;
  error = machine_add_machine( specplus3_init );
  if (error ) return error;

  return 0;
}

static int machine_add_machine( int (*init_function)( machine_info *machine ) )
{
  int error;

  machine_count++;

  machine_types = 
    (machine_info**)realloc( machine_types, 
			     machine_count * sizeof( machine_info* ) );
  if( machine_types == NULL ) {
    fprintf(stderr, "%s: out of memory at %s:%d\n", fuse_progname,
	    __FILE__, __LINE__ );
    return 1;
  }

  machine_types[ machine_count - 1 ] =
    (machine_info*)malloc( sizeof( machine_info ) );
  if( machine_types[ machine_count - 1 ] == NULL ) {
    fprintf(stderr, "%s: out of memory at %s:%d\n", fuse_progname,
	    __FILE__, __LINE__ );
    return 1;
  }

  error = init_function( machine_types[ machine_count - 1 ] );
  if( error ) return error;

  return 0;
}

int machine_select_first( void )
{
  int error;

  machine_location = 0;
  error = machine_select_machine( machine_types[0] );
  if( error ) return error;

  return 0;
}

int machine_select_next( void )
{
  if( ++machine_location == machine_count ) machine_location = 0;
  machine_select_machine( machine_types[ machine_location ] );
  
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
      if( error ) return error;
      return 0;
    }
  }

  fprintf( stderr, "%s: Machine type `%d' unavailable\n", fuse_progname,
	   type );
  return 1;
}

static int machine_select_machine( machine_info *machine )
{
  machine_current = machine;

  tstates = 0;

  /* Reset the event stack */
  event_reset();
  if( event_add( machine->timings.cycles_per_frame, EVENT_TYPE_INTERRUPT) )
    return 1;
  if( event_add( machine->line_times[0], EVENT_TYPE_LINE) ) return 1;

  /* Stop the tape playing */
  tape_stop();

  readbyte = machine->ram.read_memory;
  read_screen_memory = machine->ram.read_screen;
  writebyte = machine->ram.write_memory;
  
  ROM = machine->roms;

  machine->reset();

  return 0;
}

void machine_set_timings( machine_info *machine, DWORD hz,
			  WORD left_border_cycles,  WORD screen_cycles,
			  WORD right_border_cycles, WORD retrace_cycles,
			  WORD lines_per_frame, DWORD first_line)
{
  int y;

  machine->timings.hz=hz;

  machine->timings.left_border_cycles  = left_border_cycles;
  machine->timings.screen_cycles       = screen_cycles;
  machine->timings.right_border_cycles = right_border_cycles;
  machine->timings.retrace_cycles      = retrace_cycles;

  machine->timings.cycles_per_line     = left_border_cycles +
                                         screen_cycles +
                                         right_border_cycles +
					 retrace_cycles;

  machine->timings.lines_per_frame     = lines_per_frame;

  machine->timings.cycles_per_frame    = 
    machine->timings.cycles_per_line * 
    (DWORD)lines_per_frame;

  machine->line_times[0]=first_line;
  for( y=1; y<DISPLAY_SCREEN_HEIGHT+1; y++ ) {
    machine->line_times[y] = machine->line_times[y-1] + 
                             machine->timings.cycles_per_line;
  }

}

int machine_allocate_roms( machine_info *machine, size_t count )
{
  machine->rom_count = count;

  machine->roms = (BYTE**)malloc( count * sizeof(BYTE*) );
  if( machine->roms == NULL ) {
    fprintf(stderr, "%s: out of memory at %s:%d\n", fuse_progname,
	    __FILE__, __LINE__ );
    return 1;
  }

  return 0;
}

int machine_read_rom( machine_info *machine, size_t number,
		      const char* filename )
{
  int fd;

  struct stat file_info;

  char error_message[ ERROR_MESSAGE_MAX_LENGTH ];

  assert( number < machine->rom_count );

  fd = machine_find_rom( filename );
  if( fd == -1 ) {
    fprintf( stderr, "%s: couldn't find ROM `%s'", fuse_progname, filename );
    return 1;
  }

  if( fstat( fd, &file_info) ) {
    snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
	      "%s: Couldn't stat `%s'", fuse_progname, filename );
    perror( error_message );
    close(fd);
    return 1;
  }

  machine->roms[number] = mmap( 0, file_info.st_size, PROT_READ,
				MAP_SHARED, fd, 0 );
  if( machine->roms[number] == (void*)-1 ) {
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
    return 1;
  }

  return 0;

}

/* Find a ROM called `filename'; look in the current directory, ./roms
   and the defined roms directory; returns a fd for the ROM on success,
   -1 if it couldn't find the ROM */
int machine_find_rom( const char *filename )
{
  int fd;

  char path_and_filename[ PATHNAME_MAX_LENGTH ];

  strncpy( path_and_filename, filename, PATHNAME_MAX_LENGTH );
  fd = open( path_and_filename, O_RDONLY );
  if( fd != -1 ) return fd;

  snprintf( path_and_filename, PATHNAME_MAX_LENGTH, "roms/%s", filename );
  fd = open( path_and_filename, O_RDONLY );
  if( fd != -1 ) return fd;

  snprintf( path_and_filename, PATHNAME_MAX_LENGTH, "%s/%s",
	    ROMSDIR, filename );
  fd = open( path_and_filename, O_RDONLY );
  if( fd != -1 ) return fd;

  return -1;
}

int machine_end( void )
{
  int i;
  int error;

  for( i=0; i<machine_count; i++ ) {
    error = machine_free_machine( machine_types[i] );
    if( error ) return error;
    free( machine_types[i] );
  }

  free( machine_types );

  return 0;
}

static int machine_free_machine( machine_info *machine )
{
  size_t i;

  char error_message[ ERROR_MESSAGE_MAX_LENGTH ];

  for( i=0; i<machine->rom_count; i++ ) {

    /* FIXME: should carry the length through properly */
    if( munmap( machine->roms[i], 0x4000 ) == -1 ) {
      snprintf( error_message, ERROR_MESSAGE_MAX_LENGTH,
		"%s: couldn't munmap ROM %lu",
		fuse_progname, (unsigned long)i );
      perror( error_message );
      return 1;
    }
  }

  return 0;
}
