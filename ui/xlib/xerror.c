/* xerror.c: handle X errors
   Copyright (c) 2002-2004 Philip Kendall

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

#ifdef UI_X                     /* Use this iff we're using Xlib */

#include <stdarg.h>
#include <stdio.h>

#include <X11/Xlib.h>

#include "fuse.h"
#ifdef USE_WIDGET
#include "widget/widget.h"
#endif				/* #ifdef USE_WIDGET */

/* Are we expecting an X error to occur? */
int xerror_expecting;

/* Which error occured? */
int xerror_error;

#define MESSAGE_MAX_LENGTH 256

int
xerror_handler( Display *display, XErrorEvent *error );

int
xerror_handler( Display *display, XErrorEvent *error )
{
  /* If we were expecting an error to occur, just set a flag. Otherwise,
     exit in a fairly spectacular fashion */
  if( xerror_expecting ) {
    xerror_error = error->error_code;
  } else {

    char message[64];

    XGetErrorText( display, error->error_code, message, 63 );

    fprintf( stderr, "X Error: %s\n", message );

    exit( 1 );

  }

  return 0;
}

#endif				/* #ifdef UI_X */
