/* display.c: Routines for printing the Spectrum screen
   Copyright (c) 1999-2003 Philip Kendall, Thomas Harte, Witold Filipczyk
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

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "display.h"
#include "event.h"
#include "fuse.h"
#include "machine.h"
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

/* The border colour displayed on every line if it is homogeneous,
   or display_border_mixed (see below) if it's not */
static libspectrum_byte display_current_border[ DISPLAY_SCREEN_HEIGHT ];

/* The colours of each eight pixel chunk in the top and bottom borders */
static int top_border[DISPLAY_BORDER_HEIGHT][DISPLAY_SCREEN_WIDTH_COLS];
static int bottom_border[DISPLAY_BORDER_HEIGHT][DISPLAY_SCREEN_WIDTH_COLS];

/* And in the left and right borders */
static int left_border[DISPLAY_HEIGHT][DISPLAY_BORDER_WIDTH_COLS];
static int right_border[DISPLAY_HEIGHT][DISPLAY_BORDER_WIDTH_COLS];

/* Offsets as to where the data and the attributes for each pixel
   line start */
libspectrum_word display_line_start[ DISPLAY_HEIGHT ];
libspectrum_word display_attr_start[ DISPLAY_HEIGHT ];

/* If you write to the byte at display_dirty_?table[n+0x4000], then
   the eight pixels starting at (8*xtable[n],ytable[n]) must be
   replotted */
static libspectrum_word
  display_dirty_ytable[ DISPLAY_WIDTH_COLS * DISPLAY_HEIGHT ];
static libspectrum_word
  display_dirty_xtable[ DISPLAY_WIDTH_COLS * DISPLAY_HEIGHT ];

/* If you write to the byte at display_dirty_?table2[n+0x5800], then
   the 64 pixels starting at (8*xtable2[n],ytable2[n]) must be
   replotted */
static libspectrum_word
  display_dirty_ytable2[ DISPLAY_WIDTH_COLS * DISPLAY_HEIGHT_ROWS ];
static libspectrum_word
  display_dirty_xtable2[ DISPLAY_WIDTH_COLS * DISPLAY_HEIGHT_ROWS ];

/* The number of frames mod 32 that have elapsed.
    0<=d_f_c<16 => Flashing characters are normal
   16<=d_f_c<32 => Flashing characters are reversed
*/
static int display_frame_count;
static int display_flash_reversed;

/* Which eight-pixel chunks on each line need to be redisplayed. Bit 0
   corresponds to pixels 0-7, bit 31 to pixels 248-255. */
static libspectrum_qword display_is_dirty[ DISPLAY_SCREEN_HEIGHT ];
/* This value signifies that the entire line must be redisplayed */
static libspectrum_qword display_all_dirty;

/* Used to signify that we're redrawing the entire screen */
static int display_redraw_all;

/* The next line to be replotted */
static int display_next_line;

/* Value used to signify we're in vertical retrace */
static const int display_border_retrace=-1;

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

static void display_draw_line(int y);
static void display_dirty8( libspectrum_word address );
static void display_dirty64( libspectrum_word address );

static void display_get_attr( int x, int y,
			      libspectrum_byte *ink, libspectrum_byte *paper);

static void display_set_border(void);
static int display_border_line(void);
static int add_rectangle( int y, int x, int w );
static int end_line( int y );

static void set_border( int x, int y, libspectrum_byte colour );

static void display_dirty_flashing(void);
static int display_border_column(int time_since_line);
static void set_border_pixels( int line, int column, int colour );

libspectrum_word
display_get_addr( int x, int y )
{
  if ( scld_last_dec.name.altdfile ) {
    return display_line_start[y]+x+ALTDFILE_OFFSET;
  } else {
    return display_line_start[y]+x;
  }
}

int display_init(int *argc, char ***argv)
{
  int i,j,k,x,y;

  if(ui_init(argc, argv))
    return 1;

  /* Set up the 'all pixels must be refreshed' marker */
  display_all_dirty = 0;
  for( i = 0; i < DISPLAY_SCREEN_WIDTH_COLS; i++ )
    display_all_dirty = ( display_all_dirty << 1 ) | 0x01;

  for(i=0;i<3;i++)
    for(j=0;j<8;j++)
      for(k=0;k<8;k++)
	display_line_start[ (64*i) + (8*j) + k ] =
	  32 * ( (64*i) + j + (k*8) );

  for(y=0;y<DISPLAY_HEIGHT;y++) {
    display_attr_start[y]=6144 + (32*(y/8));
  }

  for(y=0;y<DISPLAY_HEIGHT;y++)
    for(x=0;x<DISPLAY_WIDTH_COLS;x++) {
      display_dirty_ytable[ display_line_start[y]+x ] =
	y + DISPLAY_BORDER_HEIGHT;
      display_dirty_xtable[ display_line_start[y]+x ] =
	x + DISPLAY_BORDER_WIDTH_COLS;
    }

  for(y=0;y<DISPLAY_HEIGHT_ROWS;y++)
    for(x=0;x<DISPLAY_WIDTH_COLS;x++) {
      display_dirty_ytable2[ (32*y) + x ] = ( y * 8 ) + DISPLAY_BORDER_HEIGHT;
      display_dirty_xtable2[ (32*y) + x ] = x + DISPLAY_BORDER_WIDTH_COLS;
    }

  display_frame_count=0; display_flash_reversed=0;

  for(y=0;y<DISPLAY_SCREEN_HEIGHT;y++) {
    display_current_border[y]=display_border_mixed;
    display_is_dirty[y] = display_all_dirty;
  }

  display_redraw_all = 0;

  return 0;
}

/* Draw the current screen line, and increment the line count. Called
   one more time after the entire screen has been displayed so we know
   this fact */
void display_line(void)
{
  static int frame_count = 0;
  size_t i; struct rectangle *ptr;
  int error;

  if( display_next_line < DISPLAY_SCREEN_HEIGHT ) {
    display_draw_line(display_next_line);
    event_add( machine_current->line_times[display_next_line+1],
	       EVENT_TYPE_LINE );
  } 

  /* If we're at the end of the frame, and we've got some data to
     send to the screen, send it now */
  else {

    int scale = machine_current->timex ? 2 : 1;

    /* Force all rectangles into the inactive list */
    error = end_line( display_next_line ); if( error ) return;

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

  display_next_line++;

}   

/* Redraw pixel line y if it is flagged as `dirty' */
static void
display_draw_line( int y )
{
  int start, x, border_colour, error;
  libspectrum_byte data, data2, ink, paper;
  libspectrum_word hires_data;

  x = 0;

  while( display_is_dirty[y] ) {

    /* Find the first dirty chunk on this row */
    while( ! (display_is_dirty[y] & 0x01) ) {
      display_is_dirty[y] >>= 1;
      x++;
    }

    start = x;

    /* Walk to the end of the dirty region, writing the bytes to the
       drawing area along the way */
    do {

      if( y >= DISPLAY_BORDER_HEIGHT &&
	  y < DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT ) {

	if( x >= DISPLAY_BORDER_WIDTH_COLS &&
	    x < DISPLAY_BORDER_WIDTH_COLS + DISPLAY_WIDTH_COLS ) {

	  int screen_x, screen_y;
	  libspectrum_word offset;
	  libspectrum_byte *screen;

	  screen_x = x - DISPLAY_BORDER_WIDTH_COLS;
	  screen_y = y - DISPLAY_BORDER_HEIGHT;

	  screen = RAM[ memory_current_screen ];
      
	  display_get_attr( screen_x, screen_y, &ink, &paper );

	  offset = display_get_addr( screen_x, screen_y );
	  data = screen[ offset ];

	  if( scld_last_dec.name.hires ) {
	    switch( scld_last_dec.mask.scrnmode ) {

            case HIRESATTRALTD:
	      offset =
		display_attr_start[ screen_y ] + screen_x + ALTDFILE_OFFSET;
              data2 = screen[ offset ];
              break;

            case HIRES:
              data2 = screen[ offset + ALTDFILE_OFFSET ];
              break;

            case HIRESDOUBLECOL:
              data2 = data;
              break;

            default: /* case HIRESATTR: */
	      offset = display_attr_start[ screen_y ] + screen_x;
	      data2 = screen[ offset ];
              break;

	    }
	    hires_data = (data << 8) + data2;
	    display_plot16( screen_x, screen_y, hires_data, ink, paper );
	  } else {
	    display_plot8( screen_x, screen_y, data, ink, paper );
	  }    

	} else if( x < DISPLAY_BORDER_WIDTH_COLS ) {

	  border_colour = left_border[ y - DISPLAY_BORDER_HEIGHT ][x];
	  set_border( x, y, border_colour );

	} else {

	  border_colour =
	    right_border[ y - DISPLAY_BORDER_HEIGHT ]
                        [ x - DISPLAY_BORDER_WIDTH_COLS - DISPLAY_WIDTH_COLS ];
	  set_border( x, y, border_colour );
	}

      } else if( y < DISPLAY_BORDER_HEIGHT ) {

	border_colour = top_border[y][x];
	set_border( x, y, border_colour );

      } else {

	border_colour =
	  bottom_border[y - DISPLAY_BORDER_HEIGHT - DISPLAY_HEIGHT ][x];
	set_border( x, y, border_colour );
      }

      display_is_dirty[y] >>= 1;
      x++;

    } while( display_is_dirty[y] & 0x01 );

    error = add_rectangle( y, start, x-start ); if( error ) return;

  }
  
  error = end_line( y ); if( error ) return;

  /* We've drawn the current line to the screen. Now overwrite the
     border on this line with the current border colour */
  border_colour = scld_last_dec.name.hires ? display_hires_border :
                                             display_lores_border;
  
  if( border_colour != display_current_border[y] ) {
    set_border_pixels( y, 0, border_colour );
    display_current_border[y] = border_colour;
  }

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

/* Mark as 'dirty' the pixels which have been changed by a write to
   'offset' within the RAM page containing the screen */
void
display_dirty( libspectrum_word offset )
{
  switch ( scld_last_dec.mask.scrnmode ) {

    case STANDARD: /* standard Speccy screen */
    case HIRESATTR: /* strange mode */
      if( offset >= 0x1b00 ) break;
      if( offset <  0x1800 ) {		/* 0x1800 = first attributes byte */
        display_dirty8( offset );
      } else {
        display_dirty64( offset );
      }
      break;

    case ALTDFILE: /* second screen */
    case HIRESATTRALTD: /* strange mode using second screen */      
      if( offset < 0x2000 || offset >= 0x3b00 ) break;
      if( offset < 0x3800 ) {		/* 0x3800 = first attributes byte */
        display_dirty8( offset - ALTDFILE_OFFSET );
      } else {
        display_dirty64( offset - ALTDFILE_OFFSET );
      }
      break;

    case EXTCOLOUR: /* extended colours */
    case HIRES: /* hires mode */
      if( offset >= 0x3800 ) break;
      if( offset >= 0x1800 && offset < 0x2000 ) break;
      if( offset >= 0x2000 ) offset -= ALTDFILE_OFFSET;
      display_dirty8( offset );
      break;

    default:
    /* case EXTCOLALTD: extended colours, but attributes and data
       taken from second screen */
    /* case HIRESDOUBLECOL: hires mode, but data taken only from
       second screen */
      if( offset >= 0x2000 && offset < 0x3800 )
	display_dirty8( offset - ALTDFILE_OFFSET );
      break;
  }
}

static void
display_dirty8( libspectrum_word offset )
{
  int x, y;

  x=display_dirty_xtable[ offset ];
  y=display_dirty_ytable[ offset ];

  display_is_dirty[y] |= ( (libspectrum_qword)1 << x );
  
}

static void
display_dirty64( libspectrum_word offset )
{
  int i, x, y;

  x=display_dirty_xtable2[ offset - 0x1800 ];
  y=display_dirty_ytable2[ offset - 0x1800 ];

  for( i=0; i<8; i++ ) display_is_dirty[y+i] |= ( (libspectrum_qword)1 << x );
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
  x = (x << 3) + DISPLAY_BORDER_WIDTH / 2;
  y += DISPLAY_BORDER_HEIGHT;

  if( machine_current->timex ) {

    x <<= 1; y <<= 1;
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

    y++;
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
  x = (x << 4) + DISPLAY_BORDER_WIDTH;
  y = ( y + DISPLAY_BORDER_HEIGHT ) << 1;

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

  y++;
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

static void
set_border( int x, int y, libspectrum_byte colour )
{
  size_t i;

  if( machine_current->timex ) {
    x <<= 4; y <<= 1;
    for( i = 0; i < 16; i++ ) {
      display_image[y  ][x+i] = colour;
      display_image[y+1][x+i] = colour;
    }
  } else {
    x <<= 3;
    for( i = 0; i < 8; i++ ) display_image[y][x+i] = colour;
  }

}

/* Get the attributes for the eight pixels starting at
   ( (8*x) , y ) */
static void
display_get_attr( int x, int y,
		  libspectrum_byte *ink, libspectrum_byte *paper )
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

  display_parse_attr(attr,ink,paper);
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

void display_set_lores_border(int colour)
{
  display_lores_border=colour;

  display_set_border();
}

void display_set_hires_border(int colour)
{
  display_hires_border=colour;

  display_set_border();
}

static void display_set_border(void)
{
  int current_line, time_since_line, column, colour;

  colour = scld_last_dec.name.hires ? display_hires_border : display_lores_border;
  current_line=display_border_line();

  /* Check if we're in vertical retrace; if we are, don't need to do
     change anything in the display buffer */
  if(current_line==display_border_retrace) return;

  /* If the current line is already this colour, don't need to do anything */
  if(display_current_border[current_line] == colour) return;

  time_since_line = tstates - machine_current->line_times[current_line];

  /* Now check we're not in horizonal retrace. Again, do nothing
     if we are */
  if(time_since_line >= machine_current->timings.left_border +
                        machine_current->timings.horizontal_screen +
                        machine_current->timings.right_border ) return;
      
  column = display_border_column( time_since_line );

  set_border_pixels( current_line, column, colour );

  /* Note this line has more than one colour on it */
  display_current_border[current_line]=display_border_mixed;
}

static int display_border_line(void)
{
  if( 0 < display_next_line && display_next_line <= DISPLAY_SCREEN_HEIGHT ) {
    return display_next_line-1;
  } else {
    return display_border_retrace;
  }
}

static int display_border_column(int time_since_line)
{
  int column;

  /* 2 pixels per T-state */
  column = time_since_line / 4;

  /* But now need to correct because our displayed border isn't necessarily
     the same size as the ULA's. */
  column -= 
    ( machine_current->timings.left_border >> 2 ) - DISPLAY_BORDER_WIDTH_COLS;

  if(column < 0) {
    column=0;
  } else if( column > DISPLAY_SCREEN_WIDTH_COLS ) {
    column = DISPLAY_SCREEN_WIDTH_COLS;
  }

  return column;
}

static void
set_border_pixels( int line, int column, int colour )
{
  const libspectrum_qword right_edge =
    (libspectrum_qword)1 << ( DISPLAY_BORDER_WIDTH_COLS + DISPLAY_WIDTH_COLS );

  /* See if we're in the top/bottom border */
  if( line < DISPLAY_BORDER_HEIGHT ) {

    for( ; column < DISPLAY_SCREEN_WIDTH_COLS; column++ ) {
      top_border[line][column] = colour;
      display_is_dirty[line] |= (libspectrum_qword)1 << column;
    }

  } else if( line >= DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT ) {

    for( ; column < DISPLAY_SCREEN_WIDTH_COLS; column++ ) {
      bottom_border[ line - DISPLAY_BORDER_HEIGHT - DISPLAY_HEIGHT ][column] =
	colour;
      display_is_dirty[line] |= (libspectrum_qword)1 << column;
    }

  } else {			/* In main screen */

    /* If we're in the left border, colour that bit in */
    for( ; column < DISPLAY_BORDER_WIDTH_COLS; column++ ) {
      left_border[ line - DISPLAY_BORDER_HEIGHT ][column] = colour;
      display_is_dirty[line] |= 1 << column;
    }

    /* Rebase our coordinate: zero is now the right edge of the screen */

    /* Got to the right edge of the screen if we're past it already */
    if( column <= DISPLAY_BORDER_WIDTH_COLS + DISPLAY_WIDTH_COLS ) {
      column = 0;
    } else {
      column -= DISPLAY_BORDER_WIDTH_COLS + DISPLAY_WIDTH_COLS;
    }

    /* Colour in the right border */
    for( ; column < DISPLAY_BORDER_WIDTH_COLS; column++ ) {
      right_border[ line - DISPLAY_BORDER_HEIGHT ][column] = colour;
      display_is_dirty[line] |= right_edge << column;
    }
  }
}  

int display_frame(void)
{
  display_next_line=0;
  if( event_add( machine_current->line_times[0], EVENT_TYPE_LINE) ) return 1;

  display_frame_count++;
  if(display_frame_count==16) {
    display_flash_reversed=1;
    display_dirty_flashing();
  } else if(display_frame_count==32) {
    display_flash_reversed=0;
    display_dirty_flashing();
    display_frame_count=0;
  }
  return 0;
}

static void display_dirty_flashing(void)
{
  libspectrum_word offset;
  libspectrum_byte *screen, attr;

  screen = RAM[ memory_current_screen ];
  
  if( !scld_last_dec.name.hires ) {
    if( scld_last_dec.name.b1 ) {

      for( offset = ALTDFILE_OFFSET; offset < 0x3800; offset++ ) {
        attr = screen[ offset ];
        if( attr & 0x80 ) display_dirty8( offset - ALTDFILE_OFFSET );
      }

    } else if( scld_last_dec.name.altdfile ) {

      for( offset= 0x3800; offset < 0x3b00; offset++ ) {
        attr = screen[ offset ];
        if( attr & 0x80 ) display_dirty64( offset - ALTDFILE_OFFSET );
      }

    } else { /* Standard Speccy screen */

      for( offset = 0x1800; offset < 0x1b00; offset++ ) {
	attr = screen[ offset ];
        if( attr & 0x80 ) display_dirty64( offset );
      }

    }
  }
}

void display_refresh_all(void)
{
  size_t i;

  display_redraw_all = 1;

  for( i = 0; i < DISPLAY_SCREEN_HEIGHT; i++ )
    display_is_dirty[i] = display_all_dirty;
}
