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

#include <string.h>

#include "../compat.h"
#include "../infrastructure/startup_manager.h"
#include "../machine.h"
#include "../memory_pages.h"
#include "../peripherals/scld.h"
#include "../rectangle.h"
#include "../settings.h"

libspectrum_dword tstates;

scld scld_last_dec;

int memory_current_screen;

libspectrum_byte RAM[ SPECTRUM_RAM_PAGES ][0x4000] = { { 0 } };

fuse_machine_info *machine_current;

const int LINE_TIME = 224;
const int TOP_BORDER = 24;

void display_reset_frame_count( void );
void display_set_flash_reversed( int reversed );
void display_clear_maybe_dirty( void );
void display_clear_is_dirty( void );
void display_set_maybe_dirty( int y, libspectrum_qword dirty );
libspectrum_qword display_get_is_dirty( int y );
libspectrum_dword display_get_maybe_dirty( int y );

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

struct plot8_record_t {
  int x;
  int y;
  libspectrum_byte data;
  libspectrum_byte ink;
  libspectrum_byte paper;
};

static struct plot8_record_t plot8_last_write;

static void
plot8_count_fn( int x, int y, libspectrum_byte data, libspectrum_byte ink,
                libspectrum_byte paper )
{
  plot8_count++;
  plot8_last_write.x = x;
  plot8_last_write.y = y;
  plot8_last_write.data = data;
  plot8_last_write.ink = ink;
  plot8_last_write.paper = paper;
}

static int
plot8_assert( int count, int x, int y, libspectrum_byte data,
              libspectrum_byte ink, libspectrum_byte paper )
{
  if( plot8_count != count ) {
    fprintf( stderr, "plot8_count: expected %d, got %d\n",
             count, plot8_count );
    return 1;
  }
  if( plot8_last_write.x != x ) {
    fprintf( stderr, "plot8 x: expected %d, got %d\n", x,
             plot8_last_write.x );
    return 1;
  }
  if( plot8_last_write.y != y ) {
    fprintf( stderr, "plot8 y: expected %d, got %d\n", y,
             plot8_last_write.y );
    return 1;
  }
  if( plot8_last_write.data != data ) {
    fprintf( stderr, "plot8 data: expected 0x%02x, got 0x%02x\n",
             data, plot8_last_write.data );
    return 1;
  }
  if( plot8_last_write.ink != ink ) {
    fprintf( stderr, "plot8 ink: expected 0x%02x, got 0x%02x\n",
             ink, plot8_last_write.ink );
    return 1;
  }
  if( plot8_last_write.paper != paper ) {
    fprintf( stderr, "plot8 paper: expected 0x%02x, got 0x%02x\n",
             paper, plot8_last_write.paper );
    return 1;
  }

  return 0;
}

/* Vector off to the "plot8" implementation for the current test */
void
uidisplay_plot8( int x, int y, libspectrum_byte data, libspectrum_byte ink,
                 libspectrum_byte paper )
{
  plot8_fn( x, y, data, ink, paper );
}

/* Tracking infrastructure for uidisplay_plot16 */

typedef void (*plot16_fn_t)( int x, int y, libspectrum_word data,
                             libspectrum_byte ink, libspectrum_byte paper );

static plot16_fn_t plot16_fn;

static void
plot16_null( int x, int y, libspectrum_word data, libspectrum_byte ink,
             libspectrum_byte paper )
{
  /* Do nothing */
}

/* putpixel tracking (used by the Pentagon 16-colour display path) */

typedef void (*putpixel_fn_t)( int x, int y, int colour );

static putpixel_fn_t putpixel_fn;

static void
putpixel_null( int x, int y, int colour )
{
  /* Do nothing */
}

static int plot16_count;

struct plot16_record_t {
  int x;
  int y;
  libspectrum_word data;
  libspectrum_byte ink;
  libspectrum_byte paper;
};

static struct plot16_record_t plot16_last_write;

static void
plot16_count_fn( int x, int y, libspectrum_word data, libspectrum_byte ink,
                 libspectrum_byte paper )
{
  plot16_count++;
  plot16_last_write.x = x;
  plot16_last_write.y = y;
  plot16_last_write.data = data;
  plot16_last_write.ink = ink;
  plot16_last_write.paper = paper;
}

static int
plot16_assert( int count, int x, int y, libspectrum_word data,
               libspectrum_byte ink, libspectrum_byte paper )
{
  if( plot16_count != count ) {
    fprintf( stderr, "plot16_count: expected %d, got %d\n",
             count, plot16_count );
    return 1;
  }
  if( plot16_last_write.x != x ) {
    fprintf( stderr, "plot16 x: expected %d, got %d\n", x,
             plot16_last_write.x );
    return 1;
  }
  if( plot16_last_write.y != y ) {
    fprintf( stderr, "plot16 y: expected %d, got %d\n", y,
             plot16_last_write.y );
    return 1;
  }
  if( plot16_last_write.data != data ) {
    fprintf( stderr, "plot16 data: expected 0x%04x, got 0x%04x\n",
             data, plot16_last_write.data );
    return 1;
  }
  if( plot16_last_write.ink != ink ) {
    fprintf( stderr, "plot16 ink: expected 0x%02x, got 0x%02x\n",
             ink, plot16_last_write.ink );
    return 1;
  }
  if( plot16_last_write.paper != paper ) {
    fprintf( stderr, "plot16 paper: expected 0x%02x, got 0x%02x\n",
             paper, plot16_last_write.paper );
    return 1;
  }
  return 0;
}

static int putpixel_count;

static void
putpixel_count_fn( int x, int y, int colour )
{
  putpixel_count++;
}

static int write_if_dirty_count;
static int write_if_dirty_last_x;
static int write_if_dirty_last_y;

static void
write_if_dirty_count_fn( int x, int y )
{
  write_if_dirty_count++;
  write_if_dirty_last_x = x;
  write_if_dirty_last_y = y;
}

/* Main program code */

static void
create_fake_machine( void )
{
  size_t y;

  machine_current = libspectrum_malloc( sizeof( *machine_current ) );

  machine_current->timex = 0;
  machine_current->timings.tstates_per_line = LINE_TIME;

  for( y = 0; y < ARRAY_SIZE( machine_current->line_times ); y++ )
    machine_current->line_times[y] = y * LINE_TIME;

  display_dirty_flashing = display_dirty_flashing_sinclair;
}

static void
test_before( void )
{
  memset( RAM[0], 0, ARRAY_SIZE( RAM[0] ) );
  memset( display_last_screen, 0, sizeof( display_last_screen ) );
  display_clear_maybe_dirty();

  putpixel_fn = putpixel_null;
  putpixel_count = 0;

  plot8_fn = plot8_null;
  display_reset_frame_count();
  display_write_if_dirty = display_write_if_dirty_sinclair;

  display_frame();
  display_clear_is_dirty();

  plot8_fn = plot8_count_fn;
  plot16_fn = plot16_null;
  plot8_count = 0;

  write_if_dirty_count = 0;
  write_if_dirty_last_x = -1;
  write_if_dirty_last_y = -1;
}

static void
timex_test_before( libspectrum_byte scld_byte )
{
  memset( RAM[0], 0, ARRAY_SIZE( RAM[0] ) );
  memset( display_last_screen, 0, sizeof( display_last_screen ) );
  display_clear_maybe_dirty();

  scld_last_dec.byte = scld_byte;
  plot8_fn = plot8_null;
  plot16_fn = plot16_null;
  display_reset_frame_count();
  display_write_if_dirty = display_write_if_dirty_timex;

  display_frame();
  display_clear_is_dirty();

  plot8_fn = plot8_count_fn;
  plot16_fn = plot16_count_fn;
  plot8_count = 0;
  plot16_count = 0;

  write_if_dirty_count = 0;
  write_if_dirty_last_x = -1;
  write_if_dirty_last_y = -1;
}

static int
no_write_if_data_unchanged( void )
{
  /* Arrange */
  RAM[0][0] = 0;
  RAM[0][6144] = 0;

  /* Act */
  display_write_if_dirty_sinclair( 0, 0 );

  /* Assert */
  if( plot8_count ) return 1;

  return 0;
}

static int
write_called_for_new_data( void )
{
  /* Arrange */
  RAM[0][0] = 0x01;
  RAM[0][6144] = 0x02;

  /* Act */
  display_write_if_dirty_sinclair( 0, 0 );

  /* Assert */
  if( plot8_assert( 1, 4, 24, 0x01, 2, 0 ) ) return 1;
  if( display_last_screen[ 964 ] != 0x201 ) {
    fprintf( stderr,
             "display_last_screen[964]: expected 0x201, got 0x%x (attr=0x%02x, scld=0x%02x)\n",
             display_last_screen[ 964 ], RAM[0][6144], scld_last_dec.byte );
    return 1;
  }
  if( display_get_is_dirty( 24 ) != ( (libspectrum_qword)1 << 4 ) ) {
    fprintf( stderr, "display_get_is_dirty(24): expected 0x%lx, got 0x%llx\n",
             (unsigned long)( (libspectrum_qword)1 << 4 ),
             (unsigned long long)display_get_is_dirty( 24 ) );
    return 1;
  }

  return 0;
}

static int
write_reads_from_appropriate_x( void )
{
  /* Arrange */
  RAM[0][31] = 0x12;
  RAM[0][6144 + 31] = 0x34;

  /* Act */
  display_write_if_dirty_sinclair( 31, 0 );

  /* Assert */
  if( plot8_assert( 1, 35, 24, 0x12, 4, 6 ) ) return 1;
  if( display_last_screen[ 995 ] != 0x3412 ) return 1;
  if( display_get_is_dirty( 24 ) != ( (libspectrum_qword)1 << 35 ) ) return 1;

  return 0;
}

static int
write_reads_from_appropriate_y( void )
{
  /* Arrange */
  RAM[0][32] = 0x56;
  RAM[0][6144 + 32] = 0x78;

  /* Act */
  display_write_if_dirty_sinclair( 0, 8 );

  /* Assert */
  if( plot8_assert( 1, 4, 32, 0x56, 8, 15 ) ) return 1;
  if( display_last_screen[ 1284 ] != 0x7856 ) return 1;
  if( display_get_is_dirty( 32 ) != ( (libspectrum_qword)1 << 4 ) ) return 1;

  return 0;
}

static int
flash_inverts_colours( void )
{
  /* Arrange */
  RAM[0][0] = 0x01;
  RAM[0][6144] = 0x82;

  display_set_flash_reversed( 1 );

  /* Act */
  display_write_if_dirty_sinclair( 0, 0 );

  /* Assert */
  if( plot8_assert( 1, 4, 24, 0x01, 0, 2 ) ) return 1;
  if( display_last_screen[ 964 ] != 0x01008201 ) return 1;
  if( display_get_is_dirty( 24 ) != ( (libspectrum_qword)1 << 4 ) ) return 1;

  return 0;
}

static int
no_write_if_nothing_dirty( void )
{
  /* Arrange */
  tstates = (TOP_BORDER + 1) * LINE_TIME;
  display_write_if_dirty = write_if_dirty_count_fn;

  /* Act */
  display_dirty_sinclair( 0x0000 );

  /* Assert */
  if( write_if_dirty_count ) return 1;

  return 0;
}

static int
write_if_dirty( void )
{
  /* Arrange */
  tstates = (TOP_BORDER + 1) * LINE_TIME;
  display_set_maybe_dirty( 0, 0x01 );
  display_write_if_dirty = write_if_dirty_count_fn;

  /* Act */
  display_dirty_sinclair( 0x0000 );

  /* Assert */
  if( write_if_dirty_count != 1 ) return 1;
  if( write_if_dirty_last_y != 0 ) return 1;
  if( write_if_dirty_last_x != 0 ) return 1;

  return 0;
}

static int
no_write_if_dirty_area_ahead_of_beam( void )
{
  /* Arrange */
  tstates = (TOP_BORDER + 1) * LINE_TIME;
  display_set_maybe_dirty( 2, 0x01 );
  display_write_if_dirty = write_if_dirty_count_fn;

  /* Act */
  display_dirty_sinclair( 0x0000 );

  /* Assert */
  if( write_if_dirty_count ) return 1;

  return 0;
}

static int
no_write_if_modified_area_ahead_of_critical_region( void )
{
  /* Arrange */
  tstates = TOP_BORDER * LINE_TIME;
  display_set_maybe_dirty( 0, 0x01 );
  display_write_if_dirty = write_if_dirty_count_fn;

  /* Act */
  display_dirty_sinclair( 0x0020 );

  /* Assert */
  if( write_if_dirty_count ) return 1;

  return 0;
}

/* display_dirty_flashing_sinclair() tests */

static int
flash_dirty_no_flash_attrs( void )
{
  int y;

  /* Arrange: all attribute bytes have flash bit clear (RAM zeroed by test_before) */

  /* Act */
  display_dirty_flashing_sinclair();

  /* Assert: no maybe_dirty bits should be set */
  for( y = 0; y < DISPLAY_HEIGHT; y++ )
    if( display_get_maybe_dirty( y ) ) return 1;

  return 0;
}

static int
flash_dirty_with_flash_attr_row0_col0( void )
{
  int i;

  /* Arrange: set flash bit in attribute for row 0, col 0 (offset 0x1800) */
  RAM[0][0x1800] = 0x80;

  /* Act */
  display_dirty_flashing_sinclair();

  /* Assert: all 8 pixel rows for that attribute cell must be dirty at col 0 */
  for( i = 0; i < 8; i++ )
    if( !( display_get_maybe_dirty( i ) & 0x01 ) ) return 1;

  /* Row 8 onwards must be clean */
  for( i = 8; i < DISPLAY_HEIGHT; i++ )
    if( display_get_maybe_dirty( i ) ) return 1;

  return 0;
}

static int
flash_dirty_non_flash_attr( void )
{
  int y;

  /* Arrange: non-flash attr at row 0, col 0 (bit 7 clear) */
  RAM[0][0x1800] = 0x47;

  /* Act */
  display_dirty_flashing_sinclair();

  /* Assert: no maybe_dirty bits should be set */
  for( y = 0; y < DISPLAY_HEIGHT; y++ )
    if( display_get_maybe_dirty( y ) ) return 1;

  return 0;
}

/* display_dirty_flashing_timex() tests */

static void
timex_flash_test_before( libspectrum_byte scld_byte )
{
  test_before();
  scld_last_dec.byte = scld_byte;
}

static int
timex_flash_hires_skips_flashing( void )
{
  int y;

  /* Arrange: hires mode; set flash bits in both screen areas */
  timex_flash_test_before( HIRES );
  RAM[0][ALTDFILE_OFFSET] = 0x80;
  RAM[0][0x3800] = 0x80;

  /* Act */
  display_dirty_flashing_timex();

  /* Assert: hires path is a no-op — no maybe_dirty bits should be set */
  for( y = 0; y < DISPLAY_HEIGHT; y++ )
    if( display_get_maybe_dirty( y ) ) return 1;

  return 0;
}

static int
timex_flash_b1_no_attrs_clean( void )
{
  int y;

  /* Arrange: b1 mode, no flash bytes in alternate pixel area (RAM zeroed) */
  timex_flash_test_before( 0x02 ); /* b1=1, hires=0, altdfile=0 */

  /* Act */
  display_dirty_flashing_timex();

  /* Assert: no maybe_dirty bits set */
  for( y = 0; y < DISPLAY_HEIGHT; y++ )
    if( display_get_maybe_dirty( y ) ) return 1;

  return 0;
}

static int
timex_flash_b1_marks_dirty( void )
{
  /* Arrange: b1 mode; set flash bit at first byte of second screen pixel area */
  timex_flash_test_before( 0x02 ); /* b1=1, hires=0, altdfile=0 */
  RAM[0][ALTDFILE_OFFSET] = 0x80;  /* offset 0x2000, maps to pixel row 0, col 0 */

  /* Act */
  display_dirty_flashing_timex();

  /* Assert: pixel row 0, col 0 must be marked dirty */
  if( !( display_get_maybe_dirty( 0 ) & 0x01 ) ) {
    fprintf( stderr,
             "timex_flash_b1_marks_dirty: expected maybe_dirty[0] bit 0 set\n" );
    return 1;
  }

  return 0;
}

static int
timex_flash_altdfile_marks_dirty( void )
{
  int i;

  /* Arrange: altdfile mode; flash attr at row 0, col 0 of second screen */
  timex_flash_test_before( ALTDFILE ); /* altdfile=1, b1=0, hires=0 */
  RAM[0][0x3800] = 0x80;              /* second screen attr area, row 0 col 0 */

  /* Act */
  display_dirty_flashing_timex();

  /* Assert: all 8 pixel rows for that attribute cell dirty at col 0 */
  for( i = 0; i < 8; i++ ) {
    if( !( display_get_maybe_dirty( i ) & 0x01 ) ) {
      fprintf( stderr,
               "timex_flash_altdfile_marks_dirty: expected maybe_dirty[%d] bit 0 set\n",
               i );
      return 1;
    }
  }

  /* Rows 8 onwards must be clean */
  for( i = 8; i < DISPLAY_HEIGHT; i++ )
    if( display_get_maybe_dirty( i ) ) return 1;

  return 0;
}

static int
timex_flash_standard_delegates_to_sinclair( void )
{
  int i;

  /* Arrange: standard Timex mode (scld=0); flash attr at row 0, col 0 */
  timex_flash_test_before( STANDARD );
  RAM[0][0x1800] = 0x80;

  /* Act: standard path delegates to display_dirty_flashing_sinclair() */
  display_dirty_flashing_timex();

  /* Assert: same result as sinclair flash test — 8 rows dirty at col 0 */
  for( i = 0; i < 8; i++ ) {
    if( !( display_get_maybe_dirty( i ) & 0x01 ) ) {
      fprintf( stderr,
               "timex_flash_standard_delegates: expected maybe_dirty[%d] bit 0 set\n",
               i );
      return 1;
    }
  }

  for( i = 8; i < DISPLAY_HEIGHT; i++ )
    if( display_get_maybe_dirty( i ) ) return 1;

  return 0;
}

/* display_write_if_dirty_timex() tests */

static int
timex_lores_no_redraw_if_unchanged( void )
{
  /* Arrange: STANDARD mode, all-zero RAM (matches zeroed display_last_screen) */
  timex_test_before( STANDARD );

  /* Act */
  display_write_if_dirty_timex( 0, 0 );

  /* Assert: cache hit — no redraw */
  if( plot8_count ) return 1;

  return 0;
}

static int
timex_lores_write_called_for_new_data( void )
{
  /* Arrange: STANDARD mode, non-zero pixel and attribute data */
  timex_test_before( STANDARD );
  RAM[0][0] = 0x01;
  RAM[0][6144] = 0x02; /* ink=2, paper=0 */

  /* Act */
  display_write_if_dirty_timex( 0, 0 );

  /* Assert: plot8 called; cache updated with (mode_data=0x00, attr=0x02, data=0x01) */
  if( plot8_assert( 1, 4, 24, 0x01, 2, 0 ) ) return 1;
  if( display_last_screen[ 964 ] != 0x00000201 ) {
    fprintf( stderr,
             "display_last_screen[964]: expected 0x201, got 0x%x\n",
             display_last_screen[ 964 ] );
    return 1;
  }

  return 0;
}

static int
timex_mode_change_causes_redraw( void )
{
  /* Arrange: draw once in STANDARD mode to prime the cache */
  timex_test_before( STANDARD );
  RAM[0][0] = 0x01;
  RAM[0][6144] = 0x02;
  display_write_if_dirty_timex( 0, 0 );
  plot8_count = 0;

  /* Act: change mode_data via SCLD byte (pixel/attr bytes unchanged) */
  scld_last_dec.byte = 0x40; /* intdisable set; scrnmode still STANDARD */
  display_write_if_dirty_timex( 0, 0 );

  /* Assert: mode_data differs → cache miss → redraw required */
  if( plot8_count != 1 ) {
    fprintf( stderr, "timex_mode_change: expected plot8_count=1, got %d\n",
             plot8_count );
    return 1;
  }

  return 0;
}

static int
timex_hires_plot16_called_with_correct_data( void )
{
  /* Arrange: HIRES mode; pixel from first screen, pixel from second screen */
  timex_test_before( HIRES );
  RAM[0][0] = 0xAA;
  RAM[0][ALTDFILE_OFFSET] = 0x55;

  /* Act */
  display_write_if_dirty_timex( 0, 0 );

  /* Assert: uidisplay_plot16 called with hires_data = (0xAA<<8)|0x55 */
  if( plot16_assert( 1, 4, 24, 0xAA55, 0, 0 ) ) return 1;

  return 0;
}

typedef int (*test_fn_t)( void );

struct test_t {
  const char *name;
  test_fn_t fn;
};

/* Setup for Pentagon 16-colour display tests */

static void
pentagon_test_before( void )
{
  memset( RAM[4], 0, sizeof( RAM[4] ) );
  memset( RAM[5], 0, sizeof( RAM[5] ) );
  memset( RAM[6], 0, sizeof( RAM[6] ) );
  memset( RAM[7], 0, sizeof( RAM[7] ) );
  memset( display_last_screen, 0, sizeof( display_last_screen ) );
  display_clear_is_dirty();

  putpixel_fn = putpixel_count_fn;
  putpixel_count = 0;

  memory_current_screen = 5;
}

/* display_write_if_dirty_pentagon_16_col() tests */

static int
pentagon_no_write_if_data_unchanged( void )
{
  pentagon_test_before();

  /* All RAM pages and display_last_screen are zero — cache is current,
     so no pixels should be emitted. */
  display_write_if_dirty_pentagon_16_col( 0, 0 );

  if( putpixel_count ) {
    fprintf( stderr, "putpixel_count: expected 0, got %d\n", putpixel_count );
    return 1;
  }

  return 0;
}

static int
pentagon_write_called_for_new_data( void )
{
  pentagon_test_before();

  /* Set data2 (page 5 base offset) to a nonzero value. */
  RAM[5][0] = 0x01;

  display_write_if_dirty_pentagon_16_col( 0, 0 );

  /* 8 pixels must be emitted. */
  if( putpixel_count != 8 ) {
    fprintf( stderr, "putpixel_count: expected 8, got %d\n", putpixel_count );
    return 1;
  }

  /* last_chunk_detail = (data4<<24)|(data3<<16)|(data2<<8)|data1 = 0x100 */
  if( display_last_screen[ 964 ] != 0x100 ) {
    fprintf( stderr,
             "display_last_screen[964]: expected 0x100, got 0x%x\n",
             display_last_screen[ 964 ] );
    return 1;
  }

  if( display_get_is_dirty( 24 ) != ( (libspectrum_qword)1 << 4 ) ) {
    fprintf( stderr,
             "display_is_dirty[24]: expected bit 4 set\n" );
    return 1;
  }

  return 0;
}

static int
pentagon_page7_reads_correct_pages( void )
{
  pentagon_test_before();
  memory_current_screen = 7;

  /* Set data2 from page 7 base offset — verifies page 7/6 selection. */
  RAM[7][0] = 0x11;

  display_write_if_dirty_pentagon_16_col( 0, 0 );

  if( putpixel_count != 8 ) {
    fprintf( stderr, "putpixel_count: expected 8, got %d\n", putpixel_count );
    return 1;
  }

  /* last_chunk_detail = 0|(0<<16)|(0x11<<8)|0 = 0x1100 */
  if( display_last_screen[ 964 ] != 0x1100 ) {
    fprintf( stderr,
             "display_last_screen[964]: expected 0x1100, got 0x%x\n",
             display_last_screen[ 964 ] );
    return 1;
  }

  return 0;
}

static const struct test_t tests[] = {
  /* display_write_if_dirty_sinclair() tests */
  { "no_write_if_data_unchanged", no_write_if_data_unchanged },
  { "write_called_for_new_data", write_called_for_new_data },
  { "write_reads_from_appropriate_x", write_reads_from_appropriate_x },
  { "write_reads_from_appropriate_y", write_reads_from_appropriate_y },
  { "flash_inverts_colours", flash_inverts_colours },

  /* display_dirty_sinclair() tests */
  { "no_write_if_nothing_dirty", no_write_if_nothing_dirty },
  { "write_if_dirty", write_if_dirty },
  { "no_write_if_dirty_area_ahead_of_beam",
    no_write_if_dirty_area_ahead_of_beam },
  { "no_write_if_modified_area_ahead_of_critical_region",
    no_write_if_modified_area_ahead_of_critical_region },

  /* display_dirty_flashing_sinclair() tests */
  { "flash_dirty_no_flash_attrs", flash_dirty_no_flash_attrs },
  { "flash_dirty_with_flash_attr_row0_col0",
    flash_dirty_with_flash_attr_row0_col0 },
  { "flash_dirty_non_flash_attr", flash_dirty_non_flash_attr },

  /* display_dirty_flashing_timex() tests */
  { "timex_flash_hires_skips_flashing",
    timex_flash_hires_skips_flashing },
  { "timex_flash_b1_no_attrs_clean",
    timex_flash_b1_no_attrs_clean },
  { "timex_flash_b1_marks_dirty",
    timex_flash_b1_marks_dirty },
  { "timex_flash_altdfile_marks_dirty",
    timex_flash_altdfile_marks_dirty },
  { "timex_flash_standard_delegates_to_sinclair",
    timex_flash_standard_delegates_to_sinclair },

  /* display_write_if_dirty_timex() tests */
  { "timex_lores_no_redraw_if_unchanged",
    timex_lores_no_redraw_if_unchanged },
  { "timex_lores_write_called_for_new_data",
    timex_lores_write_called_for_new_data },
  { "timex_mode_change_causes_redraw",
    timex_mode_change_causes_redraw },
  { "timex_hires_plot16_called_with_correct_data",
    timex_hires_plot16_called_with_correct_data },

  /* display_write_if_dirty_pentagon_16_col() tests */
  { "pentagon_no_write_if_data_unchanged",
    pentagon_no_write_if_data_unchanged },
  { "pentagon_write_called_for_new_data",
    pentagon_write_called_for_new_data },
  { "pentagon_page7_reads_correct_pages",
    pentagon_page7_reads_correct_pages },

  /* End marker */
  { NULL, NULL }
};

#ifdef main
/* SDL headers redefine main on Windows, but this test needs a normal entry point. */
#undef main
#endif

int
main( int argc, char *argv[] )
{
  const struct test_t *test;

  if( display_init( &argc, &argv ) ) {
    fprintf( stderr, "Error from display_init()\n");
    return 1;
  }

  create_fake_machine();

  for( test = tests; test->fn; test++ ) {
    test_before();
    int result = test->fn();
    if( result ) {
      fprintf( stderr, "Test failed: %s\n", test->name );
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
void uidisplay_putpixel( int x, int y, int colour ) { putpixel_fn( x, y, colour ); }

void
uidisplay_plot16( int x, int y, libspectrum_word data, libspectrum_byte ink,
                  libspectrum_byte paper )
{
  plot16_fn( x, y, data, ink, paper );
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
