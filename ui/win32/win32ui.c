/* win32ui.c: Win32 routines for dealing with the user interface
   Copyright (c) 2003 Marek Januszewski

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

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#ifdef UI_WIN32			/* Use this iff we're using UI_WIN32 */

#include "display.h"
#include "fuse.h"
#include "win32display.h"

LRESULT WINAPI MainWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch( msg )
  {
    case WM_CLOSE:
    {
      if( MessageBox( hWnd, "Fuse - confirm", "Exit Fuse?",
      MB_YESNO|MB_ICONQUESTION ) == IDYES	)
      {
        DestroyWindow(hWnd);
      }
      break;
    }

    case WM_DESTROY:
    {
      fuse_exiting = 1;
      PostQuitMessage( 0 );
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
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    
    if ( !RegisterClass( &wc ) )
      return 0;
  }
  
  hWnd = CreateWindow( "Fuse", "Fuse",
    WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
    0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
    NULL, NULL, hInstance, NULL );

  ShowWindow( hWnd, nCmdShow );
  UpdateWindow( hWnd );
    
  return main1(__argc, __argv);
  /* finish - how do deal with returning wParam */
}

int
ui_init( int *argc, char ***argv )
{
  return 0;

}
	
int
ui_event( void )
{
  /* Process messages */
  while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
  {
    if( msg.message==WM_QUIT ) break;
    /* finish - set exit flag somewhere */
    TranslateMessage( &msg );
    DispatchMessage( & msg );
  }

  return 0;
  /* finish - somwhere there should be return msg.wParam */
}
		
int
ui_end( void )
{
    int error;
    error = uidisplay_end();
    if ( error )
      return error;
    return 0;
}

#endif			/* #ifdef UI_WIN32 */
