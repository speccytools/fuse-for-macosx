/* xui.c: Routines for dealing with the Xlib user interface
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

#include <config.h>

#ifdef UI_X			/* Use this iff we're using Xlib */

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "fuse.h"
#include "x.h"
#include "xdisplay.h"
#include "xui.h"

int xui_screenNum;		/* Which screen are we using on our
				   X server? */
Window xui_mainWindow;		/* Window ID for the main Fuse window */

int xui_init(int argc, char **argv, int width, int height)
{
  char *displayName=NULL;	/* Use default display */
  XWMHints *wmHints;
  XSizeHints *sizeHints;
  XClassHint *classHint;
  char *windowNameList="Fuse",*iconNameList="Fuse";
  XTextProperty windowName, iconName;
  unsigned long windowFlags;
  XSetWindowAttributes windowAttributes;

  /* Allocate memory for various things */

  if(!(wmHints = XAllocWMHints())) {
    fprintf(stderr,"%s: failure allocating memory\n", fuse_progname);
    return 1;
  }

  if(!(sizeHints = XAllocSizeHints())) {
    fprintf(stderr,"%s: failure allocating memory\n", fuse_progname);
    return 1;
  }

  if(!(classHint = XAllocClassHint())) {
    fprintf(stderr,"%s: failure allocating memory\n", fuse_progname);
    return 1;
  }

  if(XStringListToTextProperty(&windowNameList,1,&windowName) == 0 ) {
    fprintf(stderr,"%s: structure allocation for windowName failed\n",
	    fuse_progname);
    return 1;
  }

  if(XStringListToTextProperty(&iconNameList,1,&iconName) == 0 ) {
    fprintf(stderr,"%s: structure allocation for iconName failed\n",
	    fuse_progname);
    return 1;
  }

  /* Open a connection to the X server */

  if ( ( display=XOpenDisplay(displayName)) == NULL ) {
    fprintf(stderr,"%s: cannot connect to X server %s\n", fuse_progname,
	    XDisplayName(displayName));
    return 1;
  }

  xui_screenNum=DefaultScreen(display);

  /* Create the main window */

  xui_mainWindow = XCreateSimpleWindow(
    display, RootWindow(display,xui_screenNum), 0, 0, width, height, 0,
    BlackPixel(display,xui_screenNum), WhitePixel(display,xui_screenNum));

  /* Set standard window properties */

  sizeHints->flags = PBaseSize | PResizeInc | PAspect | PMaxSize;
  sizeHints->base_width=0;
  sizeHints->base_height=0;
  sizeHints->width_inc=width;
  sizeHints->height_inc=height;
  sizeHints->min_aspect.x=width;
  sizeHints->min_aspect.y=height;
  sizeHints->max_aspect.x=width;
  sizeHints->max_aspect.y=height;
  sizeHints->max_width=3*width;
  sizeHints->max_height=3*height;

  wmHints->flags=StateHint | InputHint;
  wmHints->initial_state=NormalState;
  wmHints->input=True;

  classHint->res_name=fuse_progname;
  classHint->res_class="Fuse";

  XSetWMProperties(display, xui_mainWindow, &windowName, &iconName, argv, argc,
		   sizeHints,wmHints,classHint);

  /* Ask the server to use its backing store for this window */

  windowFlags=CWBackingStore;
  windowAttributes.backing_store=WhenMapped;

  XChangeWindowAttributes(display, xui_mainWindow, windowFlags,
			  &windowAttributes);

  /* Select which types of event we want to receive */

  XSelectInput(display, xui_mainWindow, ExposureMask | KeyPressMask |
	       KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
	       StructureNotifyMask);

  if(xdisplay_init(width,height)) return 1;

  /* And finally display the window */
  XMapWindow(display,xui_mainWindow);

  return 0;
}

int xui_end(void)
{
  int error;

  /* Don't display the window whilst doing all this */
  XUnmapWindow(display,xui_mainWindow);

  /* Tidy up the low level stuff */
  error=xdisplay_end(); if(error) return error;

  /* Now free up the window itself */
  XDestroyWindow(display,xui_mainWindow);

  /* And disconnect from the X server */
  XCloseDisplay(display);

  return 0;
}

#endif				/* #ifdef UI_X */
