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

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef HAVE_SIGINFO_H
#include <siginfo.h>		/* Needed for psignal on Solaris */
#endif				/* #ifdef HAVE_SIGINFO_H */

#ifdef HAVE_X11_EXTENSIONS_XSHM_H
#define X_USE_SHM
#endif				/* #ifdef HAVE_X11_EXTENSIONS_XSHM_H */

#ifdef X_USE_SHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif				/* #ifdef X_USE_SHM */

#include "display.h"
#include "fuse.h"
#include "keyboard.h"
#include "screenshot.h"
#include "xdisplay.h"
#include "xui.h"
#include "ui/uidisplay.h"
#ifdef USE_WIDGET
#include "widget/widget.h"
#endif				/* #ifdef USE_WIDGET */
#include "scld.h"

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

static int try_shm( const int width, const int height );
static int get_shm_id( const int size );
#endif				/* #ifdef X_USE_SHM */

static int shm_used = 0;

static int xdisplay_allocate_colours( int numColours,
				      unsigned long *colour_values );
static int xdisplay_allocate_gc( Window window, GC *new_gc );

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

static int
xdisplay_allocate_colours( int numColours, unsigned long *colour_values )
{
  XColor colour;
  Colormap currentMap;

  const char *colour_names[] = {
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
    colour_values[i] = colour.pixel;
  }

  return 0;
}
  
static int
xdisplay_allocate_gc( Window window, GC *new_gc )
{
  unsigned valuemask=0;
  XGCValues values;

  *new_gc = XCreateGC( display, window, valuemask, &values );

  return 0;
}

static int xdisplay_allocate_image(int width, int height)
{
  struct sigaction handler;

  handler.sa_handler = xdisplay_end;
  sigemptyset( &handler.sa_mask );
  handler.sa_flags = 0;
  sigaction( SIGINT, &handler, NULL );

#ifdef X_USE_SHM
  shm_used = try_shm( width, height );
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

#ifdef X_USE_SHM
static int
try_shm( const int width, const int height )
{
  int id;
  int error;

  if( !XShmQueryExtension( display ) ) return 0;

  shm_eventtype = XShmGetEventBase( display ) + ShmCompletion;
  image = XShmCreateImage( display, DefaultVisual( display, xui_screenNum ),
			   DefaultDepth( display, xui_screenNum ), ZPixmap,
			   0, &shm_info, 3 * width, 3 * height);
  if( !image ) return 0;

  /* Get an SHM to work with */
  id = get_shm_id( image->bytes_per_line * image->height );
  if( id == -1 ) return 0;

  /* Attempt to attach to the shared memory */
  shm_info.shmid = id;
  image->data = shm_info.shmaddr = shmat( id, 0, 0 );

  /* If we couldn't attach, remove the chunk and give up */
  if( image->data == (void*)-1 ) {
    shmctl( id, IPC_RMID, NULL );
    image->data = NULL;
    return 0;
  }

  /* This may generate an X error */
  xerror_error = 0; xerror_expecting = 1;
  error = !XShmAttach( display, &shm_info );

  /* Force any X errors to occur before we disable traps */
  XSync( display, False );
  xerror_expecting = 0;

  /* If we caught an error, don't use SHM */
  if( error || xerror_error ) {
    shmctl( id, IPC_RMID, NULL );
    shmdt( image->data ); image->data = NULL;
    return 0;
  }

  /* Now flag the chunk for deletion; this will take effect when
     everything has detached from it */
  shmctl( id, IPC_RMID, NULL );

  return 1;
}  

/* Get an SHM ID; also attempt to reclaim any stale chunks we find */
static int
get_shm_id( const int size )
{
  key_t key = 'F' << 24 | 'u' << 16 | 's' << 8 | 'e';
  struct shmid_ds shm;

  int id;

  int pollution = 5;
  
  do {
    /* See if a chunk already exists with this key */
    id = shmget( key, size, 0777 );

    /* If the chunk didn't already exist, try and create one for our
       use */
    if( id == -1 ) {
      id = shmget( key, size, IPC_CREAT | 0777 );
      continue;			/* And then jump to the end of the loop */
    }

    /* If the chunk already exists, try and get information about it */
    if( shmctl( id, IPC_STAT, &shm ) != -1 ) {

      /* If something's actively using this chunk, try another key */
      if( shm.shm_nattch ) {
	key++;
      } else {		/* Otherwise, attempt to remove the chunk */
	
	/* If we couldn't remove that chunk, try another key. If we
	   could, just try again */
	if( shmctl( id, IPC_RMID, NULL ) != 0 ) key++;
      }
    } else {		/* Couldn't get info on the chunk, so try next key */
      key++;
    }
    
    id = -1;		/* To prevent early exit from loop */

  } while( id == -1 && --pollution );

  return id;
}
#endif			/* #ifdef X_USE_SHM */

int
xdisplay_configure_notify( int width, int height GCC_UNUSED )
{
  int size, colour;

  colour = scld_last_dec.name.hires ? display_hires_border :
                                      display_lores_border;
  size = width / DISPLAY_ASPECT_WIDTH;

  /* If we're the same size as before, nothing special needed */
  if( size == xdisplay_current_size ) return 0;

  /* Else set ourselves to the new height */
  xdisplay_current_size=size;

  /* Redraw the entire screen... */
  display_refresh_all();

  /* If widgets are active, redraw the widget */
  if( widget_level >= 0 ) widget_keyhandler( KEYBOARD_Resize, KEYBOARD_NONE );

  return 0;
}

void uidisplay_putpixel(int x,int y,int colour)
{
#ifdef USE_LIBPNG
  screenshot_screen[y][x] = colour;
#endif			/* #ifdef USE_LIBPNG */

  switch(xdisplay_current_size) {
  case 1:
    if(x%2!=0) return;
    XPutPixel(image, x>>1,  y    ,colours[colour]);
    break;
  case 2:
    XPutPixel(image, x  ,   y<<1 ,colours[colour]);
    XPutPixel(image, x  ,(y<<1)+1,colours[colour]);
    break;
  }
}

void
uidisplay_frame_end( void ) 
{
  return;
}

void
uidisplay_area( int x, int y, int w, int h )
{
  xdisplay_area( xdisplay_current_size * x, xdisplay_current_size * y,
		 xdisplay_current_size * w, xdisplay_current_size * h );
}

void
xdisplay_area( int x, int y, int w, int h )
{
  if( shm_used ) {
#ifdef X_USE_SHM
    XShmPutImage( display, xui_mainWindow, gc, image, x, y, x, y, w, h, True );
    /* FIXME: should wait for an ShmCompletion event here */
#endif				/* #ifdef X_USE_SHM */
  } else {
    XPutImage( display, xui_mainWindow, gc, image, x, y, x, y, w, h );
  }
}

void uidisplay_set_border(int line, int pixel_from, int pixel_to, int colour)
{
  int x;
  
  for(x=pixel_from;x<pixel_to;x++) {
    uidisplay_putpixel(x,line,colour);
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
