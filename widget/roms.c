/* roms.c: select ROMs widget
   Copyright (c) 2003 Philip Kendall

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

int
widget_roms_draw( void *data )
{
  int i, error;
  char buffer[32];
  const char *setting; size_t length;

  /* Get a copy of the current settings, unless we're given one already */
  if( !data ) {
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
  } else {
    widget_settings = data;
  }

  /* Blank the main display area */
  widget_dialog_with_border( 1, 1, 30, SETTINGS_ROM_COUNT + 2 );

  widget_printstring( 10, 1, WIDGET_COLOUR_FOREGROUND, "Select ROMs" );

  for( i=0; i<SETTINGS_ROM_COUNT; i++ ) {

    /* Get the last twelve characters of the ROM filename */
    setting = *( settings_get_rom_setting( widget_settings, i ) );
    length = strlen( setting ); if( length > 12 ) setting += length - 12;

    snprintf( buffer, 32, "(%c) %10s: %s", ((char)i)+'A',
	      settings_rom_name[i], setting );

    widget_printstring( 2, i+3, WIDGET_COLOUR_FOREGROUND, buffer );
  }

  widget_display_lines( 1, SETTINGS_ROM_COUNT + 2 );

  return 0;
}

void
widget_roms_keyhandler( keyboard_key_name key, keyboard_key_name key2 )
{
  switch( key ) {

  case KEYBOARD_Resize:		/* Fake keypress used on window resize */
    widget_select_draw( widget_settings );
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
      key - KEYBOARD_a < (ptrdiff_t)SETTINGS_ROM_COUNT ) {

    char **setting;
    int error;
    int constant_zero = 0;

    widget_do( WIDGET_TYPE_FILESELECTOR, &constant_zero );
    if( !widget_filesel_name ) return;

    setting = settings_get_rom_setting( widget_settings, key - KEYBOARD_a );
    error = settings_set_string( setting, widget_filesel_name );
    if( error ) return;
  
    widget_roms_draw( widget_settings );
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
