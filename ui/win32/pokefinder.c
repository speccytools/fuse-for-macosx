/* pokefinder.c: Win32 interface to the poke finder
   Copyright (c) 2004 Marek Januszwski

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

#define _WIN32_IE 0x400 /* needed by LPNMITEMACTIVATE (dbl click on listview) */

#include <tchar.h>
#include <windows.h>

#include "debugger/debugger.h"
#include "pokefinder.h"
#include "pokefinder/pokefinder.h"
#include "ui/ui.h"
#include "win32internals.h"

static void win32ui_pokefinder_incremented();
static void win32ui_pokefinder_decremented();
static void win32ui_pokefinder_search();
static void win32ui_pokefinder_reset();
static void win32ui_pokefinder_close();
static void update_pokefinder( void );
static void possible_click( LPNMITEMACTIVATE lpnmitem );

#define MAX_POSSIBLE 20

int possible_page[ MAX_POSSIBLE ];
libspectrum_word possible_offset[ MAX_POSSIBLE ];

static INT_PTR CALLBACK
win32ui_pokefinder_proc( HWND hWnd, UINT msg,
                WPARAM wParam, LPARAM lParam )
{
  switch( msg ) {
    case WM_COMMAND:
      switch( LOWORD( wParam ) ) {
        case IDC_PF_CLOSE:
          win32ui_pokefinder_close();
          return TRUE;
        case IDC_PF_INC:
          win32ui_pokefinder_incremented();
          return TRUE;
        case IDC_PF_DEC:
          win32ui_pokefinder_decremented();
          return TRUE;
        case IDC_PF_SEARCH:
          win32ui_pokefinder_search();
          return TRUE;
        case IDC_PF_RESET:
          win32ui_pokefinder_reset();
          return TRUE;
      }
      return FALSE;
    case WM_CLOSE:
      win32ui_pokefinder_close();
      return TRUE;
    case WM_NOTIFY:
      switch ( ( ( LPNMHDR ) lParam )->code ) {
        case NM_DBLCLK:
          possible_click( ( LPNMITEMACTIVATE ) lParam );
          return TRUE;
      }
      return FALSE;      
  }
  return FALSE;
}

static void
update_pokefinder( void )
{
  size_t page, offset;
  TCHAR buffer[256], *possible_text[2] = { &buffer[0], &buffer[128] };

  /* clear the listview */
  SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LIST, LVM_DELETEALLITEMS, 0, 0 );

  /* display suspected locations if < 20 */
  int i = 0;
  LV_ITEM lvi;
  lvi.mask = LVIF_TEXT;

  if( pokefinder_count && pokefinder_count <= MAX_POSSIBLE ) {

    size_t which = 0;

    for( page = 0; page < 2 * SPECTRUM_RAM_PAGES; page++ )
      for( offset = 0; offset < 0x2000; offset++ )
	if( ! (pokefinder_impossible[page][offset/8] & 1 << (offset & 7)) ) {

	  possible_page[ which ] = page / 2;
	  possible_offset[ which ] = offset + 8192 * (page & 1);
	  which++;
	
	  _sntprintf( possible_text[0], 128, "%d", (unsigned)page );
	  _sntprintf( possible_text[1], 128, "0x%04X", (unsigned)offset );

          /* set new count of items */
          SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LIST, LVM_SETITEMCOUNT,
                              i, 0 );

          /* add the item */
          lvi.iItem = i;
          lvi.iSubItem = 0;
          lvi.pszText = possible_text[0];
          SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LIST, LVM_INSERTITEM, 0,
                              ( LPARAM ) &lvi );
          lvi.iSubItem = 1;
          lvi.pszText = possible_text[1];
          SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LIST, LVM_SETITEM, 0,
                              ( LPARAM ) &lvi );

          i++;
        }

    /* show the listview */
    ShowWindow( GetDlgItem( fuse_hPFWnd, IDC_PF_LIST ), SW_SHOW );
    
  } else {
    /* hide the listview */
    ShowWindow( GetDlgItem( fuse_hPFWnd, IDC_PF_LIST ), SW_HIDE );
  }

  /* print possible locations */
  _sntprintf( buffer, 256, "Possible locations: %d\n", pokefinder_count );
  SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LOCATIONS, WM_SETTEXT, 0, (LPARAM) buffer );
}

void
menu_machine_pokefinder( int action )
{
  if (fuse_hPFWnd == NULL) {
    /* FIXME: Implement accelerators for this dialog */
    fuse_hPFWnd = CreateDialog( fuse_hInstance,
                                MAKEINTRESOURCE( IDD_POKEFINDER ),
                                fuse_hWnd, 
                                ( DLGPROC ) win32ui_pokefinder_proc );
    if ( fuse_hPFWnd == NULL ) {
      win32_verror( 1 ); /* FIXME: improve this function */
      return;
    }

    /* set extended listview style to select full row, when an item is selected */
    DWORD lv_ext_style;
    lv_ext_style = SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LIST,
                                       LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 ); 
    lv_ext_style |= LVS_EX_FULLROWSELECT;
    SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LIST,
                        LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lv_ext_style ); 

    /* create columns */
    LVCOLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = 114;
    lvc.pszText = TEXT( "Page" );
    SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LIST, LVM_INSERTCOLUMN, 0,
                        ( LPARAM ) &lvc );
    lvc.mask |= LVCF_SUBITEM;
    lvc.cx = 114;
    lvc.pszText = TEXT( "Offset" );
    SendDlgItemMessage( fuse_hPFWnd, IDC_PF_LIST, LVM_INSERTCOLUMN, 1,
                        ( LPARAM ) &lvc );
  } else {
    SetActiveWindow( fuse_hPFWnd );
  }
  update_pokefinder();
}

static void
win32ui_pokefinder_incremented()
{
  pokefinder_incremented();
  update_pokefinder();
}

static void
win32ui_pokefinder_decremented()
{
  pokefinder_decremented();
  update_pokefinder();
}

static void
win32ui_pokefinder_search()
{
  long value;
  TCHAR *buffer;
  int buffer_size; 

  /* poll the size of the value in Search box first */
  buffer_size = SendDlgItemMessage( fuse_hPFWnd, IDC_PF_EDIT, WM_GETTEXTLENGTH,
                                   (WPARAM) 0, (LPARAM) 0 );
  buffer = malloc( ( buffer_size + 1 ) * sizeof( TCHAR ) );
  if( buffer == NULL ) {
    ui_error( UI_ERROR_ERROR, "Out of memory in %s.", __func__ );
    return;
  }

  /* get the value in Search box first */
  if( SendDlgItemMessage( fuse_hPFWnd, IDC_PF_EDIT, WM_GETTEXT,
                          (WPARAM) ( buffer_size + 1 ),
                          (LPARAM) buffer ) != buffer_size ) {
    ui_error( UI_ERROR_ERROR, "Couldn't get the content of the Search text box" );
    return;
  }

  value = _ttol( buffer );
  free( buffer );

  if( value < 0 || value > 255 ) {
    ui_error( UI_ERROR_ERROR, "Invalid value: use an integer from 0 to 255" );
    return;
  }
  
  pokefinder_search( value );
  update_pokefinder();
}

static void
win32ui_pokefinder_reset()
{
  pokefinder_clear();
  update_pokefinder();
}

static void
win32ui_pokefinder_close()
{
  DestroyWindow( fuse_hPFWnd );
  fuse_hPFWnd = NULL;
}

static void
possible_click( LPNMITEMACTIVATE lpnmitem )
{
  /* FIXME: implement equivalent of GTK's select-via-keyboard to enter here */
 
  int error;
  libspectrum_word row;

  if( lpnmitem->iItem < 0 ) return;
        
  row = lpnmitem->iItem;
  
  error = debugger_breakpoint_add_address(
    DEBUGGER_BREAKPOINT_TYPE_WRITE, possible_page[ row ],
    possible_offset[ row ], 0, DEBUGGER_BREAKPOINT_LIFE_PERMANENT, NULL
  );
  if( error ) return;
  
  ui_debugger_update();
}

void
win32ui_pokefinder_clear( void )
{
  pokefinder_clear();
  if( fuse_hPFWnd != NULL ) update_pokefinder();
}
