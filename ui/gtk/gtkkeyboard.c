/* gtkkeyboard.c: GTK+ routines for dealing with the keyboard
   Copyright (c) 2000-2003 Philip Kendall, Russell Marks

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
   Foundation, Inc., 49 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#ifdef UI_GTK		/* Use this file iff we're using GTK+ */

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "compat.h"
#include "gtkinternals.h"
#include "input.h"
#include "keyboard.h"
#include "ui/ui.h"

static guint gtkkeyboard_unshift_keysym(guint keysym);

static void
get_keysyms( input_event_t *event, guint keysym )
{
  guint unshifted;

  /* The GTK+ UI doesn't actually use the native keysym for anything,
     but we may as well set it up anyway as we've got it */
  event->types.key.native_key = keysyms_remap( keysym );

  unshifted = gtkkeyboard_unshift_keysym( keysym );
  event->types.key.spectrum_key = keysyms_remap( unshifted );
}

int
gtkkeyboard_keypress( GtkWidget *widget GCC_UNUSED, GdkEvent *event,
		      gpointer data GCC_UNUSED )
{
  input_event_t fuse_event;

#ifdef UI_GTK2
  if( event->key.keyval == GDK_F10 && event->key.state == 0 )
    ui_mouse_suspend();
#endif

  fuse_event.type = INPUT_EVENT_KEYPRESS;
  get_keysyms( &fuse_event, event->key.keyval );

  return input_event( &fuse_event );

  /* FIXME: handle F1 to deal with the pop-up menu */
}

int
gtkkeyboard_keyrelease( GtkWidget *widget GCC_UNUSED, GdkEvent *event,
			gpointer data GCC_UNUSED )
{
  input_event_t fuse_event;

  fuse_event.type = INPUT_EVENT_KEYRELEASE;
  get_keysyms( &fuse_event, event->key.keyval );

  return input_event( &fuse_event );
}

/* Given a keysym, return the keysym which would have been returned if
   the key where unshifted */
static guint gtkkeyboard_unshift_keysym(guint keysym)
{
  /* Oh boy is this ugly! There are better ways of doing this (see
     http://mail.gnome.org/archives/gtk-app-devel-list/2000-December/msg00261.html
     and followups). However, this will do until that functionality is
     incorporated into GTK 2.0 */
  return XKeycodeToKeysym(gdk_display,
			  XKeysymToKeycode(gdk_display,keysym),
			  0);
}

#endif			/* #ifdef UI_GTK */
