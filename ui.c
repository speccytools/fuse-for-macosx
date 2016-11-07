/* ui.c: User interface routines, but those which are independent of any UI
   Copyright (c) 2002-2015 Philip Kendall
   Copyright (c) 2016 Sergio Baldoví

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libspectrum.h>

#include "fuse.h"
#include "peripherals/if1.h"
#include "peripherals/kempmouse.h"
#include "settings.h"
#include "tape.h"
#include "ui/ui.h"
#include "ui/uimedia.h"
#include "ui/widget/widget.h"

#define MESSAGE_MAX_LENGTH 256

/* We don't start in a widget */
int ui_widget_level = -1;

static char last_message[ MESSAGE_MAX_LENGTH ] = "";
static size_t frames_since_last_message = 0;

#ifndef UI_WIN32
static int
print_error_to_stderr( ui_error_level severity, const char *message );
#endif			/* #ifndef UI_WIN32 */

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
  last_message[ MESSAGE_MAX_LENGTH - 1 ] = '\0';

#ifndef UI_WIN32
  print_error_to_stderr( severity, message );
#endif			/* #ifndef UI_WIN32 */

  /* Do any UI-specific bits as well */
  ui_error_specific( severity, message );

  return 0;
}

ui_confirm_save_t
ui_confirm_save( const char *format, ... )
{
  va_list ap;
  char message[ MESSAGE_MAX_LENGTH ];
  ui_confirm_save_t confirm;

  va_start( ap, format );

  vsnprintf( message, MESSAGE_MAX_LENGTH, format, ap );
  confirm = ui_confirm_save_specific( message );

  va_end( ap );

  return confirm;
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

int ui_mouse_present = 0;
int ui_mouse_grabbed = 0;
static int mouse_grab_suspended = 0;

void
ui_mouse_button( int button, int down )
{
  int kempston_button = !settings_current.mouse_swap_buttons;

  if( !ui_mouse_grabbed && !mouse_grab_suspended ) button = 2;

  /* Possibly we'll end up handling _more_ than one mouse interface... */
  switch( button ) {
  case 1:
    if( ui_mouse_grabbed ) kempmouse_update( 0, 0, kempston_button, down );
    break;
  case 3:
    if( ui_mouse_grabbed ) kempmouse_update( 0, 0, !kempston_button, down );
    break;
  case 2:
    if( ui_mouse_present && settings_current.kempston_mouse
	&& !down && !mouse_grab_suspended )
      ui_mouse_grabbed =
	ui_mouse_grabbed ? ui_mouse_release( 1 ) : ui_mouse_grab( 0 );
    break;
  }
}

void
ui_mouse_motion( int x, int y )
{
  if( ui_mouse_grabbed ) kempmouse_update( x, y, -1, 0 );
}

void
ui_mouse_suspend( void )
{
  mouse_grab_suspended = ui_mouse_grabbed ? 2 : 1;
  ui_mouse_grabbed = ui_mouse_release( 1 );
}

void
ui_mouse_resume( void )
{
  if( mouse_grab_suspended == 2) ui_mouse_grabbed = ui_mouse_grab( 0 );
  mouse_grab_suspended = 0;
}

void
ui_menu_disk_update( void )
{
  int drives_avail;

  drives_avail = ui_media_drive_any_available();

  /* Set the disk menu items and statusbar appropriately */

  if( drives_avail ) {
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK, 1 );
    ui_statusbar_update( UI_STATUSBAR_ITEM_DISK, UI_STATUSBAR_STATE_INACTIVE );
  } else {
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK, 0 );
    ui_statusbar_update( UI_STATUSBAR_ITEM_DISK,
                         UI_STATUSBAR_STATE_NOT_AVAILABLE );
  }

  ui_media_drive_update_parent_menus();
}

#ifdef USE_WIDGET
int
ui_widget_init( void )
{
    return widget_init();
}

int
ui_widget_end( void )
{
    return widget_end();
}
#else
int
ui_widget_init( void )
{
    return 0;
}

int
ui_widget_end( void )
{
    return 0;
}

void
ui_popup_menu( int native_key )
{
}

void
ui_widget_keyhandler( int native_key )
{
}
#endif                          /* #ifndef USE_WIDGET */
