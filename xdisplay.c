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

#include <config.h>

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

/* The current size of the window (in units of DISPLAY_SCREEN_*) */
static int xdisplay_current_size=1;

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

  /* Create the main window */

  mainWindow = XCreateSimpleWindow(
    display, RootWindow(display,screenNum), 0, 0, width, height, 0,
    BlackPixel(display,screenNum), WhitePixel(display,screenNum));

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
      fprintf(stderr,"%s: couldn't parse colour `%s'\n",progname,
	      colour_names[i]);
      return 1;
    }
    if( XAllocColor(display, currentMap, &colour) == 0 ) {
      fprintf(stderr,"%s: couldn't allocate colour `%s'\n",progname,
	      colour_names[i]);
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
		     3*width,3*height,8,0);

  if(!image) {
    fprintf(stderr,"%s: couldn't create image\n",progname);
    return 1;
  }

  if( (image->data=(char*)malloc(image->bytes_per_line*3*height)) == NULL ) {
    fprintf(stderr,"%s: out of memory for image data\n",progname);
    return 1;
  }

  return 0;
}

int xdisplay_configure_notify(int width, int height)
{
  int y,size;

  size = width / DISPLAY_SCREEN_WIDTH;

  /* If we're the same size as before, nothing special needed */
  if( size == xdisplay_current_size ) return 0;

  /* Else set ourselves to the new height */
  xdisplay_current_size=size;

  /* Redraw the entire screen... */
  display_refresh_all();

  /* And the entire border */
  for(y=0;y<DISPLAY_BORDER_HEIGHT;y++) {
    xdisplay_set_border(y,0,DISPLAY_SCREEN_WIDTH,display_border);
    xdisplay_set_border(DISPLAY_BORDER_HEIGHT+DISPLAY_HEIGHT+y,0,
			DISPLAY_SCREEN_WIDTH,display_border);
  }

  for(y=DISPLAY_BORDER_HEIGHT;y<DISPLAY_BORDER_HEIGHT+DISPLAY_HEIGHT;y++) {
    xdisplay_set_border(y,0,DISPLAY_BORDER_WIDTH,display_border);
    xdisplay_set_border(y,DISPLAY_BORDER_WIDTH+DISPLAY_WIDTH,
			DISPLAY_SCREEN_WIDTH,display_border);
  }

  return 0;
}

void xdisplay_putpixel(int x,int y,int colour)
{
  switch(xdisplay_current_size) {
  case 1:
    XPutPixel(image,  x  ,  y  ,colours[colour]);
    break;
  case 2:
    XPutPixel(image,2*x,  2*y  ,colours[colour]);
    XPutPixel(image,2*x+1,2*y  ,colours[colour]);
    XPutPixel(image,2*x  ,2*y+1,colours[colour]);
    XPutPixel(image,2*x+1,2*y+1,colours[colour]);
    break;
  case 3:
    XPutPixel(image,3*x,  3*y  ,colours[colour]);
    XPutPixel(image,3*x+1,3*y  ,colours[colour]);
    XPutPixel(image,3*x+2,3*y  ,colours[colour]);
    XPutPixel(image,3*x  ,3*y+1,colours[colour]);
    XPutPixel(image,3*x+1,3*y+1,colours[colour]);
    XPutPixel(image,3*x+2,3*y+1,colours[colour]);
    XPutPixel(image,3*x  ,3*y+2,colours[colour]);
    XPutPixel(image,3*x+1,3*y+2,colours[colour]);
    XPutPixel(image,3*x+2,3*y+2,colours[colour]);
    break;
  }
}

void xdisplay_line(int y)
{
  XPutImage(display,mainWindow,gc,image,
	    0,xdisplay_current_size*y,
	    0,xdisplay_current_size*y,
	    xdisplay_current_size*DISPLAY_SCREEN_WIDTH,xdisplay_current_size);
}

void xdisplay_area(int x, int y, int width, int height)
{
  XPutImage(display,mainWindow,gc,image,x,y,x,y,width,height);
}

void xdisplay_set_border(int line, int pixel_from, int pixel_to, int colour)
{
  int x;
  
  switch(xdisplay_current_size) {
  case 1:
    for(x=pixel_from;x<pixel_to;x++) {
      XPutPixel(image,  x  ,  line  ,colours[colour]);
    }
    break;
  case 2:
    for(x=pixel_from;x<pixel_to;x++) {
      XPutPixel(image,2*x  ,2*line  ,colours[colour]);
      XPutPixel(image,2*x+1,2*line  ,colours[colour]);
      XPutPixel(image,2*x  ,2*line+1,colours[colour]);
      XPutPixel(image,2*x+1,2*line+1,colours[colour]);
    }
    break;
  case 3:
    for(x=pixel_from;x<pixel_to;x++) {
      XPutPixel(image,3*x  ,3*line  ,colours[colour]);
      XPutPixel(image,3*x+1,3*line  ,colours[colour]);
      XPutPixel(image,3*x+2,3*line  ,colours[colour]);
      XPutPixel(image,3*x  ,3*line+1,colours[colour]);
      XPutPixel(image,3*x+1,3*line+1,colours[colour]);
      XPutPixel(image,3*x+2,3*line+1,colours[colour]);
      XPutPixel(image,3*x  ,3*line+2,colours[colour]);
      XPutPixel(image,3*x+1,3*line+2,colours[colour]);
      XPutPixel(image,3*x+2,3*line+2,colours[colour]);
    }
    break;
  }
}

int xdisplay_end(void)
{
  /* Don't display the window whilst doing all this! */
  XUnmapWindow(display,mainWindow);

  /* Free the XImage used to store screen data; also frees the malloc'd
     data */
  XDestroyImage(image);

  /* Free the allocated GC */
  XFreeGC(display,gc);

  /* Now free up the window itself */
  XDestroyWindow(display,mainWindow);

  /* And disconnect from the X server */
  XCloseDisplay(display);

  return 0;
}
