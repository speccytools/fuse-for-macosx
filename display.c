/* display.c: Routines for printing the Spectrum screen
   Copyright (c) 1999-2006 Philip Kendall, Thomas Harte, Witold Filipczyk
                           and Fredrick Meunier

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
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "display.h"
#include "event.h"
#include "fuse.h"
#include "machine.h"
#include "screenshot.h"
#include "settings.h"
#include "spectrum.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "scld.h"

/* Set once we have initialised the UI */
int display_ui_initialised = 0;

/* A copy of every pixel on the screen */
libspectrum_word
  display_image[ 2 * DISPLAY_SCREEN_HEIGHT ][ DISPLAY_SCREEN_WIDTH ];
ptrdiff_t display_pitch = DISPLAY_SCREEN_WIDTH * sizeof( libspectrum_word );

/* The current border colour */
libspectrum_byte display_lores_border;
libspectrum_byte display_hires_border;
libspectrum_byte display_last_border;

/* The border colour displayed on every line if it is homogeneous,
   or display_border_mixed (see below) if it's not */
static libspectrum_byte display_current_border[ DISPLAY_SCREEN_HEIGHT ];

/* Stores the pixel, attribute and SCLD screen mode information used to
   draw each 8x1 group of pixels (including border) last frame */
static libspectrum_byte
display_last_screen[ DISPLAY_SCREEN_WIDTH_COLS*DISPLAY_SCREEN_HEIGHT*3 ];

/* Offsets as to where the data and the attributes for each pixel
   line start */
libspectrum_word display_line_start[ DISPLAY_HEIGHT ];
libspectrum_word display_attr_start[ DISPLAY_HEIGHT ];

/* The number of frames mod 32 that have elapsed.
    0<=d_f_c<16 => Flashing characters are normal
   16<=d_f_c<32 => Flashing characters are reversed
*/
static int display_frame_count;
static int display_flash_reversed;

/* Which eight-pixel chunks on each line (including border) need to
   be redisplayed. Bit 0 corresponds to pixels 0-7, bit 39 to
   pixels 311-319. */
static libspectrum_qword display_is_dirty[ DISPLAY_SCREEN_HEIGHT ];
/* This value signifies that the entire line must be redisplayed */
static libspectrum_qword display_all_dirty;

/* Used to signify that we're redrawing the entire screen */
static int display_redraw_all;

/* Value used to signify a border line has more than one colour on it. */
static const int display_border_mixed = 0xff;

/* Used for grouping screen writes together */
struct rectangle { int x,y; int w,h; };

/* Those rectangles which were modified on the last line to be displayed */
struct rectangle *active_rectangle = NULL;
size_t active_rectangle_count = 0, active_rectangle_allocated = 0;

/* Those rectangles which weren't */
struct rectangle *inactive_rectangle = NULL;
size_t inactive_rectangle_count = 0, inactive_rectangle_allocated = 0;

/* The last point at which we updated the screen display */
int critical_region_x = 0, critical_region_y = 0;

/* The border colour changes which have occured in this frame */
struct border_change_t {
  int x, y;
  int colour;
};

static struct border_change_t border_change_end_sentinel =
  { DISPLAY_SCREEN_WIDTH_COLS, DISPLAY_SCREEN_HEIGHT - 1, 0 };

GSList *border_changes;

/* The current border colour */
int current_border[ DISPLAY_SCREEN_HEIGHT ][ DISPLAY_SCREEN_WIDTH_COLS ];

static void display_get_attr( int x, int y,
			      libspectrum_byte *ink, libspectrum_byte *paper);

static int add_rectangle( int y, int x, int w );
static int end_line( int y );

libspectrum_word
display_get_addr( int x, int y )
{
  if ( scld_last_dec.name.altdfile ) {
    return display_line_start[y]+x+ALTDFILE_OFFSET;
  } else {
    return display_line_start[y]+x;
  }
}

static int
add_border_sentinel( void )
{
  struct border_change_t *sentinel = malloc( sizeof( *sentinel ) );

  if( !sentinel ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  sentinel->x = sentinel->y = 0;
  sentinel->colour = scld_last_dec.name.hires ?
                            display_hires_border : display_lores_border;

  border_changes = g_slist_prepend( border_changes, sentinel );

  return 0;
}

int
display_init( int *argc, char ***argv )
{
  int i, j, k, y;
  int error;

  if(ui_init(argc, argv))
    return 1;

  /* Set up the 'all pixels must be refreshed' marker */
  display_all_dirty = 0;
  for( i = 0; i < DISPLAY_SCREEN_WIDTH_COLS; i++ )
    display_all_dirty = ( display_all_dirty << 1 ) | 0x01;

  memset( display_last_screen, 255,
          DISPLAY_SCREEN_WIDTH_COLS*DISPLAY_SCREEN_HEIGHT*3 );

  for(i=0;i<3;i++)
    for(j=0;j<8;j++)
      for(k=0;k<8;k++)
	display_line_start[ (64*i) + (8*j) + k ] =
	  32 * ( (64*i) + j + (k*8) );

  for(y=0;y<DISPLAY_HEIGHT;y++) {
    display_attr_start[y]=6144 + (32*(y/8));
  }

  display_frame_count=0; display_flash_reversed=0;

  for( y = 0; y < DISPLAY_SCREEN_HEIGHT; y++ )
    display_is_dirty[ y ] = display_all_dirty;

  for( y = 0; y < DISPLAY_SCREEN_HEIGHT; y++ )
    display_current_border[ y ] = display_border_mixed;

  display_redraw_all = 0;

  border_changes = NULL;
  error = add_border_sentinel(); if( error ) return error;
  display_last_border = scld_last_dec.name.hires ?
                            display_hires_border : display_lores_border;

  return 0;
}

/* Add the rectangle { x, line, w, 1 } to the list of rectangles to be
   redrawn, either by extending an existing rectangle or creating a
   new one */
static int
add_rectangle( int y, int x, int w )
{
  size_t i;
  struct rectangle *ptr;

  /* Check through all 'active' rectangles (those which were modified
     on the previous line) and see if we can use this new rectangle
     to extend them */
  for( i = 0; i < active_rectangle_count; i++ ) {

    if( active_rectangle[i].x == x &&
	active_rectangle[i].w == w    ) {
      active_rectangle[i].h++;
      return 0;
    }
  }

  /* We couldn't find a rectangle to extend, so create a new one */
  if( ++active_rectangle_count > active_rectangle_allocated ) {

    size_t new_alloc;

    new_alloc = active_rectangle_allocated     ?
                2 * active_rectangle_allocated :
                8;

    ptr = realloc( active_rectangle, new_alloc * sizeof( struct rectangle ) );
    if( !ptr ) {
      ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
      return 1;
    }

    active_rectangle_allocated = new_alloc; active_rectangle = ptr;
  }

  ptr = &active_rectangle[ active_rectangle_count - 1 ];

  ptr->x = x; ptr->y = y;
  ptr->w = w; ptr->h = 1;

  return 0;
}

#ifndef MAX
#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif

inline static int
compare_and_merge_rectangles( struct rectangle *source )
{
  size_t z;

  /* Now look to see if there is an overlapping rectangle in the inactive
     list.  These occur when frame skip is on and the same lines are
     covered more than once... */
  for( z = 0; z < inactive_rectangle_count; z++ ) {
    if( inactive_rectangle[z].x == source->x &&
          inactive_rectangle[z].w == source->w ) {
      if( inactive_rectangle[z].y == source->y &&
            inactive_rectangle[z].h == source->h )
        return 1;

      if( ( inactive_rectangle[z].y < source->y && 
          ( source->y < ( inactive_rectangle[z].y +
            inactive_rectangle[z].h + 1 ) ) ) ||
          ( source->y < inactive_rectangle[z].y && 
          ( inactive_rectangle[z].y < ( source->y + source->h + 1 ) ) ) ) {
        /* rects overlap or touch in the y dimension, merge */
        inactive_rectangle[z].h = MAX( inactive_rectangle[z].y +
                                    inactive_rectangle[z].h,
                                    source->y + source->h ) -
                                  MIN( inactive_rectangle[z].y, source->y );
        inactive_rectangle[z].y = MIN( inactive_rectangle[z].y, source->y );

        return 1;
      }
    }
    if( inactive_rectangle[z].y == source->y &&
          inactive_rectangle[z].h == source->h ) {

      if( (inactive_rectangle[z].x < source->x && 
          ( source->x < ( inactive_rectangle[z].x +
            inactive_rectangle[z].w + 1 ) ) ) ||
          ( source->x < inactive_rectangle[z].x && 
          ( inactive_rectangle[z].x < ( source->x + source->w + 1 ) ) ) ) {
        /* rects overlap or touch in the x dimension, merge */
        inactive_rectangle[z].w = MAX( inactive_rectangle[z].x +
          inactive_rectangle[z].w, source->x +
          source->w ) - MIN( inactive_rectangle[z].x, source->x );
        inactive_rectangle[z].x = MIN( inactive_rectangle[z].x, source->x );
        return 1;
      }
    }
     /* Handle overlaps offset by both x and y? how much overlap and hence 
        overdraw can be tolerated? */
  }
  return 0;
}

/* Move all rectangles not updated on this line to the inactive list */
static int
end_line( int y )
{
  size_t i;
  struct rectangle *ptr;

  for( i = 0; i < active_rectangle_count; i++ ) {

    /* Skip if this rectangle was updated this line */
    if( active_rectangle[i].y + active_rectangle[i].h == y + 1 ) continue;

    if ( settings_current.frame_rate > 1 &&
	 compare_and_merge_rectangles( &active_rectangle[i] ) ) {

      /* Mark the active rectangle as done */
      active_rectangle[i].h = 0;
      continue;
    }

    /* We couldn't find a rectangle to extend, so create a new one */
    if( ++inactive_rectangle_count > inactive_rectangle_allocated ) {

      size_t new_alloc;

      new_alloc = inactive_rectangle_allocated     ?
	          2 * inactive_rectangle_allocated :
	          8;

      ptr = realloc( inactive_rectangle,
		     new_alloc * sizeof( struct rectangle ) );
      if( !ptr ) {
	ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d",
		  __FILE__, __LINE__ );
	return 1;
      }

      inactive_rectangle_allocated = new_alloc; inactive_rectangle = ptr;
    }

    inactive_rectangle[ inactive_rectangle_count - 1 ] = active_rectangle[i];

    /* Mark the active rectangle as done */
    active_rectangle[i].h = 0;
  }

  /* Compress the list of active rectangles */
  for( i = 0, ptr = active_rectangle; i < active_rectangle_count; i++ ) {
    if( active_rectangle[i].h == 0 ) continue;
    *ptr = active_rectangle[i]; ptr++;
  }

  active_rectangle_count = ptr - active_rectangle;

  return 0;
}

static void
update_dirty_rects( void )
{
  int start, y, error;

  for( y=0; y<DISPLAY_SCREEN_HEIGHT; y++ ) {
    int x = 0;
    while( display_is_dirty[y] ) {

      /* Find the first dirty chunk on this row */
      while( !( display_is_dirty[y] & 0x01 ) ) {
        display_is_dirty[y] >>= 1;
        x++;
      }

      start = x;

      /* Walk to the end of the dirty region, writing the bytes to the
         drawing area along the way */
      do {

        display_is_dirty[y] >>= 1;
        x++;

      } while( display_is_dirty[y] & 0x01 );

      error = add_rectangle( y, start, x - start );
      if( error ) return;
    }
  
    /* compress the active rectangles list */
    error = end_line( y ); if( error ) return;
  }

  /* Force all rectangles into the inactive list */
  error = end_line( DISPLAY_SCREEN_HEIGHT ); if( error ) return;
}

static inline void
get_beam_position( int time, int *x, int *y )
{
  if( time < machine_current->line_times[ 0 ] ) {
    *x = *y = -1;
    return;
  }

  *y = ( time - machine_current->line_times[ 0 ] ) /
    machine_current->timings.tstates_per_line;

  if( *y >= DISPLAY_SCREEN_HEIGHT ) {
    *x = DISPLAY_SCREEN_WIDTH_COLS;
    *y = DISPLAY_SCREEN_HEIGHT - 1;
    return;
  }

  *x = ( time - machine_current->line_times[ *y ] ) / 4;
}

/* Set one pixel in the display */
void
display_putpixel( int x, int y, int colour )
{
  if( machine_current->timex ) {
    x <<= 1; y <<= 1;
    display_image[y  ][x  ] = colour;
    display_image[y  ][x+1] = colour;
    display_image[y+1][x  ] = colour;
    display_image[y+1][x+1] = colour;
  } else {
    display_image[y][x] = colour;
  }
}

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y ) */
void
display_plot8( int x, int y, libspectrum_byte data,
	       libspectrum_byte ink, libspectrum_byte paper )
{
  x <<= 3;

  if( machine_current->timex ) {
    int i;

    x <<= 1; y <<= 1;
    for( i=0; i<2; i++,y++ ) {
      display_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
      display_image[y][x+ 1] = ( data & 0x80 ) ? ink : paper;
      display_image[y][x+ 2] = ( data & 0x40 ) ? ink : paper;
      display_image[y][x+ 3] = ( data & 0x40 ) ? ink : paper;
      display_image[y][x+ 4] = ( data & 0x20 ) ? ink : paper;
      display_image[y][x+ 5] = ( data & 0x20 ) ? ink : paper;
      display_image[y][x+ 6] = ( data & 0x10 ) ? ink : paper;
      display_image[y][x+ 7] = ( data & 0x10 ) ? ink : paper;
      display_image[y][x+ 8] = ( data & 0x08 ) ? ink : paper;
      display_image[y][x+ 9] = ( data & 0x08 ) ? ink : paper;
      display_image[y][x+10] = ( data & 0x04 ) ? ink : paper;
      display_image[y][x+11] = ( data & 0x04 ) ? ink : paper;
      display_image[y][x+12] = ( data & 0x02 ) ? ink : paper;
      display_image[y][x+13] = ( data & 0x02 ) ? ink : paper;
      display_image[y][x+14] = ( data & 0x01 ) ? ink : paper;
      display_image[y][x+15] = ( data & 0x01 ) ? ink : paper;
    }
  } else {
    display_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
    display_image[y][x+ 1] = ( data & 0x40 ) ? ink : paper;
    display_image[y][x+ 2] = ( data & 0x20 ) ? ink : paper;
    display_image[y][x+ 3] = ( data & 0x10 ) ? ink : paper;
    display_image[y][x+ 4] = ( data & 0x08 ) ? ink : paper;
    display_image[y][x+ 5] = ( data & 0x04 ) ? ink : paper;
    display_image[y][x+ 6] = ( data & 0x02 ) ? ink : paper;
    display_image[y][x+ 7] = ( data & 0x01 ) ? ink : paper;
  }
}

/* Print the 16 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (16*x) , y ) */
void
display_plot16( int x, int y, libspectrum_word data,
		libspectrum_byte ink, libspectrum_byte paper )
{
  int i;
  x <<= 4;

  for( i=0; i<2; i++,y++ ) {
    display_image[y][x+ 0] = ( data & 0x8000 ) ? ink : paper;
    display_image[y][x+ 1] = ( data & 0x4000 ) ? ink : paper;
    display_image[y][x+ 2] = ( data & 0x2000 ) ? ink : paper;
    display_image[y][x+ 3] = ( data & 0x1000 ) ? ink : paper;
    display_image[y][x+ 4] = ( data & 0x0800 ) ? ink : paper;
    display_image[y][x+ 5] = ( data & 0x0400 ) ? ink : paper;
    display_image[y][x+ 6] = ( data & 0x0200 ) ? ink : paper;
    display_image[y][x+ 7] = ( data & 0x0100 ) ? ink : paper;
    display_image[y][x+ 8] = ( data & 0x0080 ) ? ink : paper;
    display_image[y][x+ 9] = ( data & 0x0040 ) ? ink : paper;
    display_image[y][x+10] = ( data & 0x0020 ) ? ink : paper;
    display_image[y][x+11] = ( data & 0x0010 ) ? ink : paper;
    display_image[y][x+12] = ( data & 0x0008 ) ? ink : paper;
    display_image[y][x+13] = ( data & 0x0004 ) ? ink : paper;
    display_image[y][x+14] = ( data & 0x0002 ) ? ink : paper;
    display_image[y][x+15] = ( data & 0x0001 ) ? ink : paper;
  }
}

/* Get the attribute byte or equivalent for the eight pixels starting at
   ( (8*x) , y ) */
static inline libspectrum_byte
display_get_attr_byte( int x, int y )
{
  libspectrum_byte attr;

  if ( scld_last_dec.name.hires ) {
    attr = hires_get_attr();
  } else {

    libspectrum_word offset;

    if( scld_last_dec.name.b1 ) {
      offset = display_line_start[y] + x + ALTDFILE_OFFSET;
    } else if( scld_last_dec.name.altdfile ) {
      offset = display_attr_start[y] + x + ALTDFILE_OFFSET;
    } else {
      offset = display_attr_start[y] + x;
    }

    attr = RAM[ memory_current_screen ][ offset ];
  }

  return attr;
}

/* Get the attributes for the eight pixels starting at
   ( (8*x) , y ) */
static void
display_get_attr( int x, int y,
		  libspectrum_byte *ink, libspectrum_byte *paper )
{
  display_parse_attr( display_get_attr_byte( x, y ), ink, paper );
}

void
display_parse_attr( libspectrum_byte attr,
		    libspectrum_byte *ink, libspectrum_byte *paper )
{
  if( (attr & 0x80) && display_flash_reversed ) {
    *ink  = (attr & ( 0x0f << 3 ) ) >> 3;
    *paper= (attr & 0x07) + ( (attr & 0x40) >> 3 );
  } else {
    *ink= (attr & 0x07) + ( (attr & 0x40) >> 3 );
    *paper= (attr & ( 0x0f << 3 ) ) >> 3;
  }
}

static void
push_border_change( int colour )
{
  int beam_x, beam_y;
  struct border_change_t *change;

  get_beam_position( tstates, &beam_x, &beam_y );

  if( beam_x < 0 ) beam_x = 0;
  if( beam_x > DISPLAY_SCREEN_WIDTH_COLS ) beam_x = DISPLAY_SCREEN_WIDTH_COLS;
  if( beam_y < 0 ) beam_y = 0;

  change = malloc( sizeof( *change ) );
  if( !change ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return;
  }

  change->x = beam_x;
  change->y = beam_y;
  change->colour = colour;

  border_changes = g_slist_append( border_changes, change );
}

/* Change border colour if the colour in use changes */
static void
check_border_change()
{
  if( scld_last_dec.name.hires &&
      display_hires_border != display_last_border ) {
    push_border_change( display_hires_border );
    display_last_border = display_hires_border;
  } else if( !scld_last_dec.name.hires &&
             display_lores_border != display_last_border ) {
    push_border_change( display_lores_border );
    display_last_border = display_lores_border;
  }
}

void
display_set_lores_border( int colour )
{
  if( display_lores_border != colour ) {
    display_lores_border = colour;
  }
  check_border_change();
}

void
display_set_hires_border( int colour )
{
  if( display_hires_border != colour ) {
    display_hires_border = colour;
  }
  check_border_change();
}

static void
set_border( int y, int start, int end, int colour )
{
  /* Fake byte */
  libspectrum_byte data = 255;
  /* and fake screen mode */
  libspectrum_byte mode_data = 0;
  int index = (start + y * DISPLAY_SCREEN_WIDTH_COLS)*3;

  for( ; start < end; start++ ) {
    /* Draw it if it is different to what was there last time - we know that
       data and mode will have been the same*/
    if( display_last_screen[ index+1 ] != colour ) {
      display_plot8( start, y, data, colour, 0 );

      /* Update last display record */
      display_last_screen[ index ] = data;
      display_last_screen[ index+1 ] = colour;
      display_last_screen[ index+2 ] = mode_data;

      /* And now mark it dirty */
      display_is_dirty[y] |= ( (libspectrum_qword)1 << start );
    }
    index += 3;
  }
}

static void
border_change_write( int y, int start, int end, int colour )
{
  if(   y <  DISPLAY_BORDER_HEIGHT                    ||
      ( y >= DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT )    ) {

    /* Top and bottom borders */
    set_border( y, start, end, colour );

  } else {

    /* Left border */
    if( start < DISPLAY_BORDER_WIDTH_COLS ) {

      int left_end =
	end > DISPLAY_BORDER_WIDTH_COLS ? DISPLAY_BORDER_WIDTH_COLS : end;

      set_border( y, start, left_end, colour );
    }

    /* Right border */
    if( end > DISPLAY_BORDER_WIDTH_COLS + DISPLAY_WIDTH_COLS ) {

      if( start < DISPLAY_BORDER_WIDTH_COLS + DISPLAY_WIDTH_COLS )
	start = DISPLAY_BORDER_WIDTH_COLS + DISPLAY_WIDTH_COLS;

      set_border( y, start, end, colour );
    }

  }
}

static void
border_change_line_part( int y, int start, int end, int colour )
{
  border_change_write( y, start, end, colour );
  display_current_border[ y ] = display_border_mixed;
}

static void
border_change_line( int y, int colour )
{
  if( display_current_border[y] != colour ) {
    border_change_write( y, 0, DISPLAY_SCREEN_WIDTH_COLS, colour );
    display_current_border[ y ] = colour;
  }
}

static void
do_border_change( struct border_change_t *first,
		  struct border_change_t *second )
{
  if( first->x ) {
    if( first->x != DISPLAY_SCREEN_WIDTH_COLS )
      border_change_line_part( first->y, first->x, DISPLAY_SCREEN_WIDTH_COLS,
			       first->colour );
    end_line( first->y );
    /* Don't extend region past the end of the screen */
    if( first->y < DISPLAY_SCREEN_HEIGHT - 1 ) first->y++;
  }

  for( ; first->y < second->y; first->y++ ) {
    border_change_line( first->y, first->colour );
    end_line( first->y );
  }

  if( second->x ) {
    if( second->x == DISPLAY_SCREEN_WIDTH_COLS ) {
      border_change_line( first->y, first->colour );
    } else {
      border_change_line_part( first->y, 0, second->x, first->colour );
    }
    end_line( first->y );
  }
}

/* Take account of all the border colour changes which happened in this
   frame */
static void
update_border( void )
{
  GSList *first, *second;
  int error;

  /* Put the final sentinel onto the list */
  border_changes = g_slist_append( border_changes,
				   &border_change_end_sentinel );

  for( first = border_changes, second = first->next;
       first->next;
       first = second, second = second->next ) {
    do_border_change( first->data, second->data );
    free( first->data );
  }

  g_slist_free( border_changes ); border_changes = NULL;

  error = add_border_sentinel(); if( error ) return;

  /* Force all rectangles into the inactive list */
  error = end_line( DISPLAY_SCREEN_HEIGHT ); if( error ) return;
}

/* Send the updated screen to the UI-specific code */
static void
update_ui_screen( void )
{
  static int frame_count = 0;
  int scale = machine_current->timex ? 2 : 1;
  size_t i;
  struct rectangle *ptr;

  if( settings_current.frame_rate <= ++frame_count ) {
    frame_count = 0;

    if( display_redraw_all ) {
      uidisplay_area( 0, 0,
		      scale * DISPLAY_ASPECT_WIDTH,
		      scale * DISPLAY_SCREEN_HEIGHT );
      display_redraw_all = 0;
    } else {
      for( i = 0, ptr = inactive_rectangle;
	   i < inactive_rectangle_count;
	   i++, ptr++ ) {
            uidisplay_area( 8 * scale * ptr->x, scale * ptr->y,
			8 * scale * ptr->w, scale * ptr->h );
      }
    }

    inactive_rectangle_count = 0;
    
    uidisplay_frame_end();
  }
}

int
display_frame( void )
{
  update_border();

  update_dirty_rects();

  update_ui_screen();

  if( screenshot_movie_record == 1 ) {

    snprintf( screenshot_movie_name, SCREENSHOT_MOVIE_FILE_MAX,
              "%s-frame-%09ld.scr", screenshot_movie_file,
              screenshot_movie_frame++ );
    screenshot_scr_write( screenshot_movie_name );

#ifdef USE_LIBPNG

  } else if( screenshot_movie_record == 2 ) {

    snprintf( screenshot_movie_name, SCREENSHOT_MOVIE_FILE_MAX,
              "%s-frame-%09ld.scr", screenshot_movie_file,
              screenshot_movie_frame++ );
    screenshot_save();
    screenshot_write_fast( screenshot_movie_name, screenshot_movie_scaler );

#endif                          /* #ifdef USE_LIBPNG */

  }

  display_frame_count++;
  if(display_frame_count==16) {
    display_flash_reversed=1;
  } else if(display_frame_count==32) {
    display_flash_reversed=0;
    display_frame_count=0;
  }
  
  display_write( 0 );

  return 0;
}

void display_refresh_all(void)
{
  size_t i;

  display_redraw_all = 1;

  for( i = 0; i < DISPLAY_SCREEN_HEIGHT; i++ )
    display_is_dirty[i] = display_all_dirty;

  for( i = 0; i < DISPLAY_SCREEN_HEIGHT; i++ )
    display_current_border[i] = display_border_mixed;

  memset( display_last_screen, 255,
          DISPLAY_SCREEN_WIDTH_COLS*DISPLAY_SCREEN_HEIGHT*3 );
}

void
display_write( libspectrum_dword last_tstates )
{
  static int next_ula = 0;
  int beam_x, beam_y;
  int x, y;
  libspectrum_word offset;
  libspectrum_byte *screen;
  libspectrum_byte data, data2;
  libspectrum_byte mode_data;
  libspectrum_word hires_data;

  /* last_tstates == 0 means schedule first event of the frame */
  if( !last_tstates ) {
    next_ula = 0;
    event_add( machine_current->ula_read_sequence[ next_ula ],
               EVENT_TYPE_DISPLAY_WRITE );
    return;
  }

  /* Convert tstates to address */
  get_beam_position( last_tstates, &beam_x, &beam_y );
  x = beam_x - DISPLAY_BORDER_WIDTH_COLS;
  y = beam_y - DISPLAY_BORDER_HEIGHT;
  offset = display_get_addr( x, y );

  /* Read byte, atrr/byte, and screen mode */
  screen = RAM[ memory_current_screen ];
  data = screen[ offset ];
  mode_data = ( display_flash_reversed<<7 |
                scld_last_dec.mask.hirescol |
                scld_last_dec.mask.scrnmode
              );

  if( scld_last_dec.name.hires ) {
    switch( scld_last_dec.mask.scrnmode ) {

    case HIRESATTRALTD:
      offset = display_attr_start[ y ] + x + ALTDFILE_OFFSET;
      data2 = screen[ offset ];
      break;

    case HIRES:
      data2 = screen[ offset + ALTDFILE_OFFSET ];
      break;

    case HIRESDOUBLECOL:
      data2 = data;
      break;

    default: /* case HIRESATTR: */
      offset = display_attr_start[ y ] + x;
      data2 = screen[ offset ];
      break;

    }
    hires_data = (data << 8) + data2;
  } else {
    data2 = display_get_attr_byte( x, y );
  }

  /* And draw it if it is different to what was there last time */
  int index = (beam_x + beam_y * DISPLAY_SCREEN_WIDTH_COLS)*3;
  if( display_last_screen[ index ] != data ||
      display_last_screen[ index+1 ] != data2 ||
      display_last_screen[ index+2 ] != mode_data
    ) {
    libspectrum_byte ink, paper;
    display_get_attr( x, y, &ink, &paper );
    if( scld_last_dec.name.hires ) {
      display_plot16( beam_x, beam_y, hires_data, ink, paper );
    } else {
      display_plot8( beam_x, beam_y, data, ink, paper );
    }

    /* Update last display record */
    display_last_screen[ index ] = data;
    display_last_screen[ index+1 ] = data2;
    display_last_screen[ index+2 ] = mode_data;

    /* And now mark it dirty */
    display_is_dirty[beam_y] |= ( (libspectrum_qword)1 << beam_x );
  }

  /* Schedule next EVENT_TYPE_DISPLAY_WRITE as long as there are reads
     still to be done this frame */
  if( ++next_ula < DISPLAY_WIDTH_COLS*DISPLAY_HEIGHT )
    event_add( machine_current->ula_read_sequence[ next_ula ],
                EVENT_TYPE_DISPLAY_WRITE );
}
