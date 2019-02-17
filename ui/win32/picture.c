/* picture.c: Win32 routines to draw the keyboard picture
   Copyright (c) 2002-2019 Philip Kendall, Marek Januszewski, Stuart Brady,
                           Sergio Baldoví

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

#include "display.h"
#include "picture.h"
#include "ui/ui.h"
#include "utils.h"
#include "win32internals.h"

#include <windows.h>

/* An RGB image of the keyboard picture */
/* the memory will be allocated by Windows ( height * width * 4 bytes ) */
static libspectrum_byte *picture;
static const int picture_pitch = DISPLAY_ASPECT_WIDTH * 4;

static HWND hDialogPicture = NULL;
static HBITMAP picture_BMP;

static HWND create_dialog( int width, int height );

static void draw_screen( libspectrum_byte *screen, int border );

static LRESULT WINAPI picture_wnd_proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

int
win32ui_picture( const char *filename, int border )
{
  /* Dialog already open? */
  if( hDialogPicture != NULL ) {
    return 0;
  }

    utils_file screen;

    if( utils_read_screen( filename, &screen ) )
      return 1;

    hDialogPicture = create_dialog( DISPLAY_ASPECT_WIDTH,
                                    DISPLAY_SCREEN_HEIGHT );
    if( hDialogPicture == NULL )
      return 1;

    draw_screen( screen.buffer, border );

    utils_close_file( &screen );

  ShowWindow( hDialogPicture, SW_SHOW );
  return 0;
}

static void
inflate_dialog( HWND dialog, int width, int height )
{
  RECT wr_dialog, wr_button, wr_frame;
  HWND button_hWnd, frame_hWnd;

  /* Change the initial size of the dialog to accommodate the picture */
  GetWindowRect( dialog, &wr_dialog );
  OffsetRect( &wr_dialog, -wr_dialog.left, -wr_dialog.top + height );
  if( width > wr_dialog.right ) wr_dialog.right = width;

  SetWindowPos( dialog, NULL, 0, 0, wr_dialog.right, wr_dialog.bottom,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );

  /* Move Close button */
  button_hWnd = GetDlgItem( dialog, IDCLOSE );
  GetWindowRect( button_hWnd, &wr_button );
  MapWindowPoints( 0, dialog, (POINT *)&wr_button, 2 );
  wr_button.left = ( wr_dialog.right -
                     ( wr_button.right - wr_button.left ) ) / 2;
  wr_button.top += height;

  SetWindowPos( button_hWnd, NULL, wr_button.left, wr_button.top, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );

  /* Give any unused space to picture frame */
  frame_hWnd = GetDlgItem( dialog, IDC_PICTURE_FRAME );
  wr_frame.bottom = width;
  wr_frame.right = wr_dialog.right;
  SetWindowPos( frame_hWnd, NULL, 0, 0, wr_frame.right, wr_frame.bottom,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
}

static HWND
create_dialog( int width, int height )
{
  HWND dialog;

  dialog = CreateDialog( fuse_hInstance, MAKEINTRESOURCE( IDD_PICTURE ),
                         fuse_hWnd, (DLGPROC)picture_wnd_proc );

  if( dialog == NULL )
    return NULL;

  inflate_dialog( dialog, width, height );

  /* Create the picture buffer */
  BITMAPINFO picture_BMI;
  memset( &picture_BMI, 0, sizeof( picture_BMI ) );
  picture_BMI.bmiHeader.biSize = sizeof( picture_BMI.bmiHeader );
  picture_BMI.bmiHeader.biWidth = width;
  /* negative to avoid "shep-mode": */
  picture_BMI.bmiHeader.biHeight = -height;
  picture_BMI.bmiHeader.biPlanes = 1;
  picture_BMI.bmiHeader.biBitCount = 32;
  picture_BMI.bmiHeader.biCompression = BI_RGB;
  picture_BMI.bmiHeader.biSizeImage = 0;
  picture_BMI.bmiHeader.biXPelsPerMeter = 0;
  picture_BMI.bmiHeader.biYPelsPerMeter = 0;
  picture_BMI.bmiHeader.biClrUsed = 0;
  picture_BMI.bmiHeader.biClrImportant = 0;
  picture_BMI.bmiColors[0].rgbRed = 0;
  picture_BMI.bmiColors[0].rgbGreen = 0;
  picture_BMI.bmiColors[0].rgbBlue = 0;
  picture_BMI.bmiColors[0].rgbReserved = 0;

  HDC dc = GetDC( dialog );
  picture_BMP = CreateDIBSection( dc, &picture_BMI, DIB_RGB_COLORS,
                                  (void **)&picture, NULL, 0 );
  ReleaseDC( dialog, dc );

  return dialog;
}

static int
draw_frame( LPDRAWITEMSTRUCT drawitem )
{
  if( !( drawitem->itemAction & ODA_DRAWENTIRE ) )
    return FALSE;

  HDC pic_dc = CreateCompatibleDC( drawitem->hDC );
  HBITMAP old_bmp = SelectObject( pic_dc, picture_BMP );

  BITMAP bitmap_info;
  GetObject( picture_BMP, sizeof( BITMAP ), &bitmap_info );

  BitBlt( drawitem->hDC, 0, 0, bitmap_info.bmWidth,
          bitmap_info.bmHeight, pic_dc, 0, 0, SRCCOPY );

  SelectObject( pic_dc, old_bmp );
  DeleteDC( pic_dc );

  return TRUE;
}

static LRESULT WINAPI
picture_wnd_proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch( msg ) {
    case WM_DRAWITEM:
      if( wParam == IDC_PICTURE_FRAME )
        return draw_frame( (LPDRAWITEMSTRUCT) lParam );
      break;

    case WM_COMMAND:
      switch( LOWORD( wParam ) ) {
        case IDCLOSE:
        {
          hDialogPicture = NULL;
          DestroyWindow( hWnd );
          return 0;
        }
      }
      break;

    case WM_CLOSE:
    {
      hDialogPicture = NULL;
      DestroyWindow( hWnd );
      return 0;
    }
  }
  return FALSE;
}

static void
draw_screen( libspectrum_byte *screen, int border )
{
  int i, x, y, ink, paper;
  libspectrum_byte attr, data;

  GdiFlush();

  for( y=0; y < DISPLAY_BORDER_HEIGHT; y++ ) {
    for( x=0; x < DISPLAY_ASPECT_WIDTH; x++ ) {
      *(libspectrum_dword*)( picture + y * picture_pitch + 4 * x ) =
        win32display_colours[border];
      *(libspectrum_dword*)(
          picture +
          ( y + DISPLAY_BORDER_HEIGHT + DISPLAY_HEIGHT ) * picture_pitch +
          4 * x
        ) = win32display_colours[ border ];
    }
  }

  for( y=0; y<DISPLAY_HEIGHT; y++ ) {

    for( x=0; x < DISPLAY_BORDER_ASPECT_WIDTH; x++ ) {
      *(libspectrum_dword*)
        (picture + ( y + DISPLAY_BORDER_HEIGHT) * picture_pitch + 4 * x) =
        win32display_colours[ border ];
      *(libspectrum_dword*)(
          picture +
          ( y + DISPLAY_BORDER_HEIGHT ) * picture_pitch +
          4 * ( x+DISPLAY_ASPECT_WIDTH-DISPLAY_BORDER_ASPECT_WIDTH )
        ) = win32display_colours[ border ];
    }

    for( x=0; x < DISPLAY_WIDTH_COLS; x++ ) {

      attr = screen[ display_attr_start[y] + x ];

      ink = ( attr & 0x07 ) + ( ( attr & 0x40 ) >> 3 );
      paper = ( attr & ( 0x0f << 3 ) ) >> 3;

      data = screen[ display_line_start[y]+x ];

      for( i=0; i<8; i++ ) {
        libspectrum_dword pix =
          win32display_colours[ ( data & 0x80 ) ? ink : paper ];

        /* rearrange pixel components */
        pix = ( pix & 0x0000ff00 ) |
              ( ( pix & 0x000000ff ) << 16 ) |
              ( ( pix & 0x00ff0000 ) >> 16 );

        *(libspectrum_dword*)(
            picture +
            ( y + DISPLAY_BORDER_HEIGHT ) * picture_pitch +
            4 * ( 8 * x + DISPLAY_BORDER_ASPECT_WIDTH + i )
          ) = pix;
        data <<= 1;
      }
    }

  }
}
