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

#include "win32internals.h"
#include "../../pokefinder/pokefinder.h"

#include "pokefinder.h"

void win32_pokefinder_incremented();
void win32_pokefinder_decremented();
void win32_pokefinder_search();
void win32_pokefinder_reset();
void win32_pokefinder_close();
void update_pokefinder( void );
/*
void possible_click( GtkCList *clist, gint row, gint column,
			    GdkEventButton *event, gpointer user_data );
*/

#define MAX_POSSIBLE 20

int possible_page[ MAX_POSSIBLE ];
libspectrum_word possible_offset[ MAX_POSSIBLE ];

BOOL CALLBACK PokefinderProc( HWND hWnd, UINT msg,
			      WPARAM wParam, LPARAM lParam )
{
  switch( msg )
  {
    case WM_COMMAND:
      switch( LOWORD( wParam ) )
      {
	case IDC_PF_CLOSE:
	  win32_pokefinder_close();
	  return TRUE;
	case IDC_PF_INC:
	  win32_pokefinder_incremented();
	  return TRUE;
	case IDC_PF_DEC:
	  win32_pokefinder_decremented();
	  return TRUE;
	case IDC_PF_SEARCH:
	  win32_pokefinder_search();
	  return TRUE;
	case IDC_PF_RESET:
	  win32_pokefinder_reset();
	  return TRUE;
      }
      return FALSE;
    case WM_CLOSE:
      win32_pokefinder_close();
      return TRUE;
  }
  return FALSE;
}

void
update_pokefinder( void )
{
  size_t page, offset;
  char buffer[256], *possible_text[2] = { &buffer[0], &buffer[128] };

  HWND hLocListView;
  hLocListView = GetDlgItem( fuse_hPFWnd, IDC_PF_LIST );

  /* delete the whole list */
  ListView_DeleteAllItems( hLocListView );

  /* display suspected locations if < 20 */
  int i;
  i = 0;
  LV_ITEM lvi;
  lvi.mask = LVIF_TEXT;

  if( pokefinder_count && pokefinder_count <= MAX_POSSIBLE ) {

    size_t which;

    which = 0;

    for( page = 0; page < 8; page++ )
      for( offset = 0; offset < 0x4000; offset++ )
	    if( pokefinder_possible[page][offset] != -1 )
	{
	  possible_page[ which ] = page;
	  possible_offset[ which ] = offset;
	  which++;

	  snprintf( possible_text[0], 128, "%lu", (unsigned long)page );
	  snprintf( possible_text[1], 128, "0x%04X", (unsigned)offset );

	  lvi.iItem = i;
	  lvi.iSubItem = 0;
	  lvi.pszText = possible_text[0];
	  ListView_InsertItem( hLocListView, &lvi );
	  ListView_SetItemText( hLocListView, i, 1, possible_text[1]);
	  i++;
	}
  }

  /* set number of possible locations */
  HWND hPossibleLocs;
  hPossibleLocs = GetDlgItem( fuse_hPFWnd, IDC_PF_LOCATIONS );

  snprintf( buffer, 256, "Possible locations: %lu",
	    (unsigned long)pokefinder_count );
  SendMessage( hPossibleLocs, WM_SETTEXT, 0, (LPARAM) buffer );
/*
  TODO: why doesn't it display 131072???
*/
}

void
menu_machine_pokefinder( int action )
{
  if (fuse_hPFWnd == NULL)
  {
    fuse_hPFWnd = CreateDialog( fuse_hInstance, "DLG_POKEFINDER", fuse_hWnd,
      (DLGPROC) PokefinderProc );
    win32_verror( fuse_hPFWnd == NULL );

    /* get handle to the listview and set kind and the column names */
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
  }
  else
  {
    SetActiveWindow( fuse_hPFWnd );
  }
}

void
win32_pokefinder_incremented()
{
  pokefinder_incremented();
  update_pokefinder();
}

void
win32_pokefinder_decremented()
{
  pokefinder_decremented();
  update_pokefinder();
}

void
win32_pokefinder_search()
{
  char buffer[256];

  HWND hEditBox;
  hEditBox = GetDlgItem( fuse_hPFWnd, IDC_PF_EDIT );

  SendMessage( hEditBox, WM_GETTEXT, (WPARAM) 256, (LPARAM) buffer );

  pokefinder_search( atoi( buffer ) );
  update_pokefinder();
}

void
win32_pokefinder_reset()
{
  pokefinder_clear();
  update_pokefinder();
}

void
win32_pokefinder_close()
{
  DestroyWindow( fuse_hPFWnd );
  fuse_hPFWnd = NULL;
}

/* TODO: implement clicking on value in the location
	 list to set the breakpoint
*/
