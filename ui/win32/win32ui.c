/* win32ui.c: Win32 routines for dealing with the user interface
   Copyright (c) 2003 Marek Januszewski, Philip Kendall

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

#ifdef UI_WIN32			/* Use this iff we're using UI_WIN32 */

#include "debugger/debugger.h"
#include "display.h"
#include "fuse.h"
#include "menu_data.h"
#include "win32internals.h"

HACCEL hAccels;

int paused = 0;

void blit( void );

LRESULT WINAPI MainWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch( msg )
  {
    case WM_CREATE:
    {        
      win32statusbar_create( hWnd ); 
      break;
    }    
  
    case WM_COMMAND:
    {
      handle_menu( LOWORD( wParam ), hWnd );
      break;
    }         
      
    case WM_CLOSE:
    {
      if( MessageBox( fuse_hWnd, "Fuse - confirm", "Exit Fuse?",
      MB_YESNO|MB_ICONQUESTION ) == IDYES	)
      {
        DestroyWindow(hWnd);
      }
      break;
    }

    case WM_KEYDOWN:
    {
      win32keyboard_keypress( wParam, lParam );
      break ;
    }

    case WM_KEYUP:
    {
      win32keyboard_keyrelease( wParam, lParam );
      break ;
    }
      
    case WM_PAINT:
    {
      if ( display_ui_initialised )
      {
        PAINTSTRUCT ps;
        IDirectDrawSurface_SetClipper( lpdds, lpddc );
        BeginPaint( hWnd, &ps );
        blit();
        EndPaint( hWnd, &ps );
        IDirectDrawSurface_SetClipper( lpdds, NULL );
        break ;
      }
    }
    
    case WM_SIZE:
    {
      SendMessage( fuse_hStatusWindow, WM_SIZE, wParam, lParam );
      break;
    }    

    case WM_DESTROY:
    {
      fuse_exiting = 1;
      PostQuitMessage( 0 );
      break;
    }

    default:
    {
      return( DefWindowProc( hWnd, msg, wParam, lParam ));
    }
  }
  
  return 0;
}

/* this is where windows program begins */
int WINAPI WinMain (HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine,
                    int nCmdShow)
{
  WNDCLASS wc;
  
  if( !hPrevInstance )
  {
    wc.lpszClassName = "Fuse";
    wc.lpfnWndProc = MainWndProc;
    wc.style = CS_OWNDC;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
    wc.hCursor = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)( COLOR_WINDOW+1 );
    wc.lpszMenuName = "win32_menu"; 
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    
    if ( !RegisterClass( &wc ) )
      return 0;
  }
  
  fuse_hWnd = CreateWindow( "Fuse", "Fuse",
    WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX, 
/*    WS_OVERLAPPEDWINDOW, */
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    NULL, NULL, hInstance, NULL );
    
  //init windows controls like status bar
  InitCommonControls();

  ShowWindow( fuse_hWnd, nCmdShow );
  UpdateWindow( fuse_hWnd );
  
  fuse_hInstance = hInstance;
  
  hAccels = LoadAccelerators( fuse_hInstance, "win32_accel" );
    
  return fuse_main(__argc, __argv);
  /* finish - how do deal with returning wParam */
}

int
ui_init( int *argc, char ***argv )
{
/* TODO: what's this
  if( settings_current.aspect_hint ) {
    hints |= GDK_HINT_ASPECT;
    geometry.min_aspect = geometry.max_aspect =
      ((float)DISPLAY_ASPECT_WIDTH)/DISPLAY_SCREEN_HEIGHT;
  }
*/
  if( win32display_init() ) return 1;

  return 0;
}
	
int
ui_event( void )
{
  MSG msg;    
  /* Process messages */
  while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
  {
/* TODO: double check this && */
    if( !IsDialogMessage( fuse_hPFWnd, &msg) 
     && !IsDialogMessage( fuse_hDBGWnd, &msg) )
    {
      if( !TranslateAccelerator( fuse_hWnd, hAccels, &msg ) )
      {
        if( msg.message==WM_QUIT ) break;
        /* finish - set exit flag somewhere */
        TranslateMessage( &msg );
        DispatchMessage( &msg );
       }    
    }    
  }

  return 0;
  /* finish - somwhere there should be return msg.wParam */
}
		
int
ui_end( void )
{
    int error;

    if ( lpdds ) IDirectDrawSurface_Release( lpdds );
    if ( lpdds2 ) IDirectDrawSurface_Release( lpdds2 );
    if ( lpddc ) IDirectDrawSurface_Release( lpddc );
    if ( lpdd ) IDirectDraw_Release( lpdd );

    error = win32display_end(); if ( error ) return error;
    return 0;
}

void win32_verror( int is_error )
{
  if ( !is_error ) return;

  DWORD last_error;
  static LPVOID err_msg;
  last_error = GetLastError();
  FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                 FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL, last_error, LANG_USER_DEFAULT,
                 (LPTSTR) &err_msg, 0, NULL );
  MessageBox( fuse_hWnd, err_msg, "Error", MB_OK );
}

void
menu_machine_debugger()
{
  debugger_mode = DEBUGGER_MODE_HALTED;
  if( paused ) ui_debugger_activate();
}

void
blit( void )
{
  if ( display_ui_initialised )
  {
    RECT srcrec, destrec;
    POINT point;
  
    point.x = 0;
    point.y = 0;
    srcrec.left = 0;
    srcrec.top = 0;
    ClientToScreen( fuse_hWnd, &point );
    GetClientRect( fuse_hWnd, &srcrec );
  
    destrec.top = point.y;
    destrec.left = point.x;
    destrec.bottom = destrec.top + srcrec.bottom;
    destrec.right = destrec.left + srcrec.right;
  
    IDirectDrawSurface_Blt( lpdds, &destrec, 
      lpdds2, &srcrec, DDBLT_WAIT, NULL );
  }
  return;
}

int
ui_mouse_grab( int startup )
{
  return 0; /* FIXME */
}

int
ui_mouse_release( int suspend )
{
  return 0; /* FIXME */
}

#endif			/* #ifdef UI_WIN32 */
