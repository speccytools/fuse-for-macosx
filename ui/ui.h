/* ui.h: General UI event handling routines
   Copyright (c) 2000-2004 Philip Kendall

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

#ifndef FUSE_UI_H
#define FUSE_UI_H

#include <stdarg.h>

#include <libspectrum.h>

#include "compat.h"
#include "machines/specplus3.h"
#include "trdos.h"
#include "ui/scaler/scaler.h"

/* The various severities of error level, increasing downwards */
typedef enum ui_error_level {

  UI_ERROR_INFO,		/* Informational message */
  UI_ERROR_WARNING,		/* Something is wrong, but it's not that
				   important */
  UI_ERROR_ERROR,		/* An actual error */

} ui_error_level;

int ui_init(int *argc, char ***argv);
int ui_event(void);
int ui_end(void);

/* Error handling routines */
int ui_error( ui_error_level severity, const char *format, ... )
     GCC_PRINTF( 2, 3 );
libspectrum_error ui_libspectrum_error( libspectrum_error error,
					const char *format, va_list ap );
int ui_verror( ui_error_level severity, const char *format, va_list ap );
int ui_error_specific( ui_error_level severity, const char *message );
void ui_error_frame( void );

/* Callbacks used by the debugger */
int ui_debugger_activate( void );
int ui_debugger_deactivate( int interruptable );
int ui_debugger_update( void );
int ui_debugger_disassemble( libspectrum_word address );

/* Functions defined in ../ui.c */

/* Confirm whether we want to save some data before overwriting it */
typedef enum ui_confirm_save_t {

  UI_CONFIRM_SAVE_SAVE,		/* Save the data */
  UI_CONFIRM_SAVE_DONTSAVE,	/* Don't save the data */
  UI_CONFIRM_SAVE_CANCEL,	/* Cancel the action */

} ui_confirm_save_t;

ui_confirm_save_t ui_confirm_save( const char *message );

/* Mouse handling */

extern int ui_mouse_present, ui_mouse_grabbed;
void ui_mouse_suspend( void );
void ui_mouse_resume( void );
void ui_mouse_button( int button, int down );
void ui_mouse_motion( int dx, int dy );
int ui_mouse_grab( int startup ); /* UI: grab, return 1 if done */
int ui_mouse_release( int suspend ); /* UI: ungrab, return 0 if done */

/* Write the current tape out */
int ui_tape_write( void );

/* Write a +3 or TRDOS disk out */
int ui_plus3_disk_write( specplus3_drive_number which );
int ui_trdos_disk_write( trdos_drive_number which );

/* Routines to (de)activate certain menu items */

typedef enum ui_menu_item {

  UI_MENU_ITEM_MEDIA_CARTRIDGE_DOCK,
  UI_MENU_ITEM_MEDIA_CARTRIDGE_DOCK_EJECT,
  UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2,
  UI_MENU_ITEM_MEDIA_CARTRIDGE_IF2_EJECT,
  UI_MENU_ITEM_MEDIA_DISK,
  UI_MENU_ITEM_MEDIA_DISK_A_EJECT,
  UI_MENU_ITEM_MEDIA_DISK_B_EJECT,
  UI_MENU_ITEM_MEDIA_IDE,
  UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT,
  UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_MASTER_EJECT,
  UI_MENU_ITEM_MEDIA_IDE_SIMPLE8BIT_SLAVE_EJECT,
  UI_MENU_ITEM_MEDIA_IDE_ZXATASP,
  UI_MENU_ITEM_MEDIA_IDE_ZXATASP_MASTER_EJECT,
  UI_MENU_ITEM_MEDIA_IDE_ZXATASP_SLAVE_EJECT,
  UI_MENU_ITEM_MEDIA_IDE_ZXCF,
  UI_MENU_ITEM_MEDIA_IDE_ZXCF_EJECT,
  UI_MENU_ITEM_RECORDING,
  UI_MENU_ITEM_AY_LOGGING,

} ui_menu_item;

int ui_menu_activate( ui_menu_item item, int active );

/* Functions to update the statusbar */

typedef enum ui_statusbar_item {

  UI_STATUSBAR_ITEM_DISK,
  UI_STATUSBAR_ITEM_PAUSED,
  UI_STATUSBAR_ITEM_TAPE,

} ui_statusbar_item;

typedef enum ui_statusbar_state {

  UI_STATUSBAR_STATE_NOT_AVAILABLE,
  UI_STATUSBAR_STATE_INACTIVE,
  UI_STATUSBAR_STATE_ACTIVE,

} ui_statusbar_state;

int ui_statusbar_update( ui_statusbar_item item, ui_statusbar_state state );
int ui_statusbar_update_speed( float speed );

/* Cause the tape browser to be updated */
int ui_tape_browser_update( void );

#endif			/* #ifndef FUSE_UI_H */
