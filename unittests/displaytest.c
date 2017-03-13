/* displaytest.c: Test program for Fuse's display code
   Copyright (c) 2017 Philip Kendall

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

#include "../infrastructure/startup_manager.h"
#include "../machine.h"
#include "../memory_pages.h"
#include "../peripherals/scld.h"
#include "../rectangle.h"
#include "../settings.h"

libspectrum_dword tstates;

scld scld_last_dec;

int memory_current_screen;

libspectrum_byte RAM[ SPECTRUM_RAM_PAGES ][0x4000];

fuse_machine_info *machine_current;

/* Various "mocks" for the UI code */

typedef void (*plot8_fn_t)( int x, int y, libspectrum_byte data,
                            libspectrum_byte ink, libspectrum_byte paper );

static plot8_fn_t plot8_fn;

static void
plot8_null( int x, int y, libspectrum_byte data, libspectrum_byte ink,
            libspectrum_byte paper )
{
  /* Do nothing */
}

static int plot8_count;

static void
plot8_count_fn( int x, int y, libspectrum_byte data, libspectrum_byte ink,
                libspectrum_byte paper )
{
  plot8_count++;
}

/* Vector off to the "plot8" implementation for the current test */
void
uidisplay_plot8( int x, int y, libspectrum_byte data, libspectrum_byte ink,
                 libspectrum_byte paper )
{
  plot8_fn( x, y, data, ink, paper );
}

/* Main program code */

static void
create_fake_machine( void )
{
  machine_current = libspectrum_malloc( sizeof( *machine_current ) );

  machine_current->timex = 0;

  display_write_if_dirty = display_write_if_dirty_sinclair;
}

static void
test_before( void )
{
  plot8_fn = plot8_null;
  display_frame();
}

static int
no_write_if_data_unchanged( void )
{
  /* Arrange */
  test_before();

  plot8_fn = plot8_count_fn;
  plot8_count = 0;

  RAM[0][0] = 0;

  /* Act */
  display_write_if_dirty_sinclair( 0, 0 );

  /* Assert */
  if( plot8_count ) return 1;

  return 0;
}

typedef int (*test_fn_t)( void );

static test_fn_t tests[] = {
  no_write_if_data_unchanged,
  NULL
};

int
main( int argc, char *argv[] )
{
  test_fn_t *test;

  if( display_init( &argc, &argv ) ) {
    fprintf( stderr, "Error from display_init()\n");
    return 1;
  }

  create_fake_machine();

  for( test = tests; *test; test++ ) {
    int result = (*test)();
    if( result ) {
      fprintf( stderr, "Test failed\n" );
      return 1;
    }
  }

  return 0;
}

/* Dummy code for the rest of the UI */

int
ui_init( int *argc, char ***argv )
{
  return 0;
}

void uidisplay_area( int x, int y, int w, int h ) {}
void uidisplay_frame_end( void ) {}
void uidisplay_putpixel( int x, int y, int colour ) {}

void
uidisplay_plot16( int x, int y, libspectrum_byte data, libspectrum_byte ink,
                  libspectrum_byte paper )
{
}

/* Dummy movie code */

int movie_recording;

void movie_start_frame( void ) {}
void movie_add_area( int x, int y, int w, int h ) {}

/* Dummy rectangle code */

struct rectangle *rectangle_inactive;
size_t rectangle_inactive_count, rectangle_inactive_allocated;

void rectangle_add( int y, int x, int w ) {}
void rectangle_end_line( int y ) {}

/* Dummy SCLD code */

libspectrum_byte hires_get_attr( void )
{
  return 0;
}

libspectrum_byte hires_convert_dec( libspectrum_byte attr )
{
  return 0;
}

/* Miscellaneous dummy code */

settings_info settings_current;

void startup_manager_register_no_dependencies(
  startup_manager_module module, startup_manager_init_fn init_fn,
  void *init_context, startup_manager_end_fn end_fn )
{
}
