/* error.c: handle errors
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

#include <config.h>

#ifdef UI_WIN32			/* Use this iff we're using UI_WIN32 */

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

#include <windows.h>

#include "fuse.h"
#include "ui/ui.h"

ui_error_specific( ui_error_level severity, const char *message )
{
  /* finish - can ui be not initialized? */
  UINT mtype;
  HWND hWnd;

  fuse_emulation_pause();

  hWnd = GetActiveWindow();

  switch( severity ) {

  case UI_ERROR_INFO:
    MessageBox(hWnd, message, "Fuse - Info", MB_ICONINFORMATION|MB_OK ); break;
  case UI_ERROR_WARNING:
    MessageBox(hWnd, message, "Fuse - Warning", MB_ICONWARNING|MB_OK ); break;
  case UI_ERROR_ERROR:
    MessageBox(hWnd, message, "Fuse - Error", MB_ICONERROR|MB_OK ); break;
  default:
    MessageBox(hWnd, message, "Fuse - (Unknown error)", MB_ICONINFORMATION|MB_OK );
    break;

  }

  fuse_emulation_unpause();

  return 0;
}

#endif			/* #ifdef UI_WIN32 */
