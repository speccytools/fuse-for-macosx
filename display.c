/* display.c: Routines for printing the Spectrum screen
   Copyright (c) 1999 Philip Kendall and Thomas Harte

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

   E-mail: pak21@cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <stdio.h>

#include "alleg.h"
#include "display.h"
#include "spectrum.h"

/* The Speccy's colours (R,G,B triples); colours 0-7 are non-bright,
   colours 8-15 are bright */
RGB spectrum_colours[] = {
  {   0,   0,   0},
  {   0,   0, 250},
  { 250,   0,   0},
  { 250,   0, 250},
  {   0, 250,   0},
  {   0, 250, 250},
  { 250, 250,   0},
  { 250, 250, 250},
  {   0,   0,   0},
  {   0,   0, 255},
  { 255,   0,   0},
  { 255,   0, 255},
  {   0, 255,   0},
  {   0, 255, 255},
  { 255, 255,   0},
  { 255, 255, 255}
};

/* Width and height of the emulated border */
int display_border_width=32;
int display_border_height=24;

/* Palette entry to use for the Spectrum border colour */
static const int DISPLAY_BORDER=16;
/* The current border colour */
int display_border;

/* Offsets as to where the data and the attributes for each pixel
   line start */
WORD display_line_start[192];
WORD display_attr_start[192];

/* If you write to the byte at display_dirty_?table[n+0x4000], then
   the eight pixels starting at (xtable[n],ytable[n]) must be
   replotted */
WORD display_dirty_ytable[6144];
WORD display_dirty_xtable[6144];

/* If you write to the byte at display_dirty_?table2[n+0x5800], then
   the 64 pixels starting at (xtable2[n],ytable2[n]) must be
   replotted */
WORD display_dirty_ytable2[768];
WORD display_dirty_xtable2[768];

/* The number of frames mod 32 that have elapsed.
    0<=d_f_c<16 => Flashing characters are normal
   16<=d_f_c<32 => Flashing characters are reversed
*/
BYTE display_frame_count;
int display_flash_reversed;

/* Does the byte at (x,y) need to be replotted? */
BYTE display_is_dirty[192][32];

/* The next line to be replotted */
int display_next_line;

static void display_draw_line(int y);
static void display_dirty8(WORD address);
static void display_dirty64(WORD address);

static void display_get_attr(int x,int y,BYTE *ink,BYTE *paper);
static void display_plot8_putpixel(int x,int y,BYTE data,BYTE ink,BYTE paper);
static void display_plot8_modex(int x,int y,BYTE data,BYTE ink,BYTE paper);
static void display_plot8_writeline(int x,int y,BYTE data,BYTE ink,BYTE paper);
static void display_parse_attr(BYTE attr,BYTE *ink,BYTE *paper);

static void display_dirty_flashing(void);

void (*display_plot8)(int x,int y,BYTE data,BYTE ink,BYTE paper);

void display_init(void)
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

#ifdef Xwin_ALLEGRO
  /* Get rid of XWinAllegro's annoying border */
  putenv("XWIN_ALLEGRO_XBORDER=0");
  putenv("XWIN_ALLEGRO_YBORDER=0");
#endif

  set_color_depth(8);
  display_start_res(x,y);

}

void display_start_res(int w, int h)
{
  int i; RGB border;
  
  get_color(DISPLAY_BORDER,&border);
  set_gfx_mode(GFX_AUTODETECT,w,h,0,0);
  set_color(DISPLAY_BORDER,&border);
  display_border_width = (SCREEN_W - 256) / 2;
  display_border_height = (SCREEN_H - 192) / 2;
 
#ifdef Xwin_ALLEGRO
  display_plot8 = display_plot8_putpixel;
#else			/* #ifdef Xwin_ALLEGRO */
  display_plot8 = display_plot8_writeline;
  if(is_planar_bitmap(screen))
  {
    display_plot8 = display_plot8_modex;
    display_border_width /= 4;
  }
  bmp_select(screen);
#endif			/* #ifdef Xwin_ALLEGRO */
  
  for(i=0;i<16;i++) set_color(i,&spectrum_colours[i]);
  clear_to_color(screen,DISPLAY_BORDER);
  display_entire_screen();
}


/* Redisplay the entire screen */
void display_entire_screen(void)
{
  int x,y; BYTE ink,paper,data;

  for(y=0;y<192;y++) {
    for(x=0;x<32;x++) {
      display_get_attr(x,y,&ink,&paper);
      data=read_screen_memory(display_line_start[y]+x);
      display_plot8(x,y,data,ink,paper);
    }
    memset(&display_is_dirty[y],0,32*sizeof(BYTE));
  }
}

void display_line(void)
{
  if(tstates>=machine.line_times[display_next_line]) {
    display_draw_line(display_next_line++);
  }
}   

/* Redraw any bits of pixel line y which are flagged as `dirty' */
static void display_draw_line(int y)
{
  int x; BYTE data,ink,paper;
  
  for(x=0;x<32;x++) {
    if(display_is_dirty[y][x]) {
      display_is_dirty[y][x]=0;
      display_get_attr(x,y,&ink,&paper);
      data=read_screen_memory(display_line_start[y]+x);
      display_plot8(x,y,data,ink,paper);
    }
  }
}

/* Mark as `dirty' the pixels which have been changed by a write to byte
   `address'; 0x4000 <= address < 0x5b00 */
void display_dirty(WORD address)
{
  if(address<0x5800) {		/* 0x5800 = first attributes byte */
    display_dirty8(address);
  } else {
    display_dirty64(address);
  }
}

static void display_dirty8(WORD address)
{
  display_is_dirty[ display_dirty_ytable[address-0x4000] ]
                  [ display_dirty_xtable[address-0x4000] ] = 1;
}

static void display_dirty64(WORD address)
{
  int x,y;

  x=display_dirty_xtable2[address-0x5800];
  y=display_dirty_ytable2[address-0x5800];

  display_is_dirty[y++][x]=1;
  display_is_dirty[y++][x]=1;
  display_is_dirty[y++][x]=1;
  display_is_dirty[y++][x]=1;
  display_is_dirty[y++][x]=1;
  display_is_dirty[y++][x]=1;
  display_is_dirty[y++][x]=1;
  display_is_dirty[y  ][x]=1;
}

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y ) */
static void display_plot8_putpixel(int x,int y,BYTE data,BYTE ink,BYTE paper)
{
  x*=8; x+=display_border_width; y+=display_border_height;
  putpixel(screen,x+0,y, ( data & 0x80 ) ? ink : paper );
  putpixel(screen,x+1,y, ( data & 0x40 ) ? ink : paper );
  putpixel(screen,x+2,y, ( data & 0x20 ) ? ink : paper );
  putpixel(screen,x+3,y, ( data & 0x10 ) ? ink : paper );
  putpixel(screen,x+4,y, ( data & 0x08 ) ? ink : paper );
  putpixel(screen,x+5,y, ( data & 0x04 ) ? ink : paper );
  putpixel(screen,x+6,y, ( data & 0x02 ) ? ink : paper );
  putpixel(screen,x+7,y, ( data & 0x01 ) ? ink : paper );
}

#ifdef Xwin_ALLEGRO

/* Direct memory access not available with XWinAllegro, so define
   the clever display functions as dummys which call the normal
   one
*/

static void display_plot8_writeline(int x,int y,BYTE data,BYTE ink,BYTE paper)
{
  display_plot8_putpixel(x,y,data,ink,paper);
}

static void display_plot8_modex(int x,int y,BYTE data,BYTE ink,BYTE paper)
{
  display_plot8_putpixel(x,y,data,ink,paper);
}

#else			/* #ifdef Xwin_ALLEGRO */

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y ) */
static void display_plot8_writeline(int x,int y,BYTE data,BYTE ink,BYTE paper)
{
  unsigned long addr;
  x*=8; x+=display_border_width; y+=display_border_height;
  addr = bmp_write_line(screen, y) + x;
  bmp_write8(addr++, ( data & 0x80 ) ? ink : paper);
  bmp_write8(addr++, ( data & 0x40 ) ? ink : paper);
  bmp_write8(addr++, ( data & 0x20 ) ? ink : paper);
  bmp_write8(addr++, ( data & 0x10 ) ? ink : paper);
  bmp_write8(addr++, ( data & 0x08 ) ? ink : paper);
  bmp_write8(addr++, ( data & 0x04 ) ? ink : paper);
  bmp_write8(addr++, ( data & 0x02 ) ? ink : paper);
  bmp_write8(addr++, ( data & 0x01 ) ? ink : paper);
  bmp_unwrite_line(screen);
}

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y )

   modex version - writes two pixels per plane switch, assuming that pixels
   are contiguous, hence quite a bit faster than putpixel

   NOTE: this will not function correctly if x is not on an 8 pixel
   boundary.
*/
static void display_plot8_modex(int x,int y,BYTE data,BYTE ink,BYTE paper)
{
  int xp1;

  x = (int)screen->line[y+display_border_height]+(x*2 + display_border_width);
  xp1=x+1;

  outportw(0x3c4, 0x102);
  bmp_write8(x  , ( data & 0x80 ) ? ink : paper);
  bmp_write8(xp1, ( data & 0x08 ) ? ink : paper);

  outportw(0x3c4, 0x202);
  bmp_write8(x  , ( data & 0x40 ) ? ink : paper);
  bmp_write8(xp1, ( data & 0x04 ) ? ink : paper);

  outportw(0x3c4, 0x402);
  bmp_write8(x  , ( data & 0x20 ) ? ink : paper);
  bmp_write8(xp1, ( data & 0x02 ) ? ink : paper);
  
  outportw(0x3c4, 0x802);
  bmp_write8(x  , ( data & 0x10 ) ? ink : paper);
  bmp_write8(xp1, ( data & 0x01 ) ? ink : paper);
}

#endif		/* #ifdef Xwin_ALLEGRO */

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
  set_color(DISPLAY_BORDER,&spectrum_colours[colour]);
}

void display_frame(void)
{
  display_next_line=0;
  display_frame_count++;
  if(display_frame_count==16) {
    display_flash_reversed=1;
    display_dirty_flashing();
  }
  else if(display_frame_count==32) {
    display_frame_count=0;
    display_flash_reversed=0;
    display_dirty_flashing();
  }
}

static void display_dirty_flashing(void)
{
  int offset;

  for(offset=0x1800;offset<0x1b00;offset++)
    if( read_screen_memory(offset) & 0x80 )
      display_dirty64(offset+0x4000);
}
