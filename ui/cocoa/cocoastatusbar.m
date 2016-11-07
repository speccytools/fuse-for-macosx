/* cocoastatusbar.m: Routines for dealing with Cocoa status feedback
   Copyright (c) 2007 Fredrick Meunier

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

#import "DisplayOpenGLView.h"

#include "cocoadisplay.h"
#include "settings.h"
#include "ui/ui.h"

/* The statusbar handling function */
int
ui_statusbar_update( ui_statusbar_item item, ui_statusbar_state state )
{
  switch( item ) {

  case UI_STATUSBAR_ITEM_DISK:
    [[DisplayOpenGLView instance]
          performSelectorOnMainThread:@selector(setDiskState:)
          withObject:[NSNumber numberWithUnsignedChar:state]
          waitUntilDone:NO
    ];
    return 0;

  case UI_STATUSBAR_ITEM_PAUSED:
    /* We don't support pausing this version of Fuse - but now we can! */
    return 0;

  case UI_STATUSBAR_ITEM_TAPE:
    [[DisplayOpenGLView instance]
          performSelectorOnMainThread:@selector(setTapeState:)
          withObject:[NSNumber numberWithUnsignedChar:state]
          waitUntilDone:NO
    ];
    return 0;

  case UI_STATUSBAR_ITEM_MICRODRIVE:
    [[DisplayOpenGLView instance]
          performSelectorOnMainThread:@selector(setMdrState:)
          withObject:[NSNumber numberWithUnsignedChar:state]
          waitUntilDone:NO
    ];
    return 0;

  case UI_STATUSBAR_ITEM_MOUSE:
    /* We don't support showing a grab icon */
    return 0;

  }

  ui_error( UI_ERROR_ERROR, "Attempt to update unknown statusbar item %d",
            item );
  return 1;
}
