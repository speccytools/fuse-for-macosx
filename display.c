/* display.c: Routines for printing the Spectrum screen
   Copyright (c) 1999-2001 Philip Kendall and Thomas Harte

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
#include "machine.h"
#include "spectrum.h"
#include "ui.h"
#include "uidisplay.h"

/* The current border colour */
BYTE display_border;
/* The border colour current displayed on every line */
static BYTE display_current_border[DISPLAY_SCREEN_HEIGHT];

/* Offsets as to where the data and the attributes for each pixel
   line start */
static WORD display_line_start[DISPLAY_HEIGHT];
static WORD display_attr_start[DISPLAY_HEIGHT];

/* If you write to the byte at display_dirty_?table[n+0x4000], then
   the eight pixels starting at (8*xtable[n],ytable[n]) must be
   replotted */
static WORD display_dirty_ytable[(DISPLAY_WIDTH*DISPLAY_HEIGHT)/8];
static WORD display_dirty_xtable[(DISPLAY_WIDTH*DISPLAY_HEIGHT)/8];

/* If you write to the byte at display_dirty_?table2[n+0x5800], then
   the 64 pixels starting at (8*xtable2[n],ytable2[n]) must be
   replotted */
static WORD display_dirty_ytable2[ (DISPLAY_WIDTH/8) * (DISPLAY_HEIGHT/8) ];
static WORD display_dirty_xtable2[ (DISPLAY_WIDTH/8) * (DISPLAY_HEIGHT/8) ];

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
const static DWORD display_all_dirty = 0xffffffffUL;

/* Which border lines need to be redrawn */
static int display_border_dirty[DISPLAY_SCREEN_HEIGHT];

/* The next line to be replotted */
static int display_next_line;

/* Value used to signify we're in vertical retrace */
const static int display_border_retrace=-1;

/* Value used to signify a border line has more than one colour on it. */
const static int display_border_mixed = 0xff;

/* Where the current block of lines to send to the screen starts */
static int display_blocked_write_start;

/* Value to signify `no block to send to screen' */
const static int display_blocked_write_none = -1;

static void display_draw_line(int y);
static void display_dirty8(WORD address);
static void display_dirty64(WORD address);

static void display_plot8(int x, int y, BYTE data, BYTE ink, BYTE paper);
static void display_get_attr(int x, int y, BYTE *ink, BYTE *paper);
static void display_parse_attr(BYTE attr, BYTE *ink, BYTE *paper);

static int display_border_line(void);

static void display_dirty_flashing(void);
static int display_border_column(int time_since_line);

int display_init(int *argc, char ***argv)
{
  int i,j,k,x,y;

  if(ui_init(argc, argv, DISPLAY_SCREEN_WIDTH, DISPLAY_SCREEN_HEIGHT))
    return 1;

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
    for(x=0;x<(DISPLAY_WIDTH)/8;x++) {
      display_dirty_ytable[display_line_start[y]+x]= y;
      display_dirty_xtable[display_line_start[y]+x]= x;
    }

  for(y=0;y<(DISPLAY_HEIGHT/8);y++)
    for(x=0;x<(DISPLAY_WIDTH/8);x++) {
      display_dirty_ytable2[ (32*y) + x ]= ( y * 8 );
      display_dirty_xtable2[ (32*y) + x ]= x;
    }

  display_frame_count=0; display_flash_reversed=0;

  for(y=0;y<DISPLAY_SCREEN_HEIGHT;y++) {
    display_current_border[y]=display_border_mixed;
    display_border_dirty[y]=1;
  }

  display_blocked_write_start = display_blocked_write_none;

  return 0;

}

/* Draw the current screen line, and increment the line count. Called
   one more time after the entire screen has been displayed so we know
   this fact */
void display_line(void)
{
  if( display_next_line < DISPLAY_SCREEN_HEIGHT ) {
    display_draw_line(display_next_line);
    event_add( machine_current->line_times[display_next_line+1],
	       EVENT_TYPE_LINE );
  } 

  /* If we're at the end of the frame, and we've got some data to
     send to the screen, send it now */
  else if( display_blocked_write_start != display_blocked_write_none ) {
    uidisplay_lines( display_blocked_write_start, display_next_line-1 );
    display_blocked_write_start = display_blocked_write_none;
  }

  display_next_line++;

}   

/* Redraw pixel line y if it is flagged as `dirty' */

static void display_draw_line(int y)
{

  int x, screen_y, redraw;
  BYTE data, ink, paper;

  redraw = 0;
  
  /* If we're in the main screen, see if anything redrawing; if so,
     copy the data to the image buffer, and flag that we need to copy
     this line to the screen */
  if( y >= DISPLAY_BORDER_HEIGHT &&
      y < DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT ) {

    screen_y = y - DISPLAY_BORDER_HEIGHT;
      
    if( display_is_dirty[screen_y] ) {

      for( x=0;
	   display_is_dirty[screen_y];
	   display_is_dirty[screen_y] >>= 1, x++ ) {

	/* Skip to next 8 pixel chunk if this chunk is clean */
	if( ! ( display_is_dirty[screen_y] & 0x01 ) ) continue;

	data = read_screen_memory( display_line_start[screen_y]+x );
	display_get_attr( x, screen_y, &ink, &paper );

	display_plot8( x, screen_y, data, ink, paper );

      }

      /* Need to redraw this line */
      redraw = 1;

    }

  }

  /* We need to redraw this line if the main screen or border has
     been changed since we were last here... */
  if( redraw || display_border_dirty[y] ) {

    /* If we're haven't currently got a block, note this line as the
       start; if we have currently got a block, just carry on until we
       find a line we don't want to copy to the screen. */
    if( display_blocked_write_start == display_blocked_write_none ) {
      display_blocked_write_start = y;
    }

    display_border_dirty[y]=0;

  }
  /* If we're _don't_ need to redraw this line, copy any block with
     exists to the screen */
  else if( display_blocked_write_start != display_blocked_write_none ) {
    uidisplay_lines( display_blocked_write_start, y-1 );
    display_blocked_write_start = display_blocked_write_none;
  }
  
  if(display_current_border[y] != display_border) {

    /* See if we're in the top/bottom border */
    if(y < DISPLAY_BORDER_HEIGHT ||
       y >= DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT ) {

      /* Colour in all the border to the very right edge */
      uidisplay_set_border(y, 0, DISPLAY_SCREEN_WIDTH, display_border);

    } else {			/* In main screen */

      /* Colour in the left and right borders */
      uidisplay_set_border(y, 0, DISPLAY_BORDER_WIDTH, display_border);
      uidisplay_set_border(y, DISPLAY_BORDER_WIDTH + DISPLAY_WIDTH,
	  	           DISPLAY_SCREEN_WIDTH, display_border);
    }

    display_current_border[y] = display_border;
    display_border_dirty[y]=1;	/* Need to redisplay this line next time */

  }

}

/* Mark as `dirty' the pixels which have been changed by a write to byte
   `address'; 0x4000 <= address < 0x5b00 */
void display_dirty(WORD address, BYTE data)
{
  if(address<0x5800) {		/* 0x5800 = first attributes byte */
    display_dirty8(address);
  } else {
    display_dirty64(address);
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
static void display_plot8(int x, int y, BYTE data, BYTE ink, BYTE paper)
{

  x = (x << 3) + DISPLAY_BORDER_WIDTH;
  y += DISPLAY_BORDER_HEIGHT;
  uidisplay_putpixel(x+0,y, ( data & 0x80 ) ? ink : paper );
  uidisplay_putpixel(x+1,y, ( data & 0x40 ) ? ink : paper );
  uidisplay_putpixel(x+2,y, ( data & 0x20 ) ? ink : paper );
  uidisplay_putpixel(x+3,y, ( data & 0x10 ) ? ink : paper );
  uidisplay_putpixel(x+4,y, ( data & 0x08 ) ? ink : paper );
  uidisplay_putpixel(x+5,y, ( data & 0x04 ) ? ink : paper );
  uidisplay_putpixel(x+6,y, ( data & 0x02 ) ? ink : paper );
  uidisplay_putpixel(x+7,y, ( data & 0x01 ) ? ink : paper );
}

/* Get the attributes for the eight pixels starting at
   ( (8*x) , y ) */
static void display_get_attr(int x,int y,BYTE *ink,BYTE *paper)
{
  BYTE attr=read_screen_memory(display_attr_start[y]+x);
  display_parse_attr(attr,ink,paper);
}

static void display_parse_attr(BYTE attr,BYTE *ink,BYTE *paper)
{
  if( (attr & 0x80) && display_flash_reversed ) {
    *ink  = (attr & ( 0x0f << 3 ) ) >> 3;
    *paper= (attr & 0x07) + ( (attr & 0x40) >> 3 );
  } else {
    *ink= (attr & 0x07) + ( (attr & 0x40) >> 3 );
    *paper= (attr & ( 0x0f << 3 ) ) >> 3;
  }
}

void display_set_border(int colour)
{
  int current_line,time_since_line,current_pixel;

  display_border=colour;

  current_line=display_border_line();

  /* Check if we're in vertical retrace; if we are, don't need to do
     change anything in the display buffer */
  if(current_line==display_border_retrace) return;

  /* If the current line is already this colour, don't need to do anything */
  if(display_current_border[current_line] == colour) return;

  time_since_line = tstates - machine_current->line_times[current_line];

  /* Now check we're not in horizonal retrace. Again, do nothing
     if we are */
  if(time_since_line >= machine_current->timings.left_border_cycles +
                        machine_current->timings.screen_cycles +
                        machine_current->timings.right_border_cycles ) return;
      
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
  column -= ( 2*machine_current->timings.left_border_cycles -
	      DISPLAY_BORDER_WIDTH );
  if(column < 0) {
    column=0;
  } else if(column > DISPLAY_SCREEN_WIDTH) {
    column=DISPLAY_SCREEN_WIDTH;
  }

  return column;
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

  for(offset=0x1800;offset<0x1b00;offset++) {
    attr=read_screen_memory(offset);
    if( attr & 0x80 )
      display_dirty64(offset+0x4000);
  }
}

void display_refresh_all(void)
{
  int x,y,z; BYTE ink,paper;

  for(y=0;y<DISPLAY_HEIGHT;y+=8) {
    for(x=0;x<(DISPLAY_WIDTH/8);x++) {
      display_parse_attr(read_screen_memory(display_attr_start[y]+x),
			 &ink,&paper);
      for(z=0;z<8;z++) {
	display_plot8(x,y+z,read_screen_memory(display_line_start[y+z]+x),
		      ink,paper);
	display_is_dirty[y+z] = ~0;	/* Marks all pixels as dirty */
      }
    }
  }
}
