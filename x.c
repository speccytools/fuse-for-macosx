/* x.c: Routines for dealing with X events
   Copyright (c) 2000 Philip Kendall

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
#include <stdlib.h>
#include <X11/Xlib.h>

#include "display.h"
#include "xdisplay.h"
#include "xkeyboard.h"

Display *display;		/* Which display are we connected to */

Bool x_trueFunction();

int x_event(void)
{
  XEvent event;
  int x,y,width,height;

  while(XCheckIfEvent(display,&event,x_trueFunction,NULL)) {
/*     fprintf(stderr,"%s: got an event of type %d\n",progname,event.type); */
    switch(event.type) {
    case ButtonPress:
      return 1;
    case Expose:
      x=event.xexpose.x-display_border_width;
      y=event.xexpose.y-display_border_height;
      width=event.xexpose.width;
      height=event.xexpose.height;
      if(x<0) {  width += x; x=0; }
      if(y<0) { height += y; y=0; }
      if( width>256)  width=256;
      if(height>192) height=192;
      xdisplay_area(x,y,width,height);
      break;
    case KeyPress:
      return xkeyboard_keypress(&(event.xkey));
    case KeyRelease:
      xkeyboard_keyrelease(&(event.xkey));
    }
  }
  return 0;
}
    
Bool x_trueFunction()
{
  return True;
}
