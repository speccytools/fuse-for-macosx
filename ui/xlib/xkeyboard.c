/* xkeyboard.c: X routines for dealing with the keyboard
   Copyright (c) 2000-2003 Philip Kendall

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

#ifdef UI_X			/* Use this iff we're using Xlib */

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "display.h"
#include "fuse.h"
#include "keyboard.h"
#include "machine.h"
#include "settings.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"
#ifdef USE_WIDGET
#include "widget/widget.h"
#endif				/* #ifdef USE_WIDGET */
#include "xkeyboard.h"

void xkeyboard_keypress(XKeyEvent *event)
{
  KeySym keysym;
  input_key fuse_keysym;
  input_event_t fuse_event;

  keysym=XLookupKeysym(event,0);

  fuse_keysym = keysyms_remap( keysym );

  if( fuse_keysym == INPUT_KEY_NONE ) return;

  fuse_event.type = INPUT_EVENT_KEYPRESS;
  fuse_event.types.key.key = fuse_keysym;

  input_event( &fuse_event );
}

void xkeyboard_keyrelease(XKeyEvent *event)
{
  KeySym keysym;
  input_key fuse_keysym;
  input_event_t fuse_event;

  keysym=XLookupKeysym(event,0);

  fuse_keysym = keysyms_remap( keysym );

  if( fuse_keysym == INPUT_KEY_NONE ) return;

  fuse_event.type = INPUT_EVENT_KEYRELEASE;
  fuse_event.types.key.key = fuse_keysym;

  input_event( &fuse_event );
}

#endif				/* #ifdef UI_X */
