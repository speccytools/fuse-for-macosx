/* display.c: Routines for printing the Spectrum screen
   Copyright (c) 1999-2000 Philip Kendall and Thomas Harte

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

   E-mail: pak@ast.cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include "display.h"
#include "event.h"
#include "spectrum.h"

#ifdef HAVE_LIBGTK
#include "gtkdisplay.h"
#include "gtkui.h"
#else				/* #ifdef HAVE_LIBGTK */
#include "xdisplay.h"
#include "xui.h"
#endif				/* #ifdef HAVE_LIBGTK */

/* The current border colour */
BYTE display_border;
/* The border colour current displayed on every line */
static BYTE display_current_border[DISPLAY_SCREEN_HEIGHT];

/* Offsets as to where the data and the attributes for each pixel
   line start */
static WORD display_line_start[DISPLAY_HEIGHT];
static WORD display_attr_start[DISPLAY_HEIGHT];

/* If you write to the byte at display_dirty_?table[n+0x4000], then
   the eight pixels starting at (xtable[n],ytable[n]) must be
   replotted */
static WORD display_dirty_ytable[(DISPLAY_WIDTH*DISPLAY_HEIGHT)/8];
static WORD display_dirty_xtable[(DISPLAY_WIDTH*DISPLAY_HEIGHT)/8];

/* If you write to the byte at display_dirty_?table2[n+0x5800], then
   the 64 pixels starting at (xtable2[n],ytable2[n]) must be
   replotted */
static WORD display_dirty_ytable2[ (DISPLAY_WIDTH/8) * (DISPLAY_HEIGHT/8) ];
static WORD display_dirty_xtable2[ (DISPLAY_WIDTH/8) * (DISPLAY_HEIGHT/8) ];

/* The number of frames mod 32 that have elapsed.
    0<=d_f_c<16 => Flashing characters are normal
   16<=d_f_c<32 => Flashing characters are reversed
*/
static BYTE display_frame_count;
static int display_flash_reversed;

/* Does this line need to be redisplayed? */
static BYTE display_is_dirty[DISPLAY_SCREEN_HEIGHT];

/* The next line to be replotted */
static int display_next_line;

/* Value used to signify we're in vertical retrace */
const static int display_border_retrace=-1;

/* Value used to signify a border line has more than one colour on it. */
const static int display_border_mixed = 0xff;

static void display_draw_line(int y);
static void display_dirty8(WORD address, BYTE data);
static void display_dirty64(WORD address, BYTE attr);

static void display_plot8(int x, int y, BYTE data, BYTE ink, BYTE paper);
static void display_get_attr(int x, int y, BYTE *ink, BYTE *paper);
static void display_parse_attr(BYTE attr, BYTE *ink, BYTE *paper);

static int display_border_line(void);

static void display_dirty_flashing(void);
static int display_border_column(int time_since_line);

int display_init(int *argc, char ***argv)
{
  int i,j,k,x,y;

#ifdef HAVE_LIBGTK
  if(gtkui_init(argc,argv,DISPLAY_SCREEN_WIDTH,DISPLAY_SCREEN_HEIGHT))
    return 1;
#else			/* #ifdef HAVE_LIBGTK */
  if( xui_init(*argc, *argv, DISPLAY_SCREEN_WIDTH, DISPLAY_SCREEN_HEIGHT) )
    return 1;
#endif			/* #ifdef HAVE_LIBGTK */

  for(i=0;i<3;i++)
    for(j=0;j<8;j++)
      for(k=0;k<8;k++)
	display_line_start[ (64*i) + (8*j) + k ] =
	  32 * ( (64*i) + j + (k*8) );

  for(y=0;y<DISPLAY_HEIGHT;y++)
    display_attr_start[y]=6144 + (32*(y/8));

  for(y=0;y<DISPLAY_HEIGHT;y++)
    for(x=0;x<(DISPLAY_WIDTH)/8;x++) {
      display_dirty_ytable[display_line_start[y]+x]=y;
      display_dirty_xtable[display_line_start[y]+x]=x;
    }

  for(y=0;y<(DISPLAY_HEIGHT/8);y++)
    for(x=0;x<(DISPLAY_WIDTH/8);x++) {
      display_dirty_ytable2[ (32*y) + x ]=8*y;
      display_dirty_xtable2[ (32*y) + x ]=x;
    }

  display_frame_count=0; display_flash_reversed=0;

  for(y=0;y<DISPLAY_SCREEN_HEIGHT;y++)
    display_current_border[y]=display_border_mixed;

  return 0;

}

/* Draw the current screen line, and increment the line count. Called
   one more time after the entire screen has been displayed so we know
   this fact */
void display_line(void)
{
  if( display_next_line < DISPLAY_SCREEN_HEIGHT ) {
    display_draw_line(display_next_line);
    event_add(machine.line_times[display_next_line+1],EVENT_TYPE_LINE);
  }
  display_next_line++;
}   

/* Redraw pixel line y if it is flagged as `dirty' */
static void display_draw_line(int y)
{
  if(display_is_dirty[y]) {
#ifdef HAVE_LIBGTK
    gtkdisplay_line(y);
#else			/* #ifdef HAVE_LIBGTK */
    xdisplay_line(y);
#endif			/* #ifdef HAVE_LIBGTK */
    display_is_dirty[y]=0;
  }

  if(display_current_border[y] != display_border) {

    /* See if we're in the top/bottom border */
    if(y < DISPLAY_BORDER_HEIGHT ||
       y >= DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT ) {

      /* Colour in all the border to the very right edge */
#ifdef HAVE_LIBGTK
      gtkdisplay_set_border(y, 0, DISPLAY_SCREEN_WIDTH, display_border);
#else			/* #ifdef HAVE_LIBGTK */
      xdisplay_set_border(y, 0, DISPLAY_SCREEN_WIDTH, display_border);
#endif			/* #ifdef HAVE_LIBGTK */

    } else {			/* In main screen */

      /* Colour in the left and right borders */
#ifdef HAVE_LIBGTK
      gtkdisplay_set_border(y, 0, DISPLAY_BORDER_WIDTH, display_border);
      gtkdisplay_set_border(y, DISPLAY_BORDER_WIDTH + DISPLAY_WIDTH,
			    DISPLAY_SCREEN_WIDTH, display_border);
#else			/* #ifdef HAVE_LIBGTK */
      xdisplay_set_border(y, 0, DISPLAY_BORDER_WIDTH, display_border);
      xdisplay_set_border(y, DISPLAY_BORDER_WIDTH + DISPLAY_WIDTH,
			  DISPLAY_SCREEN_WIDTH, display_border);
#endif			/* #ifdef HAVE_LIBGTK */

    }

    display_current_border[y] = display_border;
    display_is_dirty[y]=1;	/* Need to redisplay this line next time */

  }

}

/* Mark as `dirty' the pixels which have been changed by a write to byte
   `address'; 0x4000 <= address < 0x5b00 */
void display_dirty(WORD address, BYTE data)
{
  if(address<0x5800) {		/* 0x5800 = first attributes byte */
    display_dirty8(address,data);
  } else {
    display_dirty64(address,data);
  }
}

static void display_dirty8(WORD address, BYTE data)
{
  int x,y; BYTE ink,paper;

  x=display_dirty_xtable[address-0x4000];
  y=display_dirty_ytable[address-0x4000];
  display_get_attr(x,y,&ink,&paper);

  display_plot8(x,y,data,ink,paper);

  display_is_dirty[DISPLAY_BORDER_HEIGHT+y] = 1;
  
}

static void display_dirty64(WORD address, BYTE attr)
{
  int i,x,y; BYTE data,ink,paper;

  x=display_dirty_xtable2[address-0x5800];
  y=display_dirty_ytable2[address-0x5800];
  display_parse_attr(attr,&ink,&paper);

  for(i=0;i<8;i++) {
    data=read_screen_memory(display_line_start[y+i]+x);
    display_plot8(x,y+i,data,ink,paper);
  }

  memset(&display_is_dirty[DISPLAY_BORDER_HEIGHT+y],1,8*sizeof(BYTE));

}

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y ) */
static void display_plot8(int x,int y,BYTE data,BYTE ink,BYTE paper)
{
  x = (8*x) + DISPLAY_BORDER_WIDTH;
  y += DISPLAY_BORDER_HEIGHT;
#ifdef HAVE_LIBGTK
  gtkdisplay_putpixel(x+0,y, ( data & 0x80 ) ? ink : paper );
  gtkdisplay_putpixel(x+1,y, ( data & 0x40 ) ? ink : paper );
  gtkdisplay_putpixel(x+2,y, ( data & 0x20 ) ? ink : paper );
  gtkdisplay_putpixel(x+3,y, ( data & 0x10 ) ? ink : paper );
  gtkdisplay_putpixel(x+4,y, ( data & 0x08 ) ? ink : paper );
  gtkdisplay_putpixel(x+5,y, ( data & 0x04 ) ? ink : paper );
  gtkdisplay_putpixel(x+6,y, ( data & 0x02 ) ? ink : paper );
  gtkdisplay_putpixel(x+7,y, ( data & 0x01 ) ? ink : paper );
#else			/* #ifdef HAVE_LIBGTK */
  xdisplay_putpixel(x+0,y, ( data & 0x80 ) ? ink : paper );
  xdisplay_putpixel(x+1,y, ( data & 0x40 ) ? ink : paper );
  xdisplay_putpixel(x+2,y, ( data & 0x20 ) ? ink : paper );
  xdisplay_putpixel(x+3,y, ( data & 0x10 ) ? ink : paper );
  xdisplay_putpixel(x+4,y, ( data & 0x08 ) ? ink : paper );
  xdisplay_putpixel(x+5,y, ( data & 0x04 ) ? ink : paper );
  xdisplay_putpixel(x+6,y, ( data & 0x02 ) ? ink : paper );
  xdisplay_putpixel(x+7,y, ( data & 0x01 ) ? ink : paper );
#endif			/* #ifdef HAVE_LIBGTK */
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

  time_since_line = tstates - machine.line_times[current_line];

  /* Now check we're not in horizonal retrace. Again, do nothing
     if we are */
  if(time_since_line >= machine.left_border_cycles + machine.screen_cycles +
     machine.right_border_cycles ) return;
      
  current_pixel=display_border_column(time_since_line);

  /* See if we're in the top/bottom border */
  if(current_line < DISPLAY_BORDER_HEIGHT ||
     current_line >= DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT ) {

    /* Colour in all the border to the very right edge */
#ifdef HAVE_LIBGTK
    gtkdisplay_set_border(current_line, current_pixel, DISPLAY_SCREEN_WIDTH,
			  colour);
#else			/* #ifdef HAVE_LIBGTK */
    xdisplay_set_border(current_line, current_pixel, DISPLAY_SCREEN_WIDTH,
			colour);
#endif			/* #ifdef HAVE_LIBGTK */

  } else {			/* In main screen */

    /* If we're in the left border, colour that bit in */
    if(current_pixel < DISPLAY_BORDER_WIDTH)
#ifdef HAVE_LIBGTK
      gtkdisplay_set_border(current_line, current_pixel, DISPLAY_BORDER_WIDTH,
			    colour);
#else			/* #ifdef HAVE_LIBGTK */
      xdisplay_set_border(current_line, current_pixel, DISPLAY_BORDER_WIDTH,
			  colour);
#endif			/* #ifdef HAVE_LIBGTK */

    /* Advance to the right edge of the screen */
    if(current_pixel < DISPLAY_BORDER_WIDTH + DISPLAY_WIDTH)
      current_pixel = DISPLAY_BORDER_WIDTH + DISPLAY_WIDTH;

    /* Draw the right border */
#ifdef HAVE_LIBGTK
    gtkdisplay_set_border(current_line, current_pixel, DISPLAY_SCREEN_WIDTH,
			  colour);
#else			/* #ifdef HAVE_LIBGTK */
    xdisplay_set_border(current_line, current_pixel, DISPLAY_SCREEN_WIDTH,
			colour);
#endif			/* #ifdef HAVE_LIBGTK */

  }

  /* And note this line has more than one colour on it */
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

  /* 2 pixels per T-state, rounded _up_ to the next multiple of eight */
  column = ( (time_since_line+3)/4 ) * 8;

  /* But now need to correct because our displayed border isn't necessarily
     the same size as the ULA's. */
  column -= ( 2*machine.left_border_cycles - DISPLAY_BORDER_WIDTH );
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
  if(event_add(machine.line_times[0],EVENT_TYPE_LINE)) return 1;
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
      display_dirty64(offset+0x4000,attr);
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
	display_is_dirty[DISPLAY_BORDER_HEIGHT+y+z]=1;
      }
    }
  }
}

int display_end(void)
{
  int error;

#ifdef HAVE_LIBGTK
  if( (error=gtkdisplay_end()) != 0 ) return error;
#else			/* #ifdef HAVE_LIBGTK */
  if( (error=xdisplay_end()) != 0 ) return error;
#endif			/* #ifdef HAVE_LIBGTK */

  return 0;
}
