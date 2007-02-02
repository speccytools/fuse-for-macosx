/* statusbar.c: routines for updating the status bar
   Copyright (c) 2004 Marek Januszewski

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#ifdef UI_WIN32		/* Use this file iff we're using Win32 */

#include "ui/ui.h"
#include "win32internals.h"

HICON
  icon_tape_inactive, icon_tape_active,
  icon_disk_inactive, icon_disk_active,
  icon_pause_inactive, icon_pause_active;

int
win32statusbar_create( HWND hWnd )
{
/*
  icon_tape_inactive = LoadIcon( fuse_hInstance, "win32bmp_tape_active");
  icon_tape_active = LoadIcon( fuse_hInstance, "win32bmp_tape_active");
  icon_disk_inactive = LoadIcon( fuse_hInstance, "win32bmp_disk_inactive");
  icon_disk_active = LoadIcon( fuse_hInstance, "win32bmp_disk_active");
  icon_pause_inactive = LoadIcon( fuse_hInstance, "win32bmp_pause_inactive");
  icon_pause_active = LoadIcon( fuse_hInstance, "win32bmp_pause_active");
*/    
  fuse_hStatusWindow = CreateStatusWindow( 
    WS_CHILD | WS_VISIBLE, "0, 0", hWnd, ID_STATUSBAR );

  /* divide status bar */
  int parts[2] = { 100, 100 };
  SendMessage( fuse_hStatusWindow, SB_SETPARTS, (WPARAM) 2, (LPARAM) parts );
  SendMessage( fuse_hStatusWindow, SB_SETTEXT, (WPARAM) 0,
    (LPARAM) "some text");
  SendMessage( fuse_hStatusWindow, SB_SETTEXT, (WPARAM) 1,
    (LPARAM) "some text");
/*
  SendMessage( fuse_hStatusWindow, SB_SETTEXT, (WPARAM) 2,
    (LPARAM) "some text");
*/
/*
http://groups.google.com/groups?hl=en&lr=&ie=UTF-8&safe=off&th=76b04b36db7eaa16&rnum=1
*/
}

/*
 TODO: implement set_visibility from settings - similar to gtk

int
win32statusbar_set_visibility( int visible )
{
}

int
ui_statusbar_update( ui_statusbar_item item, ui_statusbar_state state )
{
}
*/

int
ui_statusbar_update_speed( float speed )
{
  char buffer[8];

  snprintf( buffer, 8, "%3.0f%%", speed );
  SendMessage( fuse_hStatusWindow, SB_SETTEXT, (WPARAM) 2,
    (LPARAM) buffer);

  return 0;
}

#endif			/* #ifdef UI_WIN32 */
