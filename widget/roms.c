/* roms.c: select ROMs widget
   Copyright (c) 2003-2004 Philip Kendall

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

#ifdef USE_WIDGET

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "fuse.h"
#include "keyboard.h"
#include "settings.h"
#include "widget_internals.h"

static settings_info *widget_settings;
static widget_roms_info *info;

static size_t first_rom, rom_count;

static void print_rom( int which );

int
widget_roms_draw( void *data )
{
  int i, error;
  char buffer[32];
  const char *name;

  if( data ) info = data;

  /* Get a copy of the current settings */
  if( !info->initialised ) {

    widget_settings = malloc( sizeof( settings_info ) );
    if( !widget_settings ) {
      ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
      return 1;
    }
    error = settings_copy( widget_settings, &settings_current );
    if( error ) {
      settings_free( widget_settings ); free( widget_settings );
      return error;
    }

    info->initialised = 1;
  }

  first_rom = info->start;
  rom_count = info->count;

  /* Blank the main display area */
  widget_dialog_with_border( 1, 2, 30, rom_count + 3 );

  name = libspectrum_machine_name( info->machine );
  widget_printstring( 15 - strlen( name ) / 2, 2, WIDGET_COLOUR_FOREGROUND,
		      name );

  for( i = 0; i < info->count; i++ ) {

    snprintf( buffer, 32, "(%c) %10s:", ((char)i) + 'A',
	      settings_rom_name[i] );
    widget_printstring( 2, i + 4, WIDGET_COLOUR_FOREGROUND, buffer );

    print_rom( i );
  }

  return 0;
}

static void
print_rom( int which )
{
  const char *setting;
  size_t length;

  setting = *( settings_get_rom_setting( widget_settings,
					 which + first_rom ) );
  length = strlen( setting ); if( length > 12 ) setting += length - 12;

  widget_rectangle( 18 * 8, ( which + 4 ) * 8, 12 * 8, 1 * 8,
		    WIDGET_COLOUR_BACKGROUND );
  widget_printstring( 18, which + 4, WIDGET_COLOUR_FOREGROUND, setting );
  widget_display_lines( which + 4, 1 );
}

void
widget_roms_keyhandler( keyboard_key_name key, keyboard_key_name key2 )
{
  switch( key ) {

  case KEYBOARD_Resize:		/* Fake keypress used on window resize */
    widget_roms_draw( NULL );
    break;

  case KEYBOARD_1: /* 1 used as `Escape' generates `Edit', which is Caps + 1 */
    if( key2 == KEYBOARD_Caps ) widget_end_widget( WIDGET_FINISHED_CANCEL );
    return;

  case KEYBOARD_Enter:
    widget_end_all( WIDGET_FINISHED_OK );
    return;

  default:	/* Keep gcc happy */
    break;

  }

  if( key >= KEYBOARD_a && key <= KEYBOARD_z &&
      key - KEYBOARD_a < (ptrdiff_t)rom_count ) {

    char **setting;
    int error;
    int constant_zero = 0;

    key -= KEYBOARD_a;

    widget_do( WIDGET_TYPE_FILESELECTOR, &constant_zero );
    if( !widget_filesel_name ) return;

    setting = settings_get_rom_setting( widget_settings, key + first_rom );
    error = settings_set_string( setting, widget_filesel_name );
    if( error ) return;

    print_rom( key );
  }
}

int
widget_roms_finish( widget_finish_state finished )
{
  int error;

  if( finished == WIDGET_FINISHED_OK ) {
    error = settings_copy( &settings_current, widget_settings );
    if( error ) return error;
  }

  settings_free( widget_settings ); free( widget_settings );
  return 0;
}

#endif				/* #ifdef USE_WIDGET */
