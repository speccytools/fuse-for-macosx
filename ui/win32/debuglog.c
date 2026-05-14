/* debuglog.c: modeless window streaming Spectranext stdout (port 0x043b)
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include "compat.h"
#include "win32internals.h"

#include "debuglog.h"

#define DEBUGLOG_CAP (256 * 1024)

static CRITICAL_SECTION debuglog_cs;
static int debuglog_cs_inited;

static char debuglog_buf[DEBUGLOG_CAP + 1];
static size_t debuglog_len;

static HWND debuglog_hwnd;
static HWND debuglog_edit_hwnd;

static UINT_PTR debuglog_timer_id = 1;
/* Set by append; cleared when UI has been updated (avoid redundant SetWindowText). */
static volatile LONG debuglog_dirty;
static HBRUSH debuglog_edit_bg_brush;

/* True if vertical scrollbar thumb is at (or near) the bottom. */
static int
debuglog_edit_at_bottom( HWND edit )
{
  SCROLLINFO si;

  if( !edit || !IsWindow( edit ) )
    return 1;

  memset( &si, 0, sizeof( si ) );
  si.cbSize = sizeof( SCROLLINFO );
  si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
  if( !GetScrollInfo( edit, SB_VERT, &si ) )
    return 1;

  /* No scrollbar or all text fits */
  if( si.nMax <= 0 || (int)si.nPage >= si.nMax )
    return 1;

  /* Near bottom: thumb range is [0 .. nMax - nPage] (approximately). */
  return (int)si.nPos + (int)si.nPage >= si.nMax - 1;
}

/* Scroll multiline EDIT so line `target_first` is the first visible line (best effort). */
static void
debuglog_edit_set_first_visible( HWND edit, int target_first )
{
  int guard = 0;

  if( !edit || !IsWindow( edit ) )
    return;

  while( guard++ < 5000 ) {
    int cur = (int)SendMessage( edit, EM_GETFIRSTVISIBLELINE, 0, 0 );
    if( cur == target_first )
      return;
    /* Positive lParam: text scrolls up → first visible line index increases (MSDN). */
    SendMessage( edit, EM_LINESCROLL, 0, target_first - cur );
    {
      int nxt = (int)SendMessage( edit, EM_GETFIRSTVISIBLELINE, 0, 0 );
      if( nxt == cur )
        return;
    }
  }
}

/* Show end of log after full text replace (SetWindowText resets scroll). */
static void
debuglog_edit_scroll_to_bottom( HWND edit )
{
  int lc, fv, need;
  RECT rc;
  TEXTMETRIC tm;
  HDC hdc;
  HFONT font, old;
  int vis;

  if( !edit || !IsWindow( edit ) )
    return;

  lc = (int)SendMessage( edit, EM_GETLINECOUNT, 0, 0 );
  fv = (int)SendMessage( edit, EM_GETFIRSTVISIBLELINE, 0, 0 );
  GetClientRect( edit, &rc );
  hdc = GetDC( edit );
  font = (HFONT)SendMessage( edit, WM_GETFONT, 0, 0 );
  old = font ? (HFONT)SelectObject( hdc, font ) : NULL;
  if( !GetTextMetrics( hdc, &tm ) ) {
    if( old )
      SelectObject( hdc, old );
    ReleaseDC( edit, hdc );
    SendMessage( edit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1 );
    SendMessage( edit, EM_SCROLLCARET, 0, 0 );
    return;
  }
  if( old )
    SelectObject( hdc, old );
  ReleaseDC( edit, hdc );

  vis = ( rc.bottom - rc.top ) / tm.tmHeight;
  if( vis < 1 )
    vis = 1;

  /* Scroll up until last line is in view (or close). */
  need = lc - fv - vis;
  if( need > 0 )
    SendMessage( edit, EM_LINESCROLL, 0, need );

  SendMessage( edit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1 );
  SendMessage( edit, EM_SCROLLCARET, 0, 0 );
}

static void
debuglog_ensure_cs( void )
{
  if( !debuglog_cs_inited ) {
    InitializeCriticalSection( &debuglog_cs );
    debuglog_cs_inited = 1;
  }
}

static void
debuglog_refresh_edit( void )
{
  char *copy;
  int scroll_to_bottom;
  int saved_first = 0;

  if( !debuglog_edit_hwnd || !IsWindow( debuglog_edit_hwnd ) )
    return;

  scroll_to_bottom = debuglog_edit_at_bottom( debuglog_edit_hwnd );
  if( !scroll_to_bottom )
    saved_first = (int)SendMessage( debuglog_edit_hwnd, EM_GETFIRSTVISIBLELINE, 0, 0 );

  EnterCriticalSection( &debuglog_cs );
  copy = malloc( debuglog_len + 1 );
  if( copy ) {
    memcpy( copy, debuglog_buf, debuglog_len + 1 );
  }
  LeaveCriticalSection( &debuglog_cs );

  if( !copy )
    return;

  SetWindowTextA( debuglog_edit_hwnd, copy );
  free( copy );

  if( scroll_to_bottom ) {
    debuglog_edit_scroll_to_bottom( debuglog_edit_hwnd );
  } else {
    /* SetWindowText resets vertical scroll to the top; restore the prior view. */
    debuglog_edit_set_first_visible( debuglog_edit_hwnd, saved_first );
  }
}

static void
debuglog_mark_dirty( void )
{
  InterlockedExchange( &debuglog_dirty, 1 );
}

static LRESULT CALLBACK
debuglog_wnd_proc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch( msg ) {

    case WM_TIMER:
      if( wParam == debuglog_timer_id ) {
        if( InterlockedExchange( &debuglog_dirty, 0 ) )
          debuglog_refresh_edit();
      }
      return 0;

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
      /* Read-only multiline edits often use CTLCOLORSTATIC. */
      if( (HWND)lParam == debuglog_edit_hwnd && debuglog_edit_hwnd ) {
        HDC hdc = (HDC)wParam;
        SetBkColor( hdc, RGB( 255, 255, 255 ) );
        SetTextColor( hdc, RGB( 0, 0, 0 ) );
        if( !debuglog_edit_bg_brush )
          debuglog_edit_bg_brush = CreateSolidBrush( RGB( 255, 255, 255 ) );
        return (LRESULT)debuglog_edit_bg_brush;
      }
      break;

    case WM_SIZE:
      if( debuglog_edit_hwnd ) {
        RECT r;
        GetClientRect( hwnd, &r );
        MoveWindow( debuglog_edit_hwnd, 0, 0, r.right, r.bottom, TRUE );
      }
      return DefWindowProc( hwnd, msg, wParam, lParam );

    case WM_CLOSE:
      KillTimer( hwnd, debuglog_timer_id );
      DestroyWindow( hwnd );
      return 0;

    case WM_DESTROY:
      if( debuglog_edit_bg_brush ) {
        DeleteObject( debuglog_edit_bg_brush );
        debuglog_edit_bg_brush = NULL;
      }
      debuglog_hwnd = NULL;
      debuglog_edit_hwnd = NULL;
      return 0;

    default:
      return DefWindowProc( hwnd, msg, wParam, lParam );
  }
}

static void
debuglog_register_class( void )
{
  static int registered;

  if( registered )
    return;

  {
    WNDCLASS wc;
    memset( &wc, 0, sizeof( wc ) );
    wc.lpfnWndProc = debuglog_wnd_proc;
    wc.hInstance = fuse_hInstance;
    wc.lpszClassName = TEXT( "FuseSpectranextDebugLog" );
    wc.hbrBackground = (HBRUSH)GetStockObject( WHITE_BRUSH );
    wc.hCursor = LoadCursor( NULL, IDC_ARROW );
    if( !RegisterClass( &wc ) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS )
      return;
    registered = 1;
  }
}

void
win32_debuglog_show( void )
{
  RECT r;

  debuglog_ensure_cs();

  if( debuglog_hwnd && IsWindow( debuglog_hwnd ) ) {
    ShowWindow( debuglog_hwnd, SW_SHOW );
    SetForegroundWindow( debuglog_hwnd );
    return;
  }

  debuglog_register_class();

  /* Centre on main Fuse window */
  SetRect( &r, 0, 0, 640, 400 );
  if( fuse_hWnd && IsWindow( fuse_hWnd ) ) {
    RECT fr;
    GetWindowRect( fuse_hWnd, &fr );
    OffsetRect( &r, fr.left + ( fr.right - fr.left - 640 ) / 2,
                fr.top + ( fr.bottom - fr.top - 400 ) / 2 );
  }

  debuglog_hwnd = CreateWindowEx(
    0, TEXT( "FuseSpectranextDebugLog" ),
    TEXT( "Spectranext debug log" ),
    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
    r.left, r.top, r.right - r.left, r.bottom - r.top,
    fuse_hWnd, NULL, fuse_hInstance, NULL
  );

  if( !debuglog_hwnd )
    return;

  /* No ES_AUTOHSCROLL: multiline edit wraps at the control width. */
  debuglog_edit_hwnd = CreateWindowEx(
    0, TEXT( "EDIT" ), TEXT( "" ),
    WS_CHILD | WS_VISIBLE | WS_VSCROLL |
      ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
    0, 0, 0, 0,
    debuglog_hwnd, (HMENU)1, fuse_hInstance, NULL
  );

  {
    HFONT mono = NULL;
    if( win32ui_get_monospaced_font( &mono ) == 0 && mono )
      SendMessage( debuglog_edit_hwnd, WM_SETFONT, (WPARAM)mono, 0 );
  }

  /* Coarse timer + dirty flag: only repaint when new data arrived. */
  SetTimer( debuglog_hwnd, debuglog_timer_id, 500, NULL );
  debuglog_refresh_edit();
  ShowWindow( debuglog_hwnd, SW_SHOW );
}

void
win32_debuglog_append_byte( int c )
{
  debuglog_ensure_cs();

  EnterCriticalSection( &debuglog_cs );

  if( debuglog_len + 2 >= DEBUGLOG_CAP ) {
    size_t drop = DEBUGLOG_CAP / 4;
    memmove( debuglog_buf, debuglog_buf + drop, debuglog_len - drop );
    debuglog_len -= drop;
  }

  if( c == '\n' )
    debuglog_buf[debuglog_len++] = '\r';
  debuglog_buf[debuglog_len++] = (char)c;
  debuglog_buf[debuglog_len] = '\0';

  LeaveCriticalSection( &debuglog_cs );
  debuglog_mark_dirty();
}

void
win32_debuglog_end( void )
{
  if( debuglog_hwnd && IsWindow( debuglog_hwnd ) )
    DestroyWindow( debuglog_hwnd );
  debuglog_hwnd = NULL;
  debuglog_edit_hwnd = NULL;

  if( debuglog_cs_inited ) {
    DeleteCriticalSection( &debuglog_cs );
    debuglog_cs_inited = 0;
  }
}

void
menu_machine_debuglog( int action GCC_UNUSED )
{
  win32_debuglog_show();
}
