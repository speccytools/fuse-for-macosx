/* widget.h: Simple dialog boxes for all user interfaces.
   Copyright (c) 2001 Matan Ziv-Av, Philip Kendall

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

#ifndef FUSE_WIDGET_H
#define FUSE_WIDGET_H

#ifndef _DIRENT_H
#include <dirent.h>
#endif				/* #ifndef _DIRENT_H */

/* The default colours used in the widget */
#define WIDGET_COLOUR_BACKGROUND 1	/* Blue */
#define WIDGET_COLOUR_FOREGROUND 7	/* White */

/* The various widgets which are available */
typedef enum widget_type {

  WIDGET_TYPE_MAINMENU,		/* Main menu */
  WIDGET_TYPE_FILESELECTOR,	/* File selector */
  WIDGET_TYPE_OPTIONS,		/* General options */
  WIDGET_TYPE_TAPE,		/* Tape options */
  WIDGET_TYPE_SNAPSHOT,		/* Snapshot options */

} widget_type;

/* The ways of finishing a widget */
typedef enum widget_finish_state {
  WIDGET_FINISHED_OK = 1,
  WIDGET_FINISHED_CANCEL,
} widget_finish_state;

/* A function to handle keypresses */
typedef void (*widget_keyhandler_fn)( int key );

/* The information we need to store for each widget */
typedef struct widget_t {
  int (*draw)(void);			/* Draw this widget */
  int (*finish)( widget_finish_state finished ); /* Post-widget processing */
  widget_keyhandler_fn keyhandler;	/* Keyhandler */
} widget_t;

/* The information we need to store to recurse from a widget */
typedef struct widget_recurse_t {
  widget_type type;		/* Which type of widget are we? */
  int finished;			/* Have we finished this widget yet? */
} widget_recurse_t;

/* A `stack' so we can recurse through widgets */
widget_recurse_t widget_return[10];

/* How many levels deep have we recursed through widgets; -1 => none */
extern int widget_level;

/* The current widget keyhandler */
widget_keyhandler_fn widget_keyhandler;

int widget_init( void );
int widget_end( void );

int widget_do( widget_type which );

int widget_timer_init( void );
int widget_timer_end( void );

void widget_rectangle( int x, int y, int w, int h, int col );
void widget_printstring( int x, int y, int col, char *s );

extern widget_finish_state widget_finished;;

int widget_dialog( int x, int y, int width, int height );
int widget_dialog_with_border( int x, int y, int width, int height );

/* Main menu dialog */

int widget_mainmenu_draw( void );
void widget_mainmenu_keyhandler( int key );

/* File selector */

typedef struct widget_dirent {
  int mode;
  char *name;
} widget_dirent;

struct widget_dirent **widget_filenames;
size_t widget_numfiles;

int widget_filesel_draw( void );
int widget_filesel_finish( widget_finish_state finished );
void widget_filesel_keyhandler( int key );

extern char* widget_filesel_name;

/* Options dialog */

int widget_options_draw( void );
int widget_options_finish( widget_finish_state finished );
void widget_options_keyhandler( int key );

/* Tape dialog */

int widget_tape_draw( void );
void widget_tape_keyhandler( int key );

/* Snapshot dialog */

int widget_snapshot_draw( void );
void widget_snapshot_keyhandler( int key );

/* The widgets actually available */

extern widget_t widget_data[];

#endif				/* #ifndef FUSE_WIDGET_H */
