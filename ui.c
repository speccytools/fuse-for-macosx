/* ui.c: User interface routines, but those which are independent of any UI
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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libspectrum.h>

#include "fuse.h"
#include "ui/ui.h"

#define MESSAGE_MAX_LENGTH 256

static char last_message[ MESSAGE_MAX_LENGTH ] = "";
static size_t frames_since_last_message = 0;

static int
print_error_to_stderr( ui_error_level severity, const char *message );

int
ui_error( ui_error_level severity, const char *format, ... )
{
  int error;
  va_list ap;

  va_start( ap, format );
  error = ui_verror( severity, format, ap );
  va_end( ap );

  return error;
}

int
ui_verror( ui_error_level severity, const char *format, va_list ap )
{
  char message[ MESSAGE_MAX_LENGTH ];

  vsnprintf( message, MESSAGE_MAX_LENGTH, format, ap );

  /* Skip the message if the same message was displayed recently */
  if( frames_since_last_message < 50 && !strcmp( message, last_message ) ) {
    frames_since_last_message = 0;
    return 0;
  }

  /* And store the 'last message' */
  strncpy( last_message, message, MESSAGE_MAX_LENGTH );

#ifndef UI_WIN32
  print_error_to_stderr( severity, message );
#endif			/* #ifndef UI_WIN32 */

  /* Do any UI-specific bits as well */
  ui_error_specific( severity, message );

  return 0;
}

static int
print_error_to_stderr( ui_error_level severity, const char *message )
{
  /* Print the error to stderr if it's more significant than just
     informational */
  if( severity > UI_ERROR_INFO ) {

    /* For the fb and svgalib UIs, we don't want to write to stderr if
       it's a terminal, as it's then likely to be what we're currently
       using for graphics output, and writing text to it isn't a good
       idea. Things are OK if we're exiting though */
#if defined( UI_FB ) || defined( UI_SVGA )
    if( isatty( STDERR_FILENO ) && !fuse_exiting ) return 1;
#endif			/* #if defined( UI_FB ) || defined( UI_SVGA ) */

    fprintf( stderr, "%s: ", fuse_progname );

    switch( severity ) {

    case UI_ERROR_INFO: break;		/* Shouldn't happen */

    case UI_ERROR_WARNING: fprintf( stderr, "warning: " ); break;
    case UI_ERROR_ERROR: fprintf( stderr, "error: " ); break;
    }

    fprintf( stderr, "%s\n", message );
  }

  return 0;
}

libspectrum_error
ui_libspectrum_error( libspectrum_error error GCC_UNUSED, const char *format,
		      va_list ap )
{
  char new_format[ 257 ];
  snprintf( new_format, 256, "libspectrum: %s", format );

  ui_verror( UI_ERROR_ERROR, new_format, ap );

  return LIBSPECTRUM_ERROR_NONE;
}

void
ui_error_frame( void )
{
  frames_since_last_message++;
}
