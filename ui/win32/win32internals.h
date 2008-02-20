/* win32internals.h: stuff internal to the Win32 UI
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

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#ifndef FUSE_WIN32INTERNALS_H
#define FUSE_WIN32INTERNALS_H

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>

#define ID_STATUSBAR 900

/* window handler */
HWND fuse_hWnd;

/* application instance */
HINSTANCE fuse_hInstance;

/* status bar handle */
HWND fuse_hStatusWindow;

/* pokefinder window handle */
HWND fuse_hPFWnd;

/* debugger window handle */
HWND fuse_hDBGWnd;

void win32statusbar_create();

int win32statusbar_set_visibility( int visible );

void win32_verror( int is_error );

void handle_menu( DWORD cmd, HWND okno );

int win32ui_picture( const char *filename, int border );

/*
 * Dialog box reset
 */

void win32ui_pokefinder_clear( void );

#endif                          /* #ifndef FUSE_WIN32INTERNALS_H */
