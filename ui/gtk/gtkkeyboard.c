/* gtkkeyboard.c: GTK+ routines for dealing with the keyboard
   Copyright (c) 2000-2002 Philip Kendall, Russell Marks

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

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#ifdef UI_GTK		/* Use this file iff we're using GTK+ */

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "fuse.h"
#include "gtkkeyboard.h"
#include "gtkui.h"
#include "keyboard.h"
#include "widget/widget.h"

static guint gtkkeyboard_unshift_keysym(guint keysym);

int
gtkkeyboard_keypress( GtkWidget *widget GCC_UNUSED, GdkEvent *event,
		      gpointer data GCC_UNUSED )
{
  guint keysym; const keysyms_key_info *ptr;

  keysym=gtkkeyboard_unshift_keysym(event->key.keyval);

  ptr=keysyms_get_data(keysym);

  if(ptr) {

    if( widget_level >= 0 ) {
      widget_keyhandler( ptr->key1, ptr->key2 );
    } else {
      if(ptr->key1 != KEYBOARD_NONE) keyboard_press(ptr->key1);
      if(ptr->key2 != KEYBOARD_NONE) keyboard_press(ptr->key2);
    }
    return TRUE;
    
  }

  /* Now deal with the non-Speccy keys. Most are dealt with by
     menu shortcuts in gtkui.c, but F1 can't be done that way. */
  
  if( keysym == GDK_F1 ) {
    gtkui_popup_menu();
    return TRUE;
  }

  return FALSE;
}

int
gtkkeyboard_keyrelease( GtkWidget *widget GCC_UNUSED, GdkEvent *event,
		        gpointer data GCC_UNUSED )
{
  guint keysym; const keysyms_key_info *ptr;

  keysym=gtkkeyboard_unshift_keysym(event->key.keyval);

  ptr=keysyms_get_data(keysym);

  if(ptr) {
    if(ptr->key1 != KEYBOARD_NONE) keyboard_release(ptr->key1);
    if(ptr->key2 != KEYBOARD_NONE) keyboard_release(ptr->key2);
    return TRUE;
  } else {
    return FALSE;
  }

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

/* Release all keys (called when focus lost) */
int
gtkkeyboard_release_all( GtkWidget *widget GCC_UNUSED,
			 GdkEvent *event GCC_UNUSED, gpointer data GCC_UNUSED )
{
  keyboard_release_all();
  return TRUE;
}
			  
#endif			/* #ifdef UI_GTK */
