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

/* The current border colour */
BYTE display_lores_border;
BYTE display_hires_border;
/* The border colour current displayed on every line */
static BYTE display_current_border[DISPLAY_SCREEN_HEIGHT];

/* Offsets as to where the data and the attributes for each pixel
   line start */
WORD display_line_start[DISPLAY_HEIGHT];
WORD display_attr_start[DISPLAY_HEIGHT];

/* If you write to the byte at display_dirty_?table[n+0x4000], then
   the eight pixels starting at (8*xtable[n],ytable[n]) must be
   replotted */
static WORD display_dirty_ytable[DISPLAY_WIDTH_COLS*DISPLAY_HEIGHT];
static WORD display_dirty_xtable[DISPLAY_WIDTH_COLS*DISPLAY_HEIGHT];

/* If you write to the byte at display_dirty_?table2[n+0x5800], then
   the 64 pixels starting at (8*xtable2[n],ytable2[n]) must be
   replotted */
static WORD display_dirty_ytable2[ DISPLAY_WIDTH_COLS * DISPLAY_HEIGHT_ROWS ];
static WORD display_dirty_xtable2[ DISPLAY_WIDTH_COLS * DISPLAY_HEIGHT_ROWS ];

/* The number of frames mod 32 that have elapsed.
    0<=d_f_c<16 => Flashing characters are normal
   16<=d_f_c<32 => Flashing characters are reversed
*/
static int display_frame_count;
static int display_flash_reversed;

/* Which eight-pixel chunks on each line need to be redisplayed. Bit 0
   corresponds to pixels 0-7, bit 31 to pixels 248-255. */
static DWORD display_is_dirty[DISPLAY_HEIGHT];
/* This value signifies that the entire line must be redisplayed */
static const DWORD display_all_dirty = ~0;

/* Which border lines need to be redrawn */
static int display_border_dirty[DISPLAY_SCREEN_HEIGHT];

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
static void display_dirty8(WORD address);
static void display_dirty64(WORD address);

static void display_get_attr(int x, int y, BYTE *ink, BYTE *paper);

static void display_set_border(void);
static int display_border_line(void);
static int add_rectangle( int y, int x, int w );
static int end_line( int y );

static void display_dirty_flashing(void);
static int display_border_column(int time_since_line);

WORD
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

  if(ui_init(argc, argv, DISPLAY_ASPECT_WIDTH, DISPLAY_SCREEN_HEIGHT))
    return 1;

  /* We can now output error messages to our output device */
  display_ui_initialised = 1;

  for(i=0;i<3;i++)
    for(j=0;j<8;j++)
      for(k=0;k<8;k++)
	display_line_start[ (64*i) + (8*j) + k ] =
	  32 * ( (64*i) + j + (k*8) );

  for(y=0;y<DISPLAY_HEIGHT;y++) {
    display_attr_start[y]=6144 + (32*(y/8));
    display_is_dirty[y]=display_all_dirty;

    display_current_border[y]=display_border_mixed;
    display_border_dirty[y]=1;
  }

  for(y=0;y<DISPLAY_HEIGHT;y++)
    for(x=0;x<DISPLAY_WIDTH_COLS;x++) {
      display_dirty_ytable[display_line_start[y]+x]= y;
      display_dirty_xtable[display_line_start[y]+x]= x;
    }

  for(y=0;y<DISPLAY_HEIGHT_ROWS;y++)
    for(x=0;x<DISPLAY_WIDTH_COLS;x++) {
      display_dirty_ytable2[ (32*y) + x ]= ( y * 8 );
      display_dirty_xtable2[ (32*y) + x ]= x;
    }

  display_frame_count=0; display_flash_reversed=0;

  for(y=0;y<DISPLAY_SCREEN_HEIGHT;y++) {
    display_current_border[y]=display_border_mixed;
    display_border_dirty[y]=1;
  }

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

    /* Force all rectangles into the inactive list */
    error = end_line( display_next_line ); if( error ) return;

    if( settings_current.frame_rate <= ++frame_count ) {
      frame_count = 0;

      for( i = 0, ptr = inactive_rectangle;
	   i < inactive_rectangle_count;
	   i++, ptr++ ) {
	printf( "%d %d %d %d\n", ptr->x, ptr->y, ptr->w, ptr->h );
	uidisplay_area( 8 * ptr->x, ptr->y + DISPLAY_BORDER_HEIGHT,
			8 * ptr->w, ptr->h );
      }
      printf( "\n" );

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
  int start, x, screen_y, redraw, colour, error;
  BYTE data, data2, ink, paper;
  WORD hires_data;

  redraw = 0;
  colour = scld_last_dec.name.hires ? display_hires_border :
                                      display_lores_border;
  
  /* If we're in the main screen, see if anything redrawing; if so,
     copy the data to the image buffer, and flag that we need to copy
     this line to the screen */
  if( y >= DISPLAY_BORDER_HEIGHT &&
      y < DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT ) {

    screen_y = y - DISPLAY_BORDER_HEIGHT;

    x = 0;

    while( display_is_dirty[screen_y] ) {

      /* Find the first dirty chunk on this row */
      while( ! (display_is_dirty[screen_y] & 0x01) ) {
	display_is_dirty[screen_y] >>= 1;
	x++;
      }

      start = x;

      /* Walk to the end of the dirty region, writing the bytes to the
         drawing area along the way */
      do {
      
        display_get_attr( x, screen_y, &ink, &paper );
        data = read_screen_memory( display_get_addr( x, screen_y ) );
        if ( scld_last_dec.name.hires ) {
          switch ( scld_last_dec.mask.scrnmode ) {
            case HIRESATTRALTD:
              data2 = read_screen_memory( display_attr_start[screen_y] + x + ALTDFILE_OFFSET );
              break;
            case HIRES:
              data2 = read_screen_memory( display_get_addr( x, screen_y ) + ALTDFILE_OFFSET );
              break;
            case HIRESDOUBLECOL:
              data2 = data;
              break;
            default: /* case HIRESATTR: */
              data2 = read_screen_memory( display_attr_start[screen_y] + x );
              break;
          }
          hires_data = (data << 8) + data2;
          display_plot16( x<<1, screen_y, hires_data, ink, paper );    
        } else {
          display_plot8( x<<1, screen_y, data, ink, paper );
        }    

	display_is_dirty[screen_y] >>= 1;
	x++;

      } while( display_is_dirty[screen_y] & 0x01 );

      error = add_rectangle( screen_y, start, x-start ); if( error ) return;
    }

    error = end_line( screen_y ); if( error ) return;

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

/* Move all rectangles not updated on this line to the inactive list */
static int
end_line( int y )
{
  size_t i;
  struct rectangle *ptr;

  for( i = 0; i < active_rectangle_count; i++ ) {

    /* Skip if this rectangle was updated this line */
    if( active_rectangle[i].y + active_rectangle[i].h == y + 1 ) continue;

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

/* Mark as `dirty' the pixels which have been changed by a write to byte
   `address'; 0x4000 <= address < 0x5b00 */
void display_dirty( WORD address )
{
  switch ( scld_last_dec.mask.scrnmode ) {
    case STANDARD: /* standard Speccy screen */
    case HIRESATTR: /* strange mode */
      if(address>=0x5b00)
        return;
      if(address<0x5800) {		/* 0x5800 = first attributes byte */
        display_dirty8(address);
      } else {
        display_dirty64(address);
      }
      break;

    case ALTDFILE: /* second screen */
    case HIRESATTRALTD: /* strange mode using second screen */      
      if( address >= 0x7b00 )
        return;
      if( address >= 0x6000 && address < 0x7800 ) {		/* 0x7800 = first attributes byte */
        display_dirty8(address-ALTDFILE_OFFSET);
      } else {
        display_dirty64(address-ALTDFILE_OFFSET);
      }
      break;

    case EXTCOLOUR: /* extended colours */
    case HIRES: /* hires mode */     
      if((address>=0x7800) || ((address>=0x5800) && (address<0x6000)))
        return;
      if(address>=0x6000) address-=ALTDFILE_OFFSET;
      display_dirty8(address);
      break;

    default:
    /* case EXTCOLALTD: extended colours, but attributes and data taken from second screen */
    /* case HIRESDOUBLECOL: hires mode, but data taken only from second screen */
      if( address >= 0x6000 && address < 0x7800 ) {		/* 0x7800 = first attributes byte */
        display_dirty8(address-ALTDFILE_OFFSET);
      }
      break;
  }
}

static void display_dirty8(WORD address)
{
  int x, y;

  x=display_dirty_xtable[address-0x4000];
  y=display_dirty_ytable[address-0x4000];

  display_is_dirty[y] |= ( 1UL << x );
  
}

static void display_dirty64(WORD address)
{
  int i, x, y;

  x=display_dirty_xtable2[address-0x5800];
  y=display_dirty_ytable2[address-0x5800];

  for( i=0; i<8; i++ ) display_is_dirty[y+i] |= ( 1UL << x );
}

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y ) */

void display_plot8(int x, int y, BYTE data, BYTE ink, BYTE paper)
{
  x = (x << 3) + DISPLAY_BORDER_WIDTH;
  y += DISPLAY_BORDER_HEIGHT;
  uidisplay_putpixel(x+0,y, ( data & 0x80 ) ? ink : paper );
  uidisplay_putpixel(x+1,y, ( data & 0x80 ) ? ink : paper );
  uidisplay_putpixel(x+2,y, ( data & 0x40 ) ? ink : paper );
  uidisplay_putpixel(x+3,y, ( data & 0x40 ) ? ink : paper );
  uidisplay_putpixel(x+4,y, ( data & 0x20 ) ? ink : paper );
  uidisplay_putpixel(x+5,y, ( data & 0x20 ) ? ink : paper );
  uidisplay_putpixel(x+6,y, ( data & 0x10 ) ? ink : paper );
  uidisplay_putpixel(x+7,y, ( data & 0x10 ) ? ink : paper );
  uidisplay_putpixel(x+8,y, ( data & 0x08 ) ? ink : paper );
  uidisplay_putpixel(x+9,y, ( data & 0x08 ) ? ink : paper );
  uidisplay_putpixel(x+10,y, ( data & 0x04 ) ? ink : paper );
  uidisplay_putpixel(x+11,y, ( data & 0x04 ) ? ink : paper );
  uidisplay_putpixel(x+12,y, ( data & 0x02 ) ? ink : paper );
  uidisplay_putpixel(x+13,y, ( data & 0x02 ) ? ink : paper );
  uidisplay_putpixel(x+14,y, ( data & 0x01 ) ? ink : paper );
  uidisplay_putpixel(x+15,y, ( data & 0x01 ) ? ink : paper );
}

/* Print the 16 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (16*x) , y ) */

void display_plot16(int x, int y, WORD data, BYTE ink, BYTE paper)
{
  x = (x << 3) + DISPLAY_BORDER_WIDTH;
  y += DISPLAY_BORDER_HEIGHT;
  uidisplay_putpixel(x+0, y, ( data & 0x8000 ) ? ink : paper );
  uidisplay_putpixel(x+1, y, ( data & 0x4000 ) ? ink : paper );
  uidisplay_putpixel(x+2, y, ( data & 0x2000 ) ? ink : paper );
  uidisplay_putpixel(x+3, y, ( data & 0x1000 ) ? ink : paper );
  uidisplay_putpixel(x+4, y, ( data & 0x0800 ) ? ink : paper );
  uidisplay_putpixel(x+5, y, ( data & 0x0400 ) ? ink : paper );
  uidisplay_putpixel(x+6, y, ( data & 0x0200 ) ? ink : paper );
  uidisplay_putpixel(x+7 ,y, ( data & 0x0100 ) ? ink : paper );
  uidisplay_putpixel(x+8 ,y, ( data & 0x0080 ) ? ink : paper );
  uidisplay_putpixel(x+9 ,y, ( data & 0x0040 ) ? ink : paper );
  uidisplay_putpixel(x+10,y, ( data & 0x0020 ) ? ink : paper );
  uidisplay_putpixel(x+11,y, ( data & 0x0010 ) ? ink : paper );
  uidisplay_putpixel(x+12,y, ( data & 0x0008 ) ? ink : paper );
  uidisplay_putpixel(x+13,y, ( data & 0x0004 ) ? ink : paper );
  uidisplay_putpixel(x+14,y, ( data & 0x0002 ) ? ink : paper );
  uidisplay_putpixel(x+15,y, ( data & 0x0001 ) ? ink : paper );
}

/* Get the attributes for the eight pixels starting at
   ( (8*x) , y ) */
static void display_get_attr(int x,int y,BYTE *ink,BYTE *paper)
{
  BYTE attr;

  if ( scld_last_dec.name.hires ) {
    attr = hires_get_attr();
  } else {
    if ( scld_last_dec.name.b1 ) {
      attr = read_screen_memory(display_line_start[y]+x+ALTDFILE_OFFSET);
    } else if ( scld_last_dec.name.altdfile ) {
      attr = read_screen_memory(display_attr_start[y]+x+ALTDFILE_OFFSET);
    } else attr = read_screen_memory(display_attr_start[y]+x);
  }

  display_parse_attr(attr,ink,paper);
}

void display_parse_attr(BYTE attr, BYTE *ink, BYTE *paper)
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
  int current_line,time_since_line,current_pixel,colour;

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
      
  current_pixel=display_border_column(time_since_line);

  /* See if we're in the top/bottom border */
  if(current_line < DISPLAY_BORDER_HEIGHT ||
     current_line >= DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT ) {

    /* Colour in all the border to the very right edge */
    uidisplay_set_border(current_line, current_pixel, DISPLAY_SCREEN_WIDTH,
	         	 colour);

  } else {			/* In main screen */

    /* If we're in the left border, colour that bit in */
    if(current_pixel < DISPLAY_BORDER_WIDTH)
      uidisplay_set_border(current_line, current_pixel, DISPLAY_BORDER_WIDTH,
			   colour);

    /* Advance to the right edge of the screen */
    if(current_pixel < DISPLAY_BORDER_WIDTH + DISPLAY_WIDTH)
      current_pixel = DISPLAY_BORDER_WIDTH + DISPLAY_WIDTH;

    /* Draw the right border */
    uidisplay_set_border(current_line, current_pixel, DISPLAY_SCREEN_WIDTH,
			 colour);

  }

  /* Note this line has more than one colour on it */
  display_current_border[current_line]=display_border_mixed;

  /* And that it needs to be copied to the display */
  display_border_dirty[current_line] = 1;

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

  /* 2 pixels per T-state, rounded _up_ to the next multiple of eight */
  column = ( (time_since_line+3)/4 ) * 8;

  /* But now need to correct because our displayed border isn't necessarily
     the same size as the ULA's. */
  column -= ( machine_current->timings.left_border << 1 ) -
    DISPLAY_BORDER_ASPECT_WIDTH;

  if(column < 0) {
    column=0;
  } else if(column > DISPLAY_ASPECT_WIDTH) {
    column=DISPLAY_ASPECT_WIDTH;
  }

  return column<<1;
}

int display_dirty_border( void )
{
  int y;

  for( y=0; y<DISPLAY_SCREEN_HEIGHT; y++ ) {
    display_current_border[y] = display_border_mixed;
    display_border_dirty[y] = 1;
  }

  return 0;
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
  int offset; BYTE attr;
  
  if ( !scld_last_dec.name.hires ) {
    if ( scld_last_dec.name.b1 ) {
      for(offset=ALTDFILE_OFFSET;offset<0x3800;offset++) {
        attr=read_screen_memory(offset);
        if( attr & 0x80 )
          display_dirty8(offset+ALTDFILE_OFFSET);
      }
    } else if ( scld_last_dec.name.altdfile ) {
      for(offset=0x3800;offset<0x3b00;offset++) {
        attr=read_screen_memory(offset);
        if( attr & 0x80 )
          display_dirty64(offset+ALTDFILE_OFFSET);
      }
    } else { /* Standard Speccy screen */
      for(offset=0x1800;offset<0x1b00;offset++) {
        attr=read_screen_memory(offset);
        if( attr & 0x80 )
          display_dirty64(offset+0x4000);
      }
    }
  }
}

void display_refresh_all(void)
{
  int x,y; BYTE ink,paper;
  WORD hires_data;

  for(y=0;y<DISPLAY_SCREEN_HEIGHT;y++) {
    display_border_dirty[y] = 1;
  }

  for(y=0;y<DISPLAY_HEIGHT;y++) {
    for(x=0;x<DISPLAY_WIDTH_COLS;x++) {
      display_get_attr(x,y,&ink,&paper);
	if( scld_last_dec.name.hires ) {
	  hires_data = (read_screen_memory( display_get_addr(x,y) ) << 8 ) +
	    read_screen_memory( display_get_addr( x, y ) + ALTDFILE_OFFSET );
	  display_plot16( x<<1, y, hires_data, ink, paper );
	} else
	  display_plot8( x<<1, y, read_screen_memory( display_get_addr(x,y) ),
			 ink, paper );
    }
    display_is_dirty[y] = display_all_dirty;	/* Marks all pixels as dirty */
  }
}

void
display_refresh_border( void )
{
  int y;
  int colour = scld_last_dec.name.hires ? display_hires_border : display_lores_border;

  /* Redraw the top and bottom borders */
  for( y = 0; y < DISPLAY_BORDER_HEIGHT; y++ ) {
    uidisplay_set_border( y, 0, DISPLAY_SCREEN_WIDTH, colour );    
    uidisplay_set_border( DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT + y, 0,
                          DISPLAY_SCREEN_WIDTH, colour );
  }

  /* And the bits to the left and right of the main screen */
  for( y = DISPLAY_BORDER_HEIGHT;
       y < DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT;
       y++ ) {
    uidisplay_set_border( y, 0, DISPLAY_BORDER_WIDTH, colour );
    uidisplay_set_border( y, DISPLAY_BORDER_WIDTH + DISPLAY_WIDTH,
                          DISPLAY_SCREEN_WIDTH, colour );
  }
}
