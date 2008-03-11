/* statusbar.c: routines for updating the status bar
   Copyright (c) 2004-2008 Marek Januszewski

   $Id$

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

#include <tchar.h>

#include "ui/ui.h"
#include "win32internals.h"

HBITMAP
  icon_tape_inactive, icon_tape_active,
  icon_mdr_inactive, icon_mdr_active,
  icon_disk_inactive, icon_disk_active,
  icon_pause_inactive, icon_pause_active,
  icon_mouse_inactive, icon_mouse_active;

ui_statusbar_state icons_status[5] = {
  UI_STATUSBAR_STATE_NOT_AVAILABLE, /* disk */
  UI_STATUSBAR_STATE_NOT_AVAILABLE, /* microdrive */
  UI_STATUSBAR_STATE_NOT_AVAILABLE, /* mouse */
  UI_STATUSBAR_STATE_NOT_AVAILABLE, /* paused */
  UI_STATUSBAR_STATE_NOT_AVAILABLE  /* tape */
};

int icons_part_width = 140; /* will be calculated dynamically later */
int icons_part_height = 27;
int icons_part_margin = 2;

void
win32statusbar_create( HWND hWnd )
{
  /* FIXME: destroy those icons later on using DeleteObject */
  
  icon_tape_inactive = LoadImage( fuse_hInstance, "win32bmp_tape_active", 
                                  IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
  icon_tape_active = LoadImage( fuse_hInstance, "win32bmp_tape_active", 
                                IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

  icon_mdr_inactive = LoadImage( fuse_hInstance, "win32bmp_mdr_inactive", 
                                  IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
  icon_mdr_active = LoadImage( fuse_hInstance, "win32bmp_mdr_active", 
                                IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

  icon_disk_inactive = LoadImage( fuse_hInstance, "win32bmp_disk_inactive", 
                                  IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
  icon_disk_active = LoadImage( fuse_hInstance, "win32bmp_disk_active", 
                                IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

  icon_pause_inactive = LoadImage( fuse_hInstance, "win32bmp_pause_inactive", 
                                   IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
  icon_pause_active = LoadImage( fuse_hInstance, "win32bmp_pause_active", 
                                 IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

  icon_mouse_inactive = LoadImage( fuse_hInstance, "win32bmp_mouse_inactive", 
                                   IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
  icon_mouse_active = LoadImage( fuse_hInstance, "win32bmp_mouse_active", 
                                 IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
  
  /* FIXME: CreateStatusWindow was superceded by CreateWindow */
  fuse_hStatusWindow = CreateStatusWindow(
    WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, TEXT( "" ), hWnd, ID_STATUSBAR );
    
  /* minimum size of the status bar is the height of the highest icon */
  SendMessage( fuse_hStatusWindow, SB_SETMINHEIGHT, icons_part_height, 0 );

  /* force redraw to apply the minimum height */
  SendMessage( fuse_hStatusWindow, WM_SIZE, 0, 0 );
}

int
win32statusbar_set_visibility( int visible )
{
/* FIXME: fix this */
  if( visible ) {
    ShowWindow( fuse_hStatusWindow, SW_SHOW );
  } else {
    ShowWindow( fuse_hStatusWindow, SW_HIDE );
  }

  return 0;
}

int
ui_statusbar_update( ui_statusbar_item item, ui_statusbar_state state )
{
  icons_status[ item ] = state;

  SendMessage( fuse_hStatusWindow, SB_SETTEXT, 1 | SBT_OWNERDRAW, 0 );

  return 0;
}

int
ui_statusbar_update_speed( float speed )
{
  TCHAR buffer[8];

  _sntprintf( buffer, 8, "\t%3.0f%%", speed ); /* \t centers the text */
  SendMessage( fuse_hStatusWindow, SB_SETTEXT, (WPARAM) 2,
               (LPARAM) buffer);

  return 0;
}

void
win32statusbar_redraw( HWND hWnd, LPARAM lParam )
{
  DRAWITEMSTRUCT* di;
  HDC src_dc, dest_dc;
  RECT rc_item;
  HBITMAP src_bmp;
  BITMAP bmp;
  size_t i;
  
  src_bmp = 0;

  di = ( DRAWITEMSTRUCT* ) lParam;
  dest_dc = di->hDC;
  rc_item = di->rcItem;

  icons_part_width = 0;

  for( i=0; i<5; i++ ) {
    
    switch( i ) {
      case UI_STATUSBAR_ITEM_DISK:
        switch( icons_status[i] ) {
          case UI_STATUSBAR_STATE_NOT_AVAILABLE:
            src_bmp = NULL; break;
          case UI_STATUSBAR_STATE_ACTIVE:
            src_bmp = icon_disk_active; break;
          default:
            src_bmp = icon_disk_inactive; break;
        }
        break;

      case UI_STATUSBAR_ITEM_MOUSE:
        src_bmp = ( icons_status[i] == UI_STATUSBAR_STATE_ACTIVE ?
                    icon_mouse_active : icon_mouse_inactive );
        break;

      case UI_STATUSBAR_ITEM_PAUSED:
        src_bmp = ( icons_status[i] == UI_STATUSBAR_STATE_ACTIVE ?
                    icon_pause_active : icon_pause_inactive );
        break;

      case UI_STATUSBAR_ITEM_MICRODRIVE:
        switch( icons_status[i] ) {
          case UI_STATUSBAR_STATE_NOT_AVAILABLE:
            src_bmp = NULL; break;
          case UI_STATUSBAR_STATE_ACTIVE:
            src_bmp = icon_mdr_active; break;
          default:
            src_bmp = icon_mdr_inactive; break;
        }
        break;

      case UI_STATUSBAR_ITEM_TAPE:
        src_bmp = ( icons_status[i] == UI_STATUSBAR_STATE_ACTIVE ?
                    icon_tape_active : icon_tape_inactive );
        break;
    }

    if( src_bmp != NULL ) {    
      icons_part_width += icons_part_margin;

      GetObject( src_bmp, sizeof( bmp ), &bmp );
      src_dc = CreateCompatibleDC( NULL );
      SelectObject( src_dc, src_bmp );
      BitBlt( dest_dc, rc_item.left + icons_part_width,
        rc_item.top + ( icons_part_height - bmp.bmHeight )
        - ( 2 * icons_part_margin ), 
        bmp.bmWidth, bmp.bmHeight, src_dc, 0, 0, SRCCOPY );
      /* FIXME: make background color transparent */
      DeleteDC( src_dc );
      icons_part_width += bmp.bmWidth;
    }
  }
  ReleaseDC( hWnd, dest_dc );
  
  if( icons_part_width > 0 ) icons_part_width += icons_part_margin;
  /* FIXME: if the calculations are correction I shouldn't be adding this */
  icons_part_width += ( 2 * icons_part_margin );
  /* force resize */
  SendMessage( fuse_hStatusWindow, WM_SIZE, 0, 0 );  
}

void
win32statusbar_resize( HWND hWnd )
{
  const int speed_bar_width = 70;
  
  /* divide status bar */
  RECT rect;
  GetClientRect( hWnd, &rect );
  
  int parts[3];
  parts[0] = rect.right - rect.left - icons_part_width - speed_bar_width;
  parts[1] = parts[0] + icons_part_width;
  parts[2] = parts[1] + speed_bar_width;
  SendMessage( fuse_hStatusWindow, SB_SETPARTS, 3, ( LPARAM ) &parts );
}
