/* debugger.c: the Win32 debugger
   Copyright (c) 2004 Philip Kendall, Marek Januszewski

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

#ifdef UI_WIN32		/* Use this file iff we're using WIN32 */

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "win32internals.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

#include "debugger.h"

int debugger_active;

int create_dialog();
void win32_debugger_close();
int ui_debugger_activate( void );
int activate_debugger( void );
int ui_debugger_disassemble( libspectrum_word address );
int ui_debugger_update( void );
int ui_debugger_deactivate( int interruptable );
int deactivate_debugger( void );
void win32_debugger_done_close();
void win32_debugger_done_continue();
void win32_debugger_done_step();
void win32_debugger_break();

BOOL CALLBACK DebuggerProc( HWND hWnd, UINT msg,
			    WPARAM wParam, LPARAM lParam )
{
  switch( msg )
  {
    case WM_COMMAND:
      switch( LOWORD( wParam ) )
      {
	case IDC_BTN_CLOSE:
	  win32_debugger_done_close();
	  return TRUE;
	case IDC_BTN_CONT:
	  win32_debugger_done_continue();
	  return TRUE;
	case IDC_BTN_STEP:
	  win32_debugger_done_continue();
	  return TRUE;
      }
      return FALSE;
    case WM_CLOSE:
      win32_debugger_close();
      return TRUE;
  }
  return FALSE;
}

int
create_dialog()
{
  if (fuse_hDBGWnd == NULL)
  {
    fuse_hDBGWnd = CreateDialog( fuse_hInstance, "IDG_DBG", fuse_hWnd,
      (DLGPROC) DebuggerProc );
    win32_verror( fuse_hDBGWnd == NULL );

    /* get handle to the listview and set kind and the column names */
/* TODO: delete - just an example
    HWND hLocListView;
    hLocListView = GetDlgItem( fuse_hPFWnd, IDC_PF_LIST );

    LVCOLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = 60;
    lvc.pszText = "Page";
    ListView_InsertColumn( hLocListView, 0, &lvc );
    lvc.cx = 92;
    lvc.pszText = "Offset";
    ListView_InsertColumn( hLocListView, 1, &lvc );

    update_pokefinder();
*/
  }
  else
  {
    SetActiveWindow( fuse_hDBGWnd );
  }
  return 0;
}

void
win32_debugger_close()
{
  DestroyWindow( fuse_hDBGWnd );
  fuse_hDBGWnd = NULL;
}

int
ui_debugger_activate( void )
{
  int error;

/* TODO: pause - probably stops the message loop
  fuse_emulation_pause();
*/

  /* Create the dialog box if it doesn't already exist */
  /* win32 create_dialog checks if dialog is created */
  if( create_dialog() ) return 1;

/* TODO: (low priority)
  error = hide_hidden_panes(); if( error ) return error;
*/

  HWND hContButton;
  hContButton = GetDlgItem( fuse_hDBGWnd, IDC_BTN_CONT );
  EnableWindow(hContButton, TRUE);

  HWND hBreakButton;
  hBreakButton = GetDlgItem( fuse_hDBGWnd, IDC_BTN_BREAK );
  EnableWindow( hBreakButton, FALSE );

  if( !debugger_active ) activate_debugger();

  return 0;
}

int
activate_debugger( void )
{
  debugger_active = 1;

  ui_debugger_disassemble( PC );
  ui_debugger_update();

  return 0;
}

int
ui_debugger_disassemble( libspectrum_word address )
{
/* TODO: finish this */
  return 0;
}

int
ui_debugger_update( void )
{
/* TODO: finish this */
  return 0;
}

int
ui_debugger_deactivate( int interruptable )
{
  if( debugger_active ) deactivate_debugger();

  HWND hContButton;
  hContButton = GetDlgItem( fuse_hDBGWnd, IDC_BTN_CONT );
  EnableWindow(hContButton, !interruptable ? TRUE : FALSE );

  HWND hBreakButton;
  hBreakButton = GetDlgItem( fuse_hDBGWnd, IDC_BTN_BREAK );
  EnableWindow( hBreakButton, interruptable ? TRUE : FALSE );

  return 0;
}

int
deactivate_debugger( void )
{
  SendMessage( fuse_hDBGWnd, WM_CLOSE, (WPARAM) 0, (LPARAM) 0 );
  debugger_active = 0;
/* TODO: unpause - probably stops the message loop
  fuse_emulation_unpause();
*/
  return 0;
}

void
win32_debugger_done_close()
{
  SendMessage( fuse_hDBGWnd, WM_CLOSE, (WPARAM) 0, (LPARAM) 0 );
  win32_debugger_done_continue();
}

void
win32_debugger_done_continue()
{
  debugger_run();
}

void
win32_debugger_done_step()
{
  debugger_step();
}

void
win32_debugger_break()
{
  debugger_mode = DEBUGGER_MODE_HALTED;

  HWND hContButton;
  hContButton = GetDlgItem( fuse_hDBGWnd, IDC_BTN_CONT );
  EnableWindow(hContButton, TRUE);

  HWND hBreakButton;
  hBreakButton = GetDlgItem( fuse_hDBGWnd, IDC_BTN_BREAK );
  EnableWindow( hBreakButton, FALSE );
}

#endif			/* #ifdef UI_WIN32 */
