/* win32display.c: Routines for dealing with the Win32 DirectDraw display
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

#include "config.h"

#ifdef UI_WIN32			/* Use this iff we're using UI_WIN32 */

#include "display.h"
#include "machine.h"
#include "ui/ui.h"
#include "win32keyboard.h"
#include "win32display.h"

#define DD_ERROR( msg )   if ( ddres != DD_OK ) {\
    uidisplay_end();\
    ui_error( UI_ERROR_ERROR, msg, NULL );\
    return 1;\
  }
  
unsigned int colour_pal[] = {
    0,0,0,
    0,0,192,
    192,0,0,
    192,0,192,
    0,192,0,
    0,192,192,
    192,192,0,
    192,192,192,
    0,0,0,
    0,0,255,
    255,0,0,
    255,0,255,
    0,255,0,
    0,255,255,
    255,255,0,
    255,255,255
};

int
uidisplay_init( int width, int height )
{
  HRESULT ddres;
  RECT rect, rect2;
  
  /* resize */
  GetClientRect( hWnd, &rect );
  rect.right = rect.left + width;
  rect.bottom = rect.top + height;
  GetWindowRect( hWnd, &rect2 );
  AdjustWindowRect( &rect,
    WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
    FALSE );
  MoveWindow( hWnd, rect2.left, rect2.top, 
    rect.right - rect.left, rect.bottom - rect.top, TRUE );
    
  /* dd init */  
  ddres = DirectDrawCreate(NULL, &lpdd, NULL);
  DD_ERROR( "error initializing" )

  /* set cooperative level to normal */  
  ddres = IDirectDraw_SetCooperativeLevel( lpdd, hWnd, DDSCL_NORMAL );
  DD_ERROR( "error setting cooperative level" )

  /* fill surface description */
  memset( &ddsd, 0, sizeof( ddsd ) );
  ddsd.dwSize = sizeof( ddsd );
  ddsd.dwFlags = DDSD_CAPS;
  ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
  
  /* attempt to create primary surface */
  ddres = IDirectDraw_CreateSurface( lpdd, &ddsd, &lpdds, NULL );
  DD_ERROR( "error creating primary surface" )

  /* fill off screen surface description based on window size*/
  GetClientRect( hWnd, &rect);
  memset( &ddsd, 0, sizeof( ddsd ) );
  ddsd.dwSize = sizeof( ddsd );
  ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
  ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
  ddsd.dwWidth = rect.right - rect.left;
  ddsd.dwHeight = rect.bottom - rect.top;
  
  /* attempt to create off screen surface */
  ddres = IDirectDraw_CreateSurface( lpdd, &ddsd, &lpdds2, NULL );
  DD_ERROR( "error creating off-screen surface" )
  
  /* create the clipper */
  ddres = IDirectDraw_CreateClipper( lpdd, 0, &lpddc, NULL );
  DD_ERROR( "error creating clipper" )
  
  /* set the window */
  ddres = IDirectDrawClipper_SetHWnd( lpddc, 0, hWnd );
  DD_ERROR( "error setting window for the clipper" )

  /* set clipper for the primary surface */
/*
  moved to WM_PAINT
  ddres = IDirectDrawSurface_SetClipper( lpdds, lpddc );
  DD_ERROR( "error setting clipper for primary surface" )
*/

  display_ui_initialised = 1;
  return 0;
}

int
uidisplay_end( void )
{
  display_ui_initialised = 0;

  if ( lpdds ) IDirectDrawSurface_Release( lpdds );
  if ( lpdds2 ) IDirectDrawSurface_Release( lpdds2 );
  if ( lpddc ) IDirectDrawSurface_Release( lpddc );
  if ( lpdd ) IDirectDraw_Release( lpdd );

  return 0;
}

void
uidisplay_hotswap_gfx_mode( void )
{
    return;
}

void
uidisplay_area( int x, int y, int width, int height )
{
  int disp_x,disp_y;
  DWORD pix_colour;

  HDC dc;

  /* get DC of off-screen buffer */
  IDirectDrawSurface_GetDC( lpdds2, &dc );

  for(disp_x=x;disp_x < x+width;disp_x++)
  {
    for(disp_y=y;disp_y < y+height;disp_y++)
    {
      pix_colour = 
        colour_pal[3*display_image[disp_y][disp_x]] + 
        colour_pal[(1+(3*display_image[disp_y][disp_x]))] * 256 +
        colour_pal[(2+(3*display_image[disp_y][disp_x]))] * 256 * 256;
      SetPixel( dc, disp_x, disp_y, pix_colour);
    }
  }

  IDirectDrawSurface_ReleaseDC( lpdds2, dc );
 return;
}


void
uidisplay_frame_end( void )
{
  InvalidateRect( hWnd, NULL, FALSE );
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
    ClientToScreen( hWnd, &point );
    GetClientRect( hWnd, &srcrec );
  
    destrec.top = point.y;
    destrec.left = point.x;
    destrec.bottom = destrec.top + srcrec.bottom;
    destrec.right = destrec.left + srcrec.right;
  
    IDirectDrawSurface_Blt( lpdds, &destrec, 
      lpdds2, &srcrec, DDBLT_WAIT, NULL );
  }
  return;
}

#endif			/* #ifdef UI_WIN32 */
