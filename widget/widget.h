/* widget.h: Simple dialog boxes for all user interfaces.
   Copyright (c) 2001,2002 Matan Ziv-Av, Philip Kendall

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

#ifndef FUSE_KEYBOARD_H
#include "keyboard.h"
#endif				/* #ifndef FUSE_KEYBOARD_H */

#ifndef FUSE_UI_H
#include "ui/ui.h"
#endif

/* How many levels deep have we recursed through widgets; -1 => none */
extern int widget_level;

/* Code called at start and end of emulation */
int widget_init( void );
int widget_end( void );

/* The various widgets which are available */
typedef enum widget_type {

  WIDGET_TYPE_FILESELECTOR,	/* File selector */
  WIDGET_TYPE_GENERAL,		/* General options */
  WIDGET_TYPE_PICTURE,		/* Keyboard picture */
  WIDGET_TYPE_MENU,		/* General menu */
  WIDGET_TYPE_SELECT,		/* Select machine */
  WIDGET_TYPE_SOUND,		/* Sound options */
  WIDGET_TYPE_ERROR,		/* Error report */
  WIDGET_TYPE_RZX,		/* RZX options */
  WIDGET_TYPE_BROWSE,		/* Browse tape */
  WIDGET_TYPE_TEXT,		/* Text entry widget */
  WIDGET_TYPE_SCALER,		/* Select scaler */
  WIDGET_TYPE_DEBUGGER,		/* Debugger widget */
  WIDGET_TYPE_ROM,		/* ROM selector widget */

} widget_type;

/* Activate a widget */
int widget_do( widget_type which, void *data );

/* A function to handle keypresses */
typedef void (*widget_keyhandler_fn)( keyboard_key_name key,
				      keyboard_key_name key2 );

/* The current widget keyhandler */
extern widget_keyhandler_fn widget_keyhandler;

/* Widget-specific bits */

/* Menu widget */

/* A generic callback function */
typedef int (*widget_menu_callback_fn)( void *data );

/* A general menu */
typedef struct widget_menu_entry {
  const char *text;		/* Menu entry text */
  keyboard_key_name key;	/* Which key to activate this widget */

  widget_menu_callback_fn action; /* What to do */
  void *data;			/* And with which arguments */

} widget_menu_entry;

/* The main menu as activated with F1 */
extern widget_menu_entry widget_menu_main[];

/* The name returned from the file selector */

extern char* widget_filesel_name;

/* Get a filename and do something with it */
int widget_apply_to_file( void *data );
     
/* The error widget data type */

typedef struct widget_error_t {
  ui_error_level severity;
  const char *message;
} widget_error_t;

#endif				/* #ifndef FUSE_WIDGET_H */
