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

#ifdef UI_GGI			/* Use this iff we're using GGI */

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

#include "fuse.h"
#include "ui/ui.h"
#ifdef USE_WIDGET
#include "widget/widget.h"
#endif				/* #ifdef USE_WIDGET */

#define MESSAGE_MAX_LENGTH 256

int
ui_verror( ui_error_level severity, const char *format, va_list ap )
{
  char message[ MESSAGE_MAX_LENGTH + 1 ];
  widget_error_t error_info;
  
  /* Create the message from the given arguments */
  vsnprintf( message, MESSAGE_MAX_LENGTH, format, ap );

  /* If this is a 'severe' error, print it to stderr with a program
     identifier and a level indicator */
  if( severity > UI_ERROR_INFO ) {

    fprintf( stderr, "%s: ", fuse_progname );

    switch( severity ) {
    case UI_ERROR_WARNING:
      fprintf( stderr, "warning: " ); break;
    case UI_ERROR_ERROR:
      fprintf( stderr, "error: " ); break;
    default:
      fprintf( stderr, "(unknown level): " ); break;
    }

    fprintf( stderr, "%s\n", message );

  }

  error_info.severity = severity;
  error_info.message  = message;

  fuse_emulation_pause();
  widget_do( WIDGET_TYPE_ERROR, &error_info );
  fuse_emulation_unpause();

  return 0;
}

#endif				/* #ifdef UI_GGI */
