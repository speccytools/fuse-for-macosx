/* error.c: handle errors
   Copyright (c) 2002 Philip Kendall

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

#ifdef UI_SVGA			/* Use this iff we're using SVGAlib */

#include <stdarg.h>
#include <stdio.h>

#include "fuse.h"
#include "widget.h"

#define MESSAGE_MAX_LENGTH 256

int
ui_error( const char *format, ... )
{
  char message[ MESSAGE_MAX_LENGTH + 1 ];
  va_list ap;
  
  /* Create the message from the given arguments */
  va_start( ap, format );
  vsnprintf( message, MESSAGE_MAX_LENGTH, format, ap );
  va_end( ap );

  /* Print the message to stderr, along with a program identifier */
  fprintf( stderr, "%s: error: %s\n", fuse_progname, message );

  fuse_emulation_pause();
  widget_do( WIDGET_TYPE_ERROR, message );
  fuse_emulation_unpause();

  return 0;
}

#endif				/* #ifdef UI_SVGA */
