/* widget_internals.h: Functions internal to the widget code
   Copyright (c) 2001-2004 Matan Ziv-Av, Philip Kendall

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

#ifndef FUSE_WIDGET_INTERNALS_H
#define FUSE_WIDGET_INTERNALS_H

#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>

#include <libspectrum.h>

#ifndef FUSE_SETTINGS_H
#include "settings.h"
#endif				/* #ifndef FUSE_SETTINGS_H */

#ifndef FUSE_WIDGET_H
#include "widget.h"
#endif				/* #ifndef FUSE_WIDGET_H */

/* The default colours used in the widget */
#define WIDGET_COLOUR_BACKGROUND 1	/* Blue */
#define WIDGET_COLOUR_FOREGROUND 7	/* White */

/* The ways of finishing a widget */
typedef enum widget_finish_state {
  WIDGET_FINISHED_OK = 1,
  WIDGET_FINISHED_CANCEL,
} widget_finish_state;

/* A function to draw a widget */
typedef int (*widget_draw_fn)( void *data );

/* The information we need to store for each widget */
typedef struct widget_t {
  widget_draw_fn draw;			/* Draw this widget */
  int (*finish)( widget_finish_state finished ); /* Post-widget processing */
  widget_keyhandler_fn keyhandler;	/* Keyhandler */
} widget_t;

int widget_end_widget( widget_finish_state state );
int widget_end_all( widget_finish_state state );

int widget_timer_init( void );
int widget_timer_end( void );

void widget_rectangle( int x, int y, int w, int h, int col );
void widget_printstring( int x, int y, int col, const char *s );
void widget_display_lines( int y, int h );

extern widget_finish_state widget_finished;

int widget_dialog( int x, int y, int width, int height );
int widget_dialog_with_border( int x, int y, int width, int height );

/* File selector */

typedef struct widget_dirent {
  int mode;
  char *name;
} widget_dirent;

extern struct widget_dirent **widget_filenames;
extern size_t widget_numfiles;

int widget_filesel_draw( void* data );
int widget_filesel_finish( widget_finish_state finished );
void widget_filesel_keyhandler( input_key key );

/* Tape menu */

int widget_tape_draw( void* data );
void widget_tape_keyhandler( input_key key );

/* File menu */

int widget_file_draw( void* data );
void widget_file_keyhandler( input_key key );

/* Options menu */
int widget_menu_filter( void *data );

/* Machine menu */

int widget_machine_draw( void* data );
void widget_machine_keyhandler( input_key key );

/* Keyboard picture */

typedef struct widget_picture_data {
  const char *filename;
  libspectrum_byte *screen;
  int border;
} widget_picture_data;

int widget_picture_draw( void* data );
void widget_picture_keyhandler( input_key key );

/* Help menu */

int widget_help_draw( void* data );
void widget_help_keyhandler( input_key key );

/* General menu code */

int widget_menu_draw( void* data );
void widget_menu_keyhandler( input_key key );

/* General callbacks */

/* The callback function to call another widget */
int widget_menu_widget( void *data );

/* The data type passed to widget_menu_widget */
typedef struct widget_menu_widget_t {
  widget_type widget;	/* The widget to call */
  void *data;		/* with this data parameter */
} widget_menu_widget_t;

/* More callbacks */
int widget_menu_open( const char *filename ); /* File/Open */
int widget_menu_save_snapshot( void *data ); /* File/Save Snapshot */
int widget_menu_rzx_recording( void *data ); /* File/Recording/Record */
int widget_menu_rzx_recording_snap( void *data ); /* File/Recording/Record
						     from snap */
int widget_menu_rzx_playback( void *data );  /* File/Recording/Play */
int widget_menu_rzx_stop( void *data );	     /* File/Recording/Stop */
int widget_menu_psg_record( void *data );    /* File/AY Logging/Record */
int widget_menu_psg_stop( void *data );	     /* File/AY Logging/Stop */
int widget_menu_save_screen( void *data );   /* File/Save Screenshot */
int widget_menu_save_scr( void *data );	     /* File/Save Scr */
int widget_menu_exit( void *data );	     /* File/Exit */

int widget_menu_joystick( void *data );	     /* Options/Joysticks/<which> */
int widget_menu_save_options( void *data );  /* Options/Save */
int widget_menu_select_roms( void *data );   /* Options/Select ROMs/<type> */

int widget_menu_reset( void *data );	     /* Machine/Reset */
int widget_menu_break( void *data );	     /* Machine/Break */
int widget_menu_nmi( void *data );	     /* Machine/NMI */

int widget_menu_play_tape( void *data );     /* Tape/Play */
int widget_menu_rewind_tape( void *data );   /* Tape/Rewind */
int widget_menu_clear_tape( void *data );    /* Tape/Close */
int widget_menu_write_tape( void *data );    /* Tape/Write */

int widget_insert_disk_a( const char *filename ); /* Disk/Drive A:/Insert */
int widget_insert_disk_b( const char *filename ); /* Disk/Drive B:/Insert */
int widget_menu_eject_disk( void *data );    /* Disk/Drive ?:/Eject */
int widget_menu_eject_write_disk( void *data ); /* Disk/Drive ?:/Eject and
						   write */

int widget_insert_ide_simple_master( const char *filename );
int widget_insert_ide_simple_slave( const char *filename );
int widget_menu_commit_ide_simple( void *data );
int widget_menu_eject_ide_simple( void *data );

int widget_insert_dock( const char *filename ); /* Cart/Timex Dock/Insert */
int widget_menu_eject_dock( void *data );    /* Cart/Timex Dock/Eject */

int widget_menu_keyboard( void *data );	     /* Help/Keyboard Picture */

scaler_type widget_select_scaler( int (*selector)( scaler_type ) );

/* The generalised selector widget */

typedef struct widget_select_t {

  const char *title;	/* Dialog title */
  const char **options;	/* The available options */
  size_t count;		/* The number of options */
  size_t current;	/* Which option starts active? */

  int result;		/* What was selected? ( -1 if dialog cancelled ) */

} widget_select_t;

int widget_select_draw( void *data );
void widget_select_keyhandler( input_key key );
int widget_select_finish( widget_finish_state finished );

/* The tape browser widget */

int widget_browse_draw( void* data );
void widget_browse_keyhandler( input_key key );
int widget_browse_finish( widget_finish_state finished );

/* The text entry widget */

typedef struct widget_text_t {
  const char *title;
  char text[40];
} widget_text_t;

int widget_text_draw( void* data );
void widget_text_keyhandler( input_key key );
int widget_text_finish( widget_finish_state finished );

extern char *widget_text_text;	/* The returned text */

/* General functions used by options dialogs */
extern settings_info widget_options_settings;
int widget_options_print_option( int number, const char* string, int value );
int widget_options_print_value( int number, int value );
int widget_options_print_entry( int number, const char *prefix, int value,
				const char *suffix );
int widget_options_finish( widget_finish_state finished );

/* The error widget */

int widget_error_draw( void *data );
void widget_error_keyhandler( input_key key );

/* The debugger widget */

int widget_debugger_draw( void *data );
void widget_debugger_keyhandler( input_key key );

/* The ROM selector widget */

typedef struct widget_roms_info {

  int initialised;

  libspectrum_machine machine;
  size_t start, count;

} widget_roms_info;

int widget_roms_draw( void *data );
void widget_roms_keyhandler( input_key key );
int widget_roms_finish( widget_finish_state finished );

/* The widgets actually available */

extern widget_t widget_data[];

#endif				/* #ifndef FUSE_WIDGET_INTERNALS_H */
