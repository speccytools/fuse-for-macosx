/* xdisplay.c: Routines for dealing with the X display
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
#include <X11/Xutil.h>

#include "display.h"
#include "x.h"
#include "xdisplay.h"

static int screenNum;		/* And which screen on that display? */

static Window mainWindow;	/* Window ID for the main Fuse window */
static XImage *image;		/* The image structure to draw the
				   Speccy's screen on */
static GC gc;			/* A graphics context to draw with */

static char* progname;

static unsigned long colours[16];

static int xdisplay_allocate_colours(int numColours, unsigned long *colours);
static int xdisplay_allocate_gc(Window window, GC *gc);
static int xdisplay_allocate_image(int width, int height);

int xdisplay_init(int argc, char **argv, int width, int height)
{
  char *displayName=NULL;	/* Use default display */
  XWMHints *wmHints;
  XSizeHints *sizeHints;
  XClassHint *classHint;
  char *windowNameList="Fuse",*iconNameList="Fuse";
  XTextProperty windowName, iconName;
  unsigned long windowFlags;
  XSetWindowAttributes windowAttributes;

  progname=argv[0];

  /* Allocate memory for various things */

  if(!(wmHints = XAllocWMHints())) {
    fprintf(stderr,"%s: failure allocating memory\n",progname);
    return 1;
  }

  if(!(sizeHints = XAllocSizeHints())) {
    fprintf(stderr,"%s: failure allocating memory\n",progname);
    return 1;
  }

  if(!(classHint = XAllocClassHint())) {
    fprintf(stderr,"%s: failure allocating memory\n",progname);
    return 1;
  }

  if(XStringListToTextProperty(&windowNameList,1,&windowName) == 0 ) {
    fprintf(stderr,"%s: structure allocation for windowName failed\n",
	    progname);
    return 1;
  }

  if(XStringListToTextProperty(&iconNameList,1,&iconName) == 0 ) {
    fprintf(stderr,"%s: structure allocation for iconName failed\n",
	    progname);
    return 1;
  }

  /* Open a connection to the X server */

  if ( ( display=XOpenDisplay(displayName)) == NULL ) {
    fprintf(stderr,"%s: cannot connect to X server %s\n",progname,
	    XDisplayName(displayName));
    return 1;
  }

  screenNum=DefaultScreen(display);

/*   fprintf(stderr,"%s: connected to screen %d on X server %s\n",progname, */
/* 	  screenNum,XDisplayName(displayName)); */

  /* Create the main window */

  mainWindow = XCreateSimpleWindow(
    display, RootWindow(display,screenNum), 0, 0, width, height, 0,
    BlackPixel(display,screenNum), WhitePixel(display,screenNum));

  /* Set standard window properties */

  sizeHints->flags = PBaseSize | PResizeInc;
  sizeHints->base_width=width;
  sizeHints->base_height=height;
  sizeHints->width_inc=width;
  sizeHints->height_inc=height;

  wmHints->flags=StateHint | InputHint;
  wmHints->initial_state=NormalState;
  wmHints->input=True;

  classHint->res_name=progname;
  classHint->res_class="Fuse";

  XSetWMProperties(display, mainWindow, &windowName, &iconName, argv, argc,
		   sizeHints,wmHints,classHint);

  /* Ask the server to use its backing store for this window */

  windowFlags=CWBackingStore;
  windowAttributes.backing_store=WhenMapped;

  XChangeWindowAttributes(display,mainWindow,windowFlags,&windowAttributes);

  /* Select which types of event we want to receive */

  XSelectInput(display,mainWindow,ExposureMask | KeyPressMask |
	       KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
	       StructureNotifyMask);

  if(xdisplay_allocate_colours(16,colours)) return 1;
  if(xdisplay_allocate_gc(mainWindow,&gc)) return 1;
  if(xdisplay_allocate_image(width,height)) return 1;

  /* Set the background (aka border) colour */
  xdisplay_set_border(7);
  
  /* And finally display the window */
  XMapWindow(display,mainWindow);

  return 0;
}

static int xdisplay_allocate_colours(int numColours, unsigned long *colours)
{
  XColor colour;
  Colormap currentMap;

  char *colour_names[] = {
    "black",
    "blue3",
    "red3",
    "magenta3",
    "green3",
    "cyan3",
    "yellow3",
    "gray80",
    "black",
    "blue",
    "red",
    "magenta",
    "green",
    "cyan",
    "yellow",
    "white",
  };

  int i;

  currentMap=DefaultColormap(display,screenNum);

  for(i=0;i<numColours;i++) {
    if( XParseColor(display, currentMap, colour_names[i], &colour) == 0 ) {
      fprintf(stderr,"%s: couldn't parse `%s'\n",progname,colour_names[i]);
      return 1;
    }
    if( XAllocColor(display, currentMap, &colour) == 0 ) {
      fprintf(stderr,"%s: couldn't allocate `%s'\n",progname,colour_names[i]);
      if(currentMap == DefaultColormap(display,screenNum)) {
	fprintf(stderr,"%s: switching to private colour map\n",progname);
	currentMap=XCopyColormapAndFree(display,currentMap);
	XSetWindowColormap(display,mainWindow,currentMap);
	i--;		/* Need to repeat the failed allocation */
      } else {
	return 1;
      }
    }
    colours[i]=colour.pixel;
  }

  return 0;
}
  
static int xdisplay_allocate_gc(Window window,GC *gc)
{
  unsigned valuemask=0;
  XGCValues values;

  *gc=XCreateGC(display,window,valuemask,&values);

  return 0;
}

static int xdisplay_allocate_image(int width, int height)
{
  image=XCreateImage(display, DefaultVisual(display,screenNum),
		     DefaultDepth(display,screenNum),ZPixmap,0,NULL,
		     width,height,8,0);

  if(!image) {
    fprintf(stderr,"%s: couldn't create image\n",progname);
    return 1;
  }

  if( (image->data=(char*)malloc(image->bytes_per_line*height)) == NULL ) {
    fprintf(stderr,"%s: out of memory for image data\n",progname);
    return 1;
  }

  return 0;
}

void xdisplay_putpixel(int x,int y,int colour)
{
  XPutPixel(image,x,y,colours[colour]);
}

void xdisplay_line(int y)
{
/*   fprintf(stderr,"Copying line %d to window\n",y); */
  XPutImage(display,mainWindow,gc,image,0,y,display_border_width,
	    y+display_border_height,256,1);
}

void xdisplay_area(int x, int y, int width, int height)
{
/*   fprintf(stderr,"Displaying area %dx%d+%d+%d\n",width,height,x,y); */
  XPutImage(display,mainWindow,gc,image,x,y,x+display_border_width,
	    y+display_border_height,width,height);
}

void xdisplay_set_border(int colour)
{
  XSetWindowBackground(display,mainWindow,colours[colour]);
}
