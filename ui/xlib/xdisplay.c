/* xdisplay.c: Routines for dealing with drawing the Speccy's screen via Xlib
   Copyright (c) 2000,2002 Philip Kendall, Darren Salt

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

#ifdef UI_X			/* Use this iff we're using Xlib */

#define X_USE_SHM

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef X_USE_SHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif /* X_USE_SHM */

#include "display.h"
#include "fuse.h"
#include "keyboard.h"
#include "xdisplay.h"
#include "xui.h"
#include "ui/uidisplay.h"
#include "widget/widget.h"

static XImage *image = 0;	/* The image structure to draw the
				   Speccy's screen on */
static GC gc;			/* A graphics context to draw with */

static unsigned long colours[16];

/* The current size of the window (in units of DISPLAY_SCREEN_*) */
static int xdisplay_current_size=1;

#ifdef X_USE_SHM
static XShmSegmentInfo shm_info;
int shm_eventtype;
int shm_finished;
#endif

static int shm_used = 0;

static int xdisplay_allocate_colours(int numColours, unsigned long *colours);
static int xdisplay_allocate_gc(Window window, GC *gc);
static int xdisplay_allocate_image(int width, int height);
static void xdisplay_destroy_image( void );
static void xdisplay_end( int );

int uidisplay_init(int width, int height)
{
  if(xdisplay_allocate_colours(16,colours)) return 1;
  if(xdisplay_allocate_gc(xui_mainWindow,&gc)) return 1;
  if(xdisplay_allocate_image(width,height)) return 1;

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

  currentMap=DefaultColormap(display,xui_screenNum);

  for(i=0;i<numColours;i++) {
    if( XParseColor(display, currentMap, colour_names[i], &colour) == 0 ) {
      fprintf(stderr,"%s: couldn't parse colour `%s'\n", fuse_progname,
	      colour_names[i]);
      return 1;
    }
    if( XAllocColor(display, currentMap, &colour) == 0 ) {
      fprintf(stderr,"%s: couldn't allocate colour `%s'\n", fuse_progname,
	      colour_names[i]);
      if(currentMap == DefaultColormap(display, xui_screenNum)) {
	fprintf(stderr,"%s: switching to private colour map\n", fuse_progname);
	currentMap=XCopyColormapAndFree(display, currentMap);
	XSetWindowColormap(display, xui_mainWindow, currentMap);
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
#ifdef X_USE_SHM
  key_t key = 'F' << 24 | 'u' << 16 | 's' << 8 | 'e';
  struct shmid_ds shm;
  int id = -1;
#endif /* X_USE_SHM */

  /* FIXME: should use sigaction */
  signal( SIGINT, xdisplay_end );

#ifdef X_USE_SHM
  shm_used = XShmQueryExtension( display );

  /* Don't try to use SHM if the display isn't local */
/*    if (shm_used && !ui_is_display_local ()) */
/*      shm_used = 0; */

  if( shm_used ) {
    shm_eventtype = XShmGetEventBase( display ) + ShmCompletion;
    image = XShmCreateImage( display, DefaultVisual( display, xui_screenNum ),
			     DefaultDepth( display, xui_screenNum ), ZPixmap,
			     0, &shm_info, 3 * width, 3 * height);
    if( !image ) shm_used = 0;
  }

  /* Claim some memory (try to reclaim a stale chunk) */
  if( shm_used )
  {
    int pollution = 5;
    const int size = image->bytes_per_line * image->height;
    do {

      id = shmget( key, size, 0777 );	/* just get the id */

      if( id == -1 ) id = shmget( key, size, IPC_CREAT | 0777 );
      else if( !shmctl( id, IPC_STAT, &shm ) ) {
	if( shm.shm_nattch ) key++;
	else {
	  if( getuid() == shm.shm_perm.cuid ) {
	    if( shmctl( id, IPC_RMID, 0) ) id = -1;
	    else {
	      id = shmget( key, size, IPC_CREAT | 0777 );
	      if( id != -1 ) shmctl (id, IPC_STAT, &shm);
	      break;
	    }
	  }
	  if( size >= shm.shm_segsz ) break;
	  key++;
	}
      }
    }
    while( id == -1 && --pollution );

    shm_used = ( id == -1 ? 0 : 1 );

    if( shm_used && image ) {
      shm_info.shmid = id;
      image->data = shm_info.shmaddr = shmat( id, 0, 0 );
      if( image->data ) {

	/* This may generate an X error */
	xerror_error = 0; xerror_expecting = 1;
	if( !XShmAttach( display, &shm_info ) ) xdisplay_destroy_image();

	/* Force any errors to occur before we disable traps */
	XSync( display, False );
	xerror_expecting = 0;

	/* If we caught an X error, don't use SHM */
	if( xerror_error ) shm_used = 0;

      } else {
	xdisplay_destroy_image();
      }
    } else {
      shm_used = 0;
    }
  }

  if( !shm_used && image ) xdisplay_destroy_image ();

#endif				/* #ifdef X_USE_SHM */

  /* If SHM isn't available, or we're not using it for some reason,
     just get a normal image */
  if( !shm_used ) {
    image=XCreateImage(display, DefaultVisual(display, xui_screenNum),
		       DefaultDepth(display, xui_screenNum), ZPixmap, 0, NULL,
		       3*width,3*height,8,0);
    if(!image) {
      fprintf(stderr,"%s: couldn't create image\n",fuse_progname);
      return 1;
    }

    if( (image->data=(char*)malloc(image->bytes_per_line*3*height)) == NULL ) {
      fprintf(stderr, "%s: out of memory for image data\n", fuse_progname);
      return 1;
    }
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
    uidisplay_set_border(y, 0, DISPLAY_SCREEN_WIDTH, display_border);
    uidisplay_set_border(DISPLAY_BORDER_HEIGHT+DISPLAY_HEIGHT+y, 0,
			 DISPLAY_SCREEN_WIDTH, display_border);
  }

  for(y=DISPLAY_BORDER_HEIGHT;y<DISPLAY_BORDER_HEIGHT+DISPLAY_HEIGHT;y++) {
    uidisplay_set_border(y, 0, DISPLAY_BORDER_WIDTH, display_border);
    uidisplay_set_border(y, DISPLAY_BORDER_WIDTH+DISPLAY_WIDTH,
			 DISPLAY_SCREEN_WIDTH, display_border);
  }

  return 0;
}

void uidisplay_putpixel(int x,int y,int colour)
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

void uidisplay_lines( int start, int end )
{
  xdisplay_area( 0, xdisplay_current_size * start,
		 xdisplay_current_size * DISPLAY_SCREEN_WIDTH,
		 xdisplay_current_size * ( end - start + 1 ) );
}

void xdisplay_area(int x, int y, int width, int height)
{
#ifdef X_USE_SHM
  if( shm_used ) {
    XShmPutImage( display, xui_mainWindow, gc, image,
		  x, y, x, y, width, height, True );
    /* FIXME: should wait for an ShmCompletion event here */
  } else {
    XPutImage(display, xui_mainWindow, gc, image, x, y, x, y, width, height);
  }
#else				/* #ifdef X_USE_SHM */
  XPutImage(display, xui_mainWindow, gc, image, x, y, x, y, width, height);
#endif				/* #ifdef X_USE_SHM */
}

void uidisplay_set_border(int line, int pixel_from, int pixel_to, int colour)
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

static void xdisplay_destroy_image (void)
{
  /* Free the XImage used to store screen data; also frees the malloc'd
     data */
#ifdef X_USE_SHM
  if( shm_used ) {
    XShmDetach( display, &shm_info );
    shmdt( shm_info.shmaddr );
    shmctl( shm_info.shmid, IPC_RMID, 0 );
    image->data = NULL;
    shm_used = 0;
  }
#endif
  if( image ) XDestroyImage( image ); image = 0;
}

static void xdisplay_end (int sig)
{
  uidisplay_end();
  psignal( sig, fuse_progname );
  exit( 1 );
}

int uidisplay_end(void)
{
  xdisplay_destroy_image();
  /* Free the allocated GC */
  if( gc ) XFreeGC( display, gc ); gc = 0;

  return 0;
}

#endif				/* #ifdef UI_X */
