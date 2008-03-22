/* memorybrowser.c: the Win32 memory browser
   Copyright (c) 2008 Marek Januszewski

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

#define _WIN32_IE 0x0300 /* needed by LVM_SETEXTENDEDLISTVIEWSTYLE */

#include <libspectrum.h>
#include <tchar.h>

#include "memory.h"
#include "win32internals.h"

#include "memorybrowser.h"

INT_PTR CALLBACK
memorybrowser_proc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

void
memorybrowser_init( HWND hwndDlg );

/* helper constants for memory listview's scrollbar */
const int memorysb_min = 0x0000;
const int memorysb_max = 0xffff;
const int memorysb_step = 0x10;
const int memorysb_page_inc = 0xa0;
const int memorysb_page_size = 0x13f;

/* listview monospaced font */
HFONT hfont;

void
update_display( HWND hwndDlg, libspectrum_word base )
{
  size_t i, j;

  TCHAR buffer[ 8 + 64 + 20 ];
  TCHAR *text[] = { &buffer[0], &buffer[ 8 ], &buffer[ 8 + 64 ] };
  TCHAR buffer2[ 8 ];

  SendDlgItemMessage( hwndDlg, IDC_MEM_LV, LVM_DELETEALLITEMS, 0, 0 );

  LV_ITEM lvi;
  lvi.mask = LVIF_TEXT;

  for( i = 0; i < 20; i++ ) {
    _sntprintf( text[0], 8, TEXT( "%04X" ), base );

    text[1][0] = '\0';
    for( j = 0; j < 0x10; j++, base++ ) {

      libspectrum_byte b = readbyte_internal( base );

      _sntprintf( buffer2, 4, TEXT( "%02X " ), b );
      _tcsncat( text[1], buffer2, 4 );

      text[2][j] = ( b >= 32 && b < 127 ) ? b : '.';
    }
    text[2][ 0x10 ] = '\0';

    /* append the item */
    lvi.iItem = SendDlgItemMessage( hwndDlg, IDC_MEM_LV,
                                    LVM_GETITEMCOUNT, 0, 0 );
    lvi.iSubItem = 0;
    lvi.pszText = text[0];
    SendDlgItemMessage( hwndDlg, IDC_MEM_LV, LVM_INSERTITEM, 0,
                        ( LPARAM ) &lvi );
    lvi.iSubItem = 1;
    lvi.pszText = text[1];
    SendDlgItemMessage( hwndDlg, IDC_MEM_LV, LVM_SETITEM, 0,
                        ( LPARAM ) &lvi );
    lvi.iSubItem = 2;
    lvi.pszText = text[2];
    SendDlgItemMessage( hwndDlg, IDC_MEM_LV, LVM_SETITEM, 0,
                        ( LPARAM ) &lvi );
  }
}

void
scroller( HWND hwndDlg, WPARAM scroll_command )
{
  libspectrum_word base;
  SCROLLINFO si;

  si.cbSize = sizeof(si); 
  si.fMask = SIF_POS; 
  GetScrollInfo( GetDlgItem( hwndDlg, IDC_MEM_SB ), SB_CTL, &si );

  int value = si.nPos;
  
  /* in Windows we have to read the command and scroll the scrollbar manually */
  switch( LOWORD( scroll_command ) ) {
    case SB_BOTTOM:
      value = memorysb_max;
      break;
    case SB_TOP:
      value = memorysb_min;
      break;
    case SB_LINEDOWN:
      value += memorysb_step;
      break;
    case SB_LINEUP:
      value -= memorysb_step;
      break;
    case SB_PAGEUP:
      value -= memorysb_page_inc;
      break;
    case SB_PAGEDOWN:
      value += memorysb_page_inc;
      break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
      value = HIWORD( scroll_command );
      break;
  }
  if( value > memorysb_max ) value = memorysb_max;
  if( value < memorysb_min ) value = memorysb_min;

  /* Drop the low bits before displaying anything */
  base = value; base &= 0xfff0;

  /* set the new scrollbar position */
  memset( &si, 0, sizeof(si) );
  si.cbSize = sizeof(si); 
  si.fMask = SIF_POS; 
  si.nPos = base;
  SetScrollInfo( GetDlgItem( hwndDlg, IDC_MEM_SB ), SB_CTL, &si, TRUE );
  
  update_display( hwndDlg, base );
}

void
menu_machine_memorybrowser()
{
  DialogBox( fuse_hInstance, MAKEINTRESOURCE( IDD_MEM ),
             fuse_hWnd, (DLGPROC) memorybrowser_proc );
  return;
}

INT_PTR CALLBACK
memorybrowser_proc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch( uMsg )
  {
    case WM_INITDIALOG:
      memorybrowser_init( hwndDlg );
      return TRUE;

    case WM_COMMAND:
      if( LOWORD( wParam ) == IDC_MEM_BTN_CLOSE ) {
        EndDialog( hwndDlg, 0 );
        DeleteObject( hfont );
        hfont = NULL;
        return TRUE;
      }
      return FALSE;

    case WM_CLOSE: {
      EndDialog( hwndDlg, 0 );
      DeleteObject( hfont );
      hfont = NULL;
      return TRUE;
    }

    case WM_VSCROLL:
      if( ( HWND ) lParam != NULL ) {
        scroller( hwndDlg, wParam );
        return 0;
      }
      return FALSE;
      
    case WM_MOUSEWHEEL:
/* FIXME: when one of the lines in the listview is selected,
          mouse wheel doesn't work */      
      scroller( hwndDlg, MAKEWPARAM( 
                ( ( short ) HIWORD( wParam ) < 0 ) ? SB_LINEDOWN : SB_LINEUP,
                0 ) );
      return 0;
  }
  return FALSE;
}

void
memorybrowser_init( HWND hwndDlg )
{
  size_t i;

  TCHAR *titles[] = { "Address", "Hex", "Data" };
  int column_widths[] = { 62, 348, 124 };

  /* set extended listview style to select full row, when an item is selected */
  DWORD lv_ext_style;
  lv_ext_style = SendDlgItemMessage( hwndDlg, IDC_MEM_LV,
                                     LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 ); 
  lv_ext_style |= LVS_EX_FULLROWSELECT;
  SendDlgItemMessage( hwndDlg, IDC_MEM_LV,
                      LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lv_ext_style ); 

  /* create columns */
  LVCOLUMN lvc;
  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
  lvc.fmt = LVCFMT_LEFT;

  for( i = 0; i < 3; i++ ) {
    if( i != 0 )
      lvc.mask |= LVCF_SUBITEM;
    lvc.cx = column_widths[i];
    lvc.pszText = titles[i];
    SendDlgItemMessage( hwndDlg, IDC_MEM_LV, LVM_INSERTCOLUMN, i,
                        ( LPARAM ) &lvc );
  }

  /* set the scrollbar parameters */
  SCROLLINFO si;
  si.cbSize = sizeof(si); 
  si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE; 
  si.nPos = 0;
  si.nMin = memorysb_min;
  si.nMax = memorysb_max;
  si.nPage = memorysb_page_size;
  SetScrollInfo( GetDlgItem( hwndDlg, IDC_MEM_SB ), SB_CTL, &si, TRUE );
/* FIXME: if the scrollbar is position on the bottom, clicking on the down arrow
          reveals address 0x0000 */
  
  /* set font of the listview to monospaced one */
  hfont = CreateFont( -11, 0, 0, 0, 400, FALSE, FALSE, FALSE, 0, 400, 2,
                            1, 1, TEXT( "Courier New" ) );
  SendDlgItemMessage( hwndDlg, IDC_MEM_LV , WM_SETFONT,
                      (WPARAM) hfont, FALSE );

  update_display( hwndDlg, 0x0000 );
}
