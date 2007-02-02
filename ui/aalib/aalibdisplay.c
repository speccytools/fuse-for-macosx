/* aalibdisplay.c: Routines for dealing with the aalib display
   Copyright (c) 2001 Philip Kendall

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#ifdef UI_AALIB			/* Use this iff we're using svgalib */

#include <stdio.h>
#include <stdlib.h>

#include <aalib.h>

#include "aalibui.h"
#include "fuse.h"
#include "display.h"
#include "ui/uidisplay.h"

static int colours[16];

static int aalibdisplay_allocate_colours(int numColours, int *colours);

int uidisplay_init(int width, int height)
{
  int error;

  aa_defparams.width = 128;
  aa_defparams.height = 48;

  aalibui_context = aa_autoinit( &aa_defparams );
  if( aalibui_context == NULL ) return 1;

  error = aalibdisplay_allocate_colours(16, colours);
  if( error ) return error;

  return 0;
}

static int aalibdisplay_allocate_colours(int num_colours, int *colours)
{
  int colour_palette[] = {
  0,0,0,
  0,0,192,
  192,0,0,
  192,0,192,
  0,192,0,
  0,192,192,
  192,192,0,
  192,192,192,
  0,0,0,
  0,0,255,
  255,0,0,
  255,0,255,
  0,255,0,
  0,255,255,
  255,255,0,
  255,255,255
  };
  
  int i;

  for( i=0; i<num_colours; i++ )
    aa_setpalette( colours, i,
		   colour_palette[ i*3   ],
		   colour_palette[ i*3+1 ],
		   colour_palette[ i*3+2 ] );

  return 0;
}
  
void uidisplay_putpixel(int x,int y,int colour)
{
  if( colour != 7 ) fprintf( stderr, "putpixel: %d %d %d\n", x, y, colour);
  if( y % 2 == 0 ) {
    if(x%2!=0) return;
    x -= DISPLAY_BORDER_WIDTH;
    y -= DISPLAY_BORDER_HEIGHT;
    aa_putpixel( aalibui_context, x>>1, y/2, colour );
  }
}

void
uidisplay_frame_end( void )
{
  return;
}

void uidisplay_lines( int start, int end )
{
  return;
}

void uidisplay_set_border(int line, int pixel_from, int pixel_to, int colour)
{
  return;
}

int uidisplay_end(void)
{
    aa_close( aalibui_context );
    return 0;
}

#endif				/* #ifdef UI_AALIB */
