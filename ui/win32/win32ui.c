/* win32ui.c: Win32 routines for dealing with the user interface
   Copyright (c) 2003-2007 Marek Januszewski, Philip Kendall, Stuart Brady

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

#include "debugger/debugger.h"
#include "display.h"
#include "fuse.h"
#include "machine_select.h"
#include "menu.h"
#include "menu_data.h"
#include "psg.h"
#include "rzx.h"
#include "screenshot.h"
#include "settings.h"
#include "snapshot.h"
#include "timer/timer.h"
#include "ui/ui.h"
#include "ui/uijoystick.h"
#include "utils.h"
#include "win32internals.h"
#include "win32keyboard.h"
#include "win32display.h"

HACCEL hAccels;

HWND fuse_hWnd;
HINSTANCE fuse_hInstance;
HWND fuse_hStatusWindow;
HWND fuse_hPFWnd;
HWND fuse_hDBGWnd;
HFONT h_ms_font = NULL; /* machine select dialog's font object */

int paused = 0;
int size_paused = 0;
int selected_machine = 0; /* this variable keeps the currently selected machine in the Machine Select dialog */

#define STUB do { printf("STUB: %s()\n", __func__); fflush(stdout); } while(0)

int
win32ui_confirm( const char *string )
{
  int result;

  fuse_emulation_pause();
  result = MessageBox( fuse_hWnd, string, "Fuse - Confirm",
                       MB_YESNO|MB_ICONQUESTION ) == IDYES;
  fuse_emulation_unpause();
  return result;
}

void
handle_drop( HDROP hDrop )
{
  size_t bufsize;
  char *namebuf;

  /* Check that only one file was dropped */
  if( DragQueryFile( hDrop, ~0UL, NULL, 0 ) == 1) {
    bufsize = DragQueryFile( hDrop, 0, NULL, 0 ) + 1;
    if( ( namebuf = malloc( bufsize ) ) ) {
      DragQueryFile( hDrop, 0, namebuf, bufsize );

      fuse_emulation_pause();

      utils_open_file( namebuf, settings_current.auto_load, NULL );

      free( namebuf );

      display_refresh_all();

      fuse_emulation_unpause();
    }
  }
  DragFinish( hDrop );
}

LRESULT WINAPI
MainWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch( msg ) {
    case WM_CREATE:
      win32statusbar_create( hWnd );
      break;

    case WM_COMMAND:
      handle_menu( LOWORD( wParam ), hWnd );
      break;

    case WM_DROPFILES:
      handle_drop( ( HDROP )wParam );
      break;

    case WM_CLOSE:
      if( win32ui_confirm( "Exit Fuse?" ) ) {
        DestroyWindow(hWnd);
      }
      break;

    case WM_KEYDOWN:
      win32keyboard_keypress( wParam, lParam );
      break;

    case WM_KEYUP:
      win32keyboard_keyrelease( wParam, lParam );
      break;

    case WM_PAINT:
      if( display_ui_initialised ) {
        PAINTSTRUCT ps;
        BeginPaint( hWnd, &ps );
        blit();
        EndPaint( hWnd, &ps );
      }
      break;

    case WM_SIZING:
    {
      RECT *selr, wr, cr /*, statr */; /* FIXME: unused variable - is it needed? */
      int width, height, size, w_ofs, h_ofs;

      selr = (RECT *)lParam;
      GetWindowRect( fuse_hWnd, &wr );
      GetClientRect( fuse_hWnd, &cr );
/*      GetClientRect( fuse_hStatusWindow, &statr ); */

      w_ofs = ( wr.right - wr.left ) - ( cr.right - cr.left );
      h_ofs = ( wr.bottom - wr.top ) - ( cr.bottom - cr.top );
/*      + ( statr.bottom - statr.top ); */

      width = selr->right - selr->left + DISPLAY_ASPECT_WIDTH / 2;
      height = selr->bottom - selr->top + DISPLAY_SCREEN_HEIGHT / 2;

      width -= w_ofs; height -= h_ofs;
      width /= DISPLAY_ASPECT_WIDTH; height /= DISPLAY_SCREEN_HEIGHT;

      if( wParam == WMSZ_LEFT || wParam == WMSZ_RIGHT ) {
        height = width;
      } else if( wParam == WMSZ_TOP || wParam == WMSZ_BOTTOM ) {
        width = height;
      } else if( width < 1 || height < 1 ) {
        width = 1; height = 1;
      }

      if( width > 3 ) {
        width = 3;
      }
      if( height > 3 ) {
        height = 3;
      }

      if( width < height ) {
        height = width;
      } else {
        width = height;
      }
      size = width;

      width *= DISPLAY_ASPECT_WIDTH; height *= DISPLAY_SCREEN_HEIGHT;
      width += w_ofs; height += h_ofs;

      if( wParam == WMSZ_TOP ||
          wParam == WMSZ_TOPLEFT ||
          wParam == WMSZ_TOPRIGHT ) {
        selr->top = selr->bottom - height;
      } else {
        selr->bottom = selr->top + height;
      }
      if( wParam == WMSZ_LEFT ||
          wParam == WMSZ_TOPLEFT ||
          wParam == WMSZ_BOTTOMLEFT ) {
        selr->left = selr->right - width;
      } else {
        selr->right = selr->left + width;
      }
      win32display_resize( size );

      return TRUE;
    }

    case WM_SIZE:
      if( win32display_sizechanged )
        win32display_resize_update();
/*      SendMessage( fuse_hStatusWindow, WM_SIZE, wParam, lParam ); */
      if( wParam == SIZE_MINIMIZED ) {
        if( !size_paused ) {
          size_paused = 1;
          fuse_emulation_pause();
        }
      } else {
        if( size_paused ) {
          size_paused = 0;
          fuse_emulation_unpause();
        }
      }

/*
      RECT r;
      GetClientRect( fuse_hStatusWindow, &r );
      int cornerwidth = r.bottom - r.top;
      int parts[2] = { r.right - cornerwidth - 65, r.right - cornerwidth };
      SendMessage( fuse_hStatusWindow, SB_SETPARTS, (WPARAM) 2,
                   (LPARAM) parts );
*/
      return 0;

    case WM_DESTROY:
      fuse_exiting = 1;
      PostQuitMessage( 0 );

      /* Stop the paused state to allow us to exit (occurs from main
         emulation loop) */
      if( paused ) menu_machine_pause( 0 );
      break;

    case WM_ENTERMENULOOP:
    case WM_ENTERSIZEMOVE:
    {
      fuse_emulation_pause();
      break;
    }

    case WM_EXITMENULOOP:
    case WM_EXITSIZEMOVE:
    {
      fuse_emulation_unpause();
      break;
    }

    default:
      return( DefWindowProc( hWnd, msg, wParam, lParam ) );
  }

  return 0;
}

/* this is where windows program begins */
int WINAPI
WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
         int nCmdShow )
{
  WNDCLASS wc;

  if( !hPrevInstance ) {
    wc.lpszClassName = "Fuse";
    wc.lpfnWndProc = MainWndProc;
    wc.style = CS_OWNDC;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, "win32_icon" );
    wc.hCursor = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)( COLOR_WINDOW+1 );
    wc.lpszMenuName = "win32_menu";
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;

    if( !RegisterClass( &wc ) )
      return 0;
  }

  fuse_hInstance = hInstance;

  fuse_hWnd = CreateWindow( "Fuse", "Fuse", WS_OVERLAPPED | WS_CAPTION |
    WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    NULL, NULL, hInstance, NULL );

  /* init windows controls such as the status bar */
  InitCommonControls();

  fuse_nCmdShow = nCmdShow;
  UpdateWindow( fuse_hWnd );

  hAccels = LoadAccelerators( fuse_hInstance, "win32_accel" );

  DragAcceptFiles( fuse_hWnd, TRUE );

  return fuse_main(__argc, __argv);
  /* finish - how do deal with returning wParam */
}

int
ui_init( int *argc, char ***argv )
{
  if( win32display_init() ) return 1;

  return 0;
}

int
ui_event( void )
{
  MSG msg;

  do {

    /* Process messages */
    while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
      if( !IsDialogMessage( fuse_hPFWnd, &msg  ) &&
          !IsDialogMessage( fuse_hDBGWnd, &msg ) ) {
        if( !TranslateAccelerator( fuse_hWnd, hAccels, &msg ) ) {
          if( msg.message == WM_QUIT ) break;
          /* finish - set exit flag somewhere */
          TranslateMessage( &msg );
          DispatchMessage( &msg );
        }
      }
    }

    if( fuse_emulation_paused )
      Sleep( 25 );

  } while( fuse_emulation_paused );

  return 0;
  /* finish - somwhere there should be return msg.wParam */
}

int
ui_end( void )
{
  int error;

  error = win32display_end(); if( error ) return error;
  return 0;
}

void
win32_verror( int is_error )
{
  if( !is_error ) return;

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
menu_machine_debugger( int action )
{
  debugger_mode = DEBUGGER_MODE_HALTED;
  if( paused ) ui_debugger_activate();
}

int
ui_mouse_grab( int startup )
{
  STUB;
  return 0;
}

int
ui_mouse_release( int suspend )
{
  STUB;
  return 0;
}

int
ui_joystick_init( void )
{
  STUB;
  return 0;
}

void
ui_joystick_end( void )
{
  STUB;
}

void
ui_joystick_poll( void )
{
  /* STUB; */
}

int
ui_widgets_reset( void )
{
  STUB;
  return 0;
}

int
menu_select_roms_with_title( const char *title, size_t start, size_t count )
{
  STUB;
  return 0;
}

scaler_type
menu_get_scaler( scaler_available_fn selector )
{
  STUB;
  return SCALER_NORMAL;
}

char *
win32ui_get_filename( const char *title, int is_saving )
{
  OPENFILENAME ofn;
  char szFile[512];
  int result;

  memset( &ofn, 0, sizeof( ofn ) );
  szFile[0] = '\0';

  ofn.lStructSize = sizeof( ofn );
  ofn.hwndOwner = fuse_hWnd;
  ofn.lpstrFilter = "All Files\0*.*\0\0";
  ofn.lpstrCustomFilter = NULL;
  ofn.nFilterIndex = 0;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof( szFile );
  ofn.lpstrFileTitle = NULL;
  ofn.lpstrInitialDir = NULL;
  ofn.lpstrTitle = title;
  ofn.Flags = /* OFN_DONTADDTORECENT | */ OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
  if( is_saving ) {
    ofn.Flags |= OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN;
  } else {
    ofn.Flags |= OFN_FILEMUSTEXIST;
  }
  ofn.nFileOffset = 0;
  ofn.nFileExtension = 0;
  ofn.lpstrDefExt = NULL;
/* ofn.pvReserved = NULL; */
/* ofn.FlagsEx = 0; */

  if( is_saving ) {
    result = GetSaveFileName( &ofn );
  } else {
    result = GetOpenFileName( &ofn );
  }

  if( !result ) {
    return NULL;
  } else {
    return strdup( ofn.lpstrFile );
  }
}

char *
ui_get_open_filename( const char *title )
{
  return win32ui_get_filename( title, 0 );
}

char *
ui_get_save_filename( const char *title )
{
  return win32ui_get_filename( title, 1 );
}

ui_confirm_save_t
ui_confirm_save_specific( const char *message )
{
  STUB;
  return UI_CONFIRM_SAVE_CANCEL;
}

ui_confirm_joystick_t
ui_confirm_joystick( libspectrum_joystick libspectrum_type, int inputs )
{
  STUB;
  return UI_CONFIRM_JOYSTICK_NONE;
}

int
ui_tape_browser_update( ui_tape_browser_update_type change,
                        libspectrum_tape_block *block )
{
  STUB;
  return 0;
}

int
ui_get_rollback_point( GSList *points )
{
  STUB;
  return -1;
}

int
ui_menu_item_set_active( const char *path, int active )
{
  /* STUB; */
  return 1;
}

void
menu_file_savesnapshot( int action )
{
  char *filename;

  fuse_emulation_pause();

  filename = ui_get_save_filename( "Fuse - Save Snapshot" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  snapshot_write( filename );

  free( filename );

  fuse_emulation_unpause();
}

void
menu_file_recording_record( int action )
{
  char *recording;

  if( rzx_playback || rzx_recording ) return;

  fuse_emulation_pause();

  recording = ui_get_save_filename( "Fuse - Start Recording" );
  if( !recording ) { fuse_emulation_unpause(); return; }

  rzx_start_recording( recording, 1 );

  free( recording );

  fuse_emulation_unpause();
}

void
menu_file_recording_recordfromsnapshot( int action )
{
  char *snap, *recording;

  if( rzx_playback || rzx_recording ) return;

  fuse_emulation_pause();

  snap = ui_get_open_filename( "Fuse - Load Snapshot " );
  if( !snap ) { fuse_emulation_unpause(); return; }

  recording = ui_get_save_filename( "Fuse - Start Recording" );
  if( !recording ) { free( snap ); fuse_emulation_unpause(); return; }

  if( snapshot_read( snap ) ) {
    free( snap ); free( recording ); fuse_emulation_unpause(); return;
  }

  rzx_start_recording( recording, settings_current.embed_snapshot );

  free( recording );

  display_refresh_all();

  fuse_emulation_unpause();
}

void
menu_file_aylogging_record( int action )
{
  char *psgfile;

  if( psg_recording ) return;

  fuse_emulation_pause();

  psgfile = ui_get_save_filename( "Fuse - Start AY Log" );
  if( !psgfile ) { fuse_emulation_unpause(); return; }

  psg_start_recording( psgfile );

  free( psgfile );

  display_refresh_all();

  ui_menu_activate( UI_MENU_ITEM_AY_LOGGING, 1 );

  fuse_emulation_unpause();
}

void
menu_file_savescreenasscr( int action )
{
  char *filename;

  fuse_emulation_pause();

  filename = ui_get_save_filename( "Fuse - Save Screenshot as SCR" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  screenshot_scr_write( filename );

  free( filename );

  fuse_emulation_unpause();
}

#ifdef USE_LIBPNG
void
menu_file_savescreenaspng( int action )
{
  scaler_type scaler;
  char *filename;

  fuse_emulation_pause();

  scaler = menu_get_scaler( screenshot_available_scalers );
  if( scaler == SCALER_NUM ) {
    fuse_emulation_unpause();
    return;
  }

  filename =
    ui_get_save_filename( "Fuse - Save Screenshot as PNG" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  screenshot_write( filename, scaler );

  free( filename );

  fuse_emulation_unpause();
}
#endif

void
menu_file_movies_recordmovieasscr( int action )
{
  char *filename;
  
  fuse_emulation_pause();

  filename = ui_get_save_filename( "Fuse - Record Movie as SCR" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  snprintf( screenshot_movie_file, PATH_MAX-SCREENSHOT_MOVIE_FILE_MAX, "%s",
            filename );

  screenshot_movie_record = 1;
  ui_menu_activate( UI_MENU_ITEM_FILE_MOVIES_RECORDING, 1 );

  free( filename );

  fuse_emulation_unpause();
}

#ifdef USE_LIBPNG
void
menu_file_movies_recordmovieaspng( int action )
{
  scaler_type scaler;
  char *filename;

  fuse_emulation_pause();

  scaler = menu_get_scaler( screenshot_available_scalers );
  if( scaler == SCALER_NUM ) {
    fuse_emulation_unpause();
    return;
  }

  filename = ui_get_save_filename( "Fuse - Save Screenshot as PNG" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  screenshot_write( filename, scaler );

  free( filename );

  fuse_emulation_unpause();
}
#endif

void menu_file_loadbinarydata( int action ) { STUB; }
void menu_file_savebinarydata( int action ) { STUB; }

void
menu_file_exit( int action )
{
  if( win32ui_confirm( "Exit Fuse?" ) ) {
    DestroyWindow(fuse_hWnd);
  }
}

void menu_options_general( int action ) { STUB; }
void menu_options_sound( int action ) { STUB; }
void menu_options_peripherals( int action ) { STUB; }
void menu_options_rzx( int action ) { STUB; }
void menu_options_joysticks_select( int action ) { STUB; }

void
menu_machine_pause( int action )
{
  int error;

  if( paused ) {
    paused = 0;
    ui_statusbar_update( UI_STATUSBAR_ITEM_PAUSED,
                         UI_STATUSBAR_STATE_INACTIVE );
    timer_estimate_reset();
    fuse_emulation_unpause();
  } else {

    /* Stop recording any competition mode RZX file */
    if( rzx_recording && rzx_competition_mode ) {
      ui_error( UI_ERROR_INFO, "Stopping competition mode RZX recording" );
      error = rzx_stop_recording(); if( error ) return;
    }

    paused = 1;
    ui_statusbar_update( UI_STATUSBAR_ITEM_PAUSED, UI_STATUSBAR_STATE_ACTIVE );
    fuse_emulation_pause();
  }
}

void
menu_machine_reset( int action )
{
  if( win32ui_confirm( "Reset?" ) ) {
    if( machine_reset( 0 ) ) {
      ui_error( UI_ERROR_ERROR, "couldn't reset machine: giving up!" );

      /* FIXME: abort() seems a bit extreme here, but it'll do for now */
      fuse_abort();
    }
  }
}

void
menu_help_keyboard( int action )
{
  win32ui_picture( "keyboard.scr", 0 );
}

INT_PTR CALLBACK
menu_machine_select_proc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{

  HWND h_temp_handle;
  int i, pos_y;

  switch( uMsg ) {
    case WM_INITDIALOG: 
    {
      /* create radio buttons */
      pos_y = 8;
      for( i=0; i<machine_count; i++ ) {
        h_temp_handle = CreateWindow( TEXT( "BUTTON" ), libspectrum_machine_name( machine_types[i]->machine ),
                                      /* no need for WS_GROUP since they're all in the same group */  
                                      WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTORADIOBUTTON,
                                      8, pos_y, 155, 16,
                                      hwndDlg, (HMENU) ( IDC_MS_OFFSET + i ), fuse_hInstance, 0 );
        SendMessage( h_temp_handle, WM_SETFONT, (WPARAM) h_ms_font, FALSE );

        /* check the radiobutton corresponding to current machine */
        if( machine_current == machine_types[i] ) {
          selected_machine = i;
          SendMessage( h_temp_handle, BM_SETCHECK, BST_CHECKED, 0 );
        }

        pos_y += 16;
      }

      /* create OK and Cancel buttons */
      pos_y += 6;
      h_temp_handle = CreateWindow( TEXT( "BUTTON" ), TEXT( "&OK" ),
                                    WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                    6, pos_y, 75, 23,
                                    hwndDlg, (HMENU) IDC_MS_OK, fuse_hInstance, 0 );
      SendMessage( h_temp_handle, WM_SETFONT, (WPARAM) h_ms_font, FALSE );

      h_temp_handle = CreateWindow( TEXT( "BUTTON" ), TEXT( "&Cancel" ),
                                    WS_VISIBLE | WS_CHILD | WS_TABSTOP,
                                    90, pos_y, 75, 23,
                                    hwndDlg, (HMENU) IDC_MS_CANCEL, fuse_hInstance, 0 );
      SendMessage( h_temp_handle, WM_SETFONT, (WPARAM) h_ms_font, FALSE );
      
      pos_y += 54;
      /* the following will only change the size of the window */
      SetWindowPos( hwndDlg, NULL, 0, 0, 177, pos_y, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
      
      return TRUE;
    }
    case WM_SETFONT:
    {
      h_ms_font = (HFONT) wParam;
      return TRUE;
    }
    case WM_COMMAND:
    {
      if ( HIWORD( wParam ) != BN_CLICKED ) return 0;

      /* service OK and Cancel buttons */
      switch LOWORD( wParam ) {
        case IDC_MS_OK:
        {
          if( machine_types[ selected_machine ] != machine_current ) {
            machine_select( machine_types[ selected_machine ]->machine );
          }
          EndDialog( hwndDlg, 0 );
          return TRUE;
        }
        case IDC_MS_CANCEL:
        {
          EndDialog( hwndDlg, 0 );
          return TRUE;
        }
      }

      /* service clicking radiobuttons */
      if( ( LOWORD( wParam ) >= IDC_MS_OFFSET ) &&
          ( LOWORD( wParam ) < ( IDC_MS_OFFSET + machine_count ) ) ) {
        if( SendMessage( (HWND) lParam, BM_GETCHECK, 0, 0 ) == BST_CHECKED ) {
          selected_machine = LOWORD( wParam ) - IDC_MS_OFFSET;
        }
        return TRUE;
      }
    }
    case WM_DESTROY:
    {
      EndDialog( hwndDlg, 0 );
      return TRUE;
    }
  }
  return FALSE;
}

void
menu_machine_select( int action )
{
  /* FIXME: fix accelerators for this window */
  /* FIXME: choosing spectrum SE crashes Fuse sound_frame () at sound.c:477 "ay_change[f].ofs = ( ay_change[f].tstates * sfreq ) / cpufreq;" */
  /* FIXME: choosing some Timexes crashes (win32) fuse as well */
  /* FIXME: switching machines changes filter (typically to 1x) */

  /* Stop emulation */
  fuse_emulation_pause();

  /* start the machine select dialog box */
  DialogBox( fuse_hInstance, MAKEINTRESOURCE( IDD_MS_DIALOG ),
             fuse_hWnd, menu_machine_select_proc );

  /* Resume emulation */
  fuse_emulation_unpause();
}

void menu_machine_pokefinder( int action ) { STUB; }
void menu_machine_memorybrowser( int action ) { STUB; }
void menu_media_tape_browse( int action ) { STUB; }
