/* ggiui.c: Routines for dealing with the GGI user interface
   Copyright (c) 2003 Catalin Mihaila, Philip Kendall

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

   Catalin: catalin@idgrup.ro

*/

#include "config.h"

#ifdef UI_GGI

#include <ggi/ggi.h>

#include "display.h"
#include "ggi_internals.h"
#include "ui/ui.h"

/* A copy of every pixel on the screen, replaceable by plotting directly into
   rgb_image below */
libspectrum_word
  ggidisplay_image[ 2 * DISPLAY_SCREEN_HEIGHT ][ DISPLAY_SCREEN_WIDTH ];
ptrdiff_t ggidisplay_pitch = DISPLAY_SCREEN_WIDTH * sizeof( libspectrum_word );

static const unsigned int colour_pal[] = {

    0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0xC000,
    0xC000, 0x0000, 0x0000,
    0xC000, 0x0000, 0xC000,
    0x0000, 0xC000, 0x0000,
    0x0000, 0xC000, 0xC000,
    0xC000, 0xC000, 0x0000,
    0xC000, 0xC000, 0xC000,
    0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0xFFFF,
    0xFFFF, 0x0000, 0x0000,
    0xFFFF, 0x0000, 0xFFFF,
    0x0000, 0xFFFF, 0x0000,
    0x0000, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0x0000,
    0xFFFF, 0xFFFF, 0xFFFF

};

static ggi_visual_t vis;
static ggi_mode mo;
static ggi_color col;

void
uidisplay_area( int x, int y, int width, int height )
{
  int disp_x, disp_y;

  for(disp_x=x;disp_x < x+width;disp_x++) {

    for(disp_y=y;disp_y < y+height;disp_y++) {

      col.r = colour_pal[ (       3 * ggidisplay_image[disp_y][disp_x] )   ];
      col.g = colour_pal[ ( 1 + ( 3 * ggidisplay_image[disp_y][disp_x] ) ) ];
      col.b = colour_pal[ ( 2 + ( 3 * ggidisplay_image[disp_y][disp_x] ) ) ];

      ggiSetGCForeground( vis, ggiMapColor( vis, &col ) );
      ggiDrawPixel( vis, disp_x, disp_y );

    }
  }
}

int
uidisplay_end( void )
{
  if( vis == NULL ) return 0;

  display_ui_initialised = 0;

  ggiClose( vis );
  ggiExit();

  return 0;
}

void
uidisplay_frame_end( void )
{
  ggiFlush( vis );
}

int
uidisplay_init( int width, int height )
{
  int error;

  error = ggiInit();
  if( error ) {
    ui_error( UI_ERROR_ERROR, "uidisplay_init: ggiInit failed" );
    return error;
  }

  vis = ggiOpen( NULL );

  if( vis == NULL ) {
    ui_error( UI_ERROR_ERROR,
	      "uidisplay_init: unable to open default visual" );
    return 1;
  }

  error = ggiSetFlags( vis, GGIFLAG_ASYNC );
  if( error ) {
    ui_error( UI_ERROR_ERROR, "uidisplay_init: ggiSetFlags failed" );
    return error;
  }

  mo.virt.x = width;
  mo.virt.y = height;

  mo.visible.x = GGI_AUTO;
  mo.visible.y = GGI_AUTO;

  mo.frames = GGI_AUTO;
  mo.graphtype = GGI_AUTO;

  mo.dpp.x = 1;
  mo.dpp.y = 1;

  error = ggiCheckMode( vis, &mo );
  if( error ) {
    ui_error( UI_ERROR_ERROR, "uidisplay_init: ggiCheckMode failed" );
    return error;
  }

  error = ggiSetMode( vis, &mo );
  if( error ) {
    ui_error( UI_ERROR_ERROR, "uidisplay_init: ggiSetMode failed" );
    return error;
  }

  display_ui_initialised = 1;

  return 0;
}


void
uidisplay_hotswap_gfx_mode( void )
{
  return;
}

/* Set one pixel in the display */
void
uidisplay_putpixel( int x, int y, int colour )
{
  if( machine_current->timex ) {
    x <<= 1; y <<= 1;
    ggidisplay_image[y  ][x  ] = colour;
    ggidisplay_image[y  ][x+1] = colour;
    ggidisplay_image[y+1][x  ] = colour;
    ggidisplay_image[y+1][x+1] = colour;
  } else {
    ggidisplay_image[y][x] = colour;
  }
}

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y ) */
void
uidisplay_plot8( int x, int y, libspectrum_byte data,
                libspectrum_byte ink, libspectrum_byte paper )
{
  x <<= 3;

  if( machine_current->timex ) {
    int i;

    x <<= 1; y <<= 1;
    for( i=0; i<2; i++,y++ ) {
      ggidisplay_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
      ggidisplay_image[y][x+ 1] = ( data & 0x80 ) ? ink : paper;
      ggidisplay_image[y][x+ 2] = ( data & 0x40 ) ? ink : paper;
      ggidisplay_image[y][x+ 3] = ( data & 0x40 ) ? ink : paper;
      ggidisplay_image[y][x+ 4] = ( data & 0x20 ) ? ink : paper;
      ggidisplay_image[y][x+ 5] = ( data & 0x20 ) ? ink : paper;
      ggidisplay_image[y][x+ 6] = ( data & 0x10 ) ? ink : paper;
      ggidisplay_image[y][x+ 7] = ( data & 0x10 ) ? ink : paper;
      ggidisplay_image[y][x+ 8] = ( data & 0x08 ) ? ink : paper;
      ggidisplay_image[y][x+ 9] = ( data & 0x08 ) ? ink : paper;
      ggidisplay_image[y][x+10] = ( data & 0x04 ) ? ink : paper;
      ggidisplay_image[y][x+11] = ( data & 0x04 ) ? ink : paper;
      ggidisplay_image[y][x+12] = ( data & 0x02 ) ? ink : paper;
      ggidisplay_image[y][x+13] = ( data & 0x02 ) ? ink : paper;
      ggidisplay_image[y][x+14] = ( data & 0x01 ) ? ink : paper;
      ggidisplay_image[y][x+15] = ( data & 0x01 ) ? ink : paper;
    }
  } else {
    ggidisplay_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
    ggidisplay_image[y][x+ 1] = ( data & 0x40 ) ? ink : paper;
    ggidisplay_image[y][x+ 2] = ( data & 0x20 ) ? ink : paper;
    ggidisplay_image[y][x+ 3] = ( data & 0x10 ) ? ink : paper;
    ggidisplay_image[y][x+ 4] = ( data & 0x08 ) ? ink : paper;
    ggidisplay_image[y][x+ 5] = ( data & 0x04 ) ? ink : paper;
    ggidisplay_image[y][x+ 6] = ( data & 0x02 ) ? ink : paper;
    ggidisplay_image[y][x+ 7] = ( data & 0x01 ) ? ink : paper;
  }
}

/* Print the 16 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (16*x) , y ) */
void
uidisplay_plot16( int x, int y, libspectrum_word data,
                 libspectrum_byte ink, libspectrum_byte paper )
{
  int i;
  x <<= 4; y <<= 1;

  for( i=0; i<2; i++,y++ ) {
    ggidisplay_image[y][x+ 0] = ( data & 0x8000 ) ? ink : paper;
    ggidisplay_image[y][x+ 1] = ( data & 0x4000 ) ? ink : paper;
    ggidisplay_image[y][x+ 2] = ( data & 0x2000 ) ? ink : paper;
    ggidisplay_image[y][x+ 3] = ( data & 0x1000 ) ? ink : paper;
    ggidisplay_image[y][x+ 4] = ( data & 0x0800 ) ? ink : paper;
    ggidisplay_image[y][x+ 5] = ( data & 0x0400 ) ? ink : paper;
    ggidisplay_image[y][x+ 6] = ( data & 0x0200 ) ? ink : paper;
    ggidisplay_image[y][x+ 7] = ( data & 0x0100 ) ? ink : paper;
    ggidisplay_image[y][x+ 8] = ( data & 0x0080 ) ? ink : paper;
    ggidisplay_image[y][x+ 9] = ( data & 0x0040 ) ? ink : paper;
    ggidisplay_image[y][x+10] = ( data & 0x0020 ) ? ink : paper;
    ggidisplay_image[y][x+11] = ( data & 0x0010 ) ? ink : paper;
    ggidisplay_image[y][x+12] = ( data & 0x0008 ) ? ink : paper;
    ggidisplay_image[y][x+13] = ( data & 0x0004 ) ? ink : paper;
    ggidisplay_image[y][x+14] = ( data & 0x0002 ) ? ink : paper;
    ggidisplay_image[y][x+15] = ( data & 0x0001 ) ? ink : paper;
  }
}

void
uiggi_event( void )
{
  ggi_event ev;
  ggi_event_mask mask;
  struct timeval t = { 0, 0 };

  mask = ggiEventPoll( vis, emAll, &t );
  if( !mask ) return;

  while( ggiEventsQueued( vis, mask ) ) {

    ggiEventRead( vis, &ev, mask );

    switch( ev.any.type ) {

    case evKeyPress:
      ggikeyboard_keypress( ev.key.sym );
      break;

    case evKeyRelease:
      ggikeyboard_keyrelease( ev.key.sym );
      break;

    }
  }
}

#endif				/* #ifdef UI_GGI */
