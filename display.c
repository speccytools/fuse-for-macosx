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

#include <stdio.h>

#include "display.h"
#include "spectrum.h"
#include "xdisplay.h"

/* Width and height of the emulated border */
int display_border_width=32;
int display_border_height=24;

/* Palette entry to use for the Spectrum border colour */
static const int DISPLAY_BORDER=16;
/* The current border colour */
int display_border;

/* Offsets as to where the data and the attributes for each pixel
   line start */
static WORD display_line_start[192];
static WORD display_attr_start[192];

/* If you write to the byte at display_dirty_?table[n+0x4000], then
   the eight pixels starting at (xtable[n],ytable[n]) must be
   replotted */
static WORD display_dirty_ytable[6144];
static WORD display_dirty_xtable[6144];

/* If you write to the byte at display_dirty_?table2[n+0x5800], then
   the 64 pixels starting at (xtable2[n],ytable2[n]) must be
   replotted */
static WORD display_dirty_ytable2[768];
static WORD display_dirty_xtable2[768];

/* The number of frames mod 32 that have elapsed.
    0<=d_f_c<16 => Flashing characters are normal
   16<=d_f_c<32 => Flashing characters are reversed
*/
static BYTE display_frame_count;
static int display_flash_reversed;

/* Does this line need to be redisplayed? */
static BYTE display_is_dirty[192];

/* The next line to be replotted */
static int display_next_line;
static DWORD display_next_line_time;

static void display_draw_line(int y);
static void display_dirty8(WORD address, BYTE data);
static void display_dirty64(WORD address, BYTE attr);

static void display_plot8(int x,int y,BYTE data,BYTE ink,BYTE paper);
static void display_get_attr(int x,int y,BYTE *ink,BYTE *paper);
static void display_parse_attr(BYTE attr,BYTE *ink,BYTE *paper);

static void display_dirty_flashing(void);

int display_init(int argc, char **argv)
{
  int i,j,k,x,y;

  for(i=0;i<3;i++)
    for(j=0;j<8;j++)
      for(k=0;k<8;k++)
	display_line_start[ (64*i) + (8*j) + k ] =
	  32 * ( (64*i) + j + (k*8) );

  for(y=0;y<192;y++)
    display_attr_start[y]=6144 + (32*(y/8));

  for(y=0;y<192;y++)
    for(x=0;x<32;x++) {
      display_dirty_ytable[display_line_start[y]+x]=y;
      display_dirty_xtable[display_line_start[y]+x]=x;
    }

  for(y=0;y<24;y++)
    for(x=0;x<32;x++) {
      display_dirty_ytable2[ (32*y) + x ]=8*y;
      display_dirty_xtable2[ (32*y) + x ]=x;
    }

  display_frame_count=0; display_flash_reversed=0;

  x=256+ (2*display_border_width );
  y=192+ (2*display_border_height);

  if(xdisplay_init(argc,argv,x,y)) return 1;

  return 0;

}

void display_line(void)
{
  if(tstates>=display_next_line_time) {
    display_draw_line(display_next_line++);
    display_next_line_time=machine.line_times[display_next_line];
  }
}   

/* Redraw any bits of pixel line y which are flagged as `dirty' */
static void display_draw_line(int y)
{
  if(display_is_dirty[y]) {
    display_is_dirty[y]=0;
    xdisplay_line(y);
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

  display_is_dirty[y] = 1;
  
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

  memset(&display_is_dirty[y],1,8*sizeof(BYTE));

}

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y ) */
static void display_plot8(int x,int y,BYTE data,BYTE ink,BYTE paper)
{
  x*=8;
  xdisplay_putpixel(x+0,y, ( data & 0x80 ) ? ink : paper );
  xdisplay_putpixel(x+1,y, ( data & 0x40 ) ? ink : paper );
  xdisplay_putpixel(x+2,y, ( data & 0x20 ) ? ink : paper );
  xdisplay_putpixel(x+3,y, ( data & 0x10 ) ? ink : paper );
  xdisplay_putpixel(x+4,y, ( data & 0x08 ) ? ink : paper );
  xdisplay_putpixel(x+5,y, ( data & 0x04 ) ? ink : paper );
  xdisplay_putpixel(x+6,y, ( data & 0x02 ) ? ink : paper );
  xdisplay_putpixel(x+7,y, ( data & 0x01 ) ? ink : paper );
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
  display_border=colour;
  xdisplay_set_border(colour);
}

void display_frame(void)
{
  display_next_line=0;
  display_next_line_time=machine.line_times[0];
  display_frame_count++;
  if(display_frame_count==16) {
    display_flash_reversed=1;
    display_dirty_flashing();
  } else if(display_frame_count==32) {
    display_frame_count=0;
    display_flash_reversed=0;
    display_dirty_flashing();
  }
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

  for(y=0;y<192;y+=8) {
    for(x=0;x<32;x++) {
      display_parse_attr(read_screen_memory(display_attr_start[y]+x),
			 &ink,&paper);
      for(z=0;z<8;z++) {
	display_plot8(x,y+z,read_screen_memory(display_line_start[y+z]+x),
		      ink,paper);
	display_is_dirty[y+z]=1;
      }
    }
  }
}
