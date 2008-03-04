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

/* FIXME: is that needed?
#include <stdio.h>
#include <string.h>
*/

#define _WIN32_IE 0x400 /* needed by LPNMITEMACTIVATE (dbl click on listview) */

#include <libspectrum.h>
#include <tchar.h>
#include <windows.h>
 
#include "debugger/debugger.h"
#include "event.h"
#include "fuse.h"
#include "scld.h"
#include "settings.h"
#include "ui/ui.h"
#include "ula.h"
#include "win32internals.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"
#include "zxcf.h"

#include "debugger.h"

/* FIXME: delete whatever is not needed

#include "machine.h"
#include "memory.h"

 */

/* The various debugger panes */
typedef enum debugger_pane {

  DEBUGGER_PANE_BEGIN = 1,	/* Start marker */

  DEBUGGER_PANE_REGISTERS = DEBUGGER_PANE_BEGIN,
  DEBUGGER_PANE_MEMORYMAP,
  DEBUGGER_PANE_BREAKPOINTS,
  DEBUGGER_PANE_DISASSEMBLY,
  DEBUGGER_PANE_STACK,
  DEBUGGER_PANE_EVENTS,

  DEBUGGER_PANE_END		/* End marker */
} debugger_pane;

int create_dialog( void );
int hide_hidden_panes( void );
void get_pane_menu_item( debugger_pane pane );
void get_pane( debugger_pane pane );
int create_menu_bar( void );
void toggle_display( void );
/* int create_register_display( void ); this function is handled by rc */
/* int create_memory_map( void ); this function is handled by rc */
int create_breakpoints();
int create_disassembly();
int create_stack_display();
void stack_click( LPNMITEMACTIVATE lpnmitem );
int create_events();
void events_click( LPNMITEMACTIVATE lpnmitem );
/* int create_command_entry( void ); this function is handled by rc */
/* int create_buttons( void ); this function is handled by rc */

int activate_debugger( void );
int update_memory_map( void );
int update_breakpoints();
int update_disassembly();
int update_events( void );
void add_event( gpointer data, gpointer user_data GCC_UNUSED );
int deactivate_debugger( void );

void move_disassembly( WPARAM scroll_command );

void evaluate_command();
void win32ui_debugger_done_step();
void win32ui_debugger_done_continue();
void win32ui_debugger_break();
void delete_dialog();
void win32ui_debugger_done_close();
INT_PTR CALLBACK win32ui_debugger_proc( HWND hWnd, UINT msg,
                                        WPARAM wParam, LPARAM lParam );

/* The top line of the current disassembly */
libspectrum_word disassembly_top;

/* helper constants for disassembly listview's scrollbar */
const int disassembly_min = 0x0000;
const int disassembly_max = 0xffff;
const int disassembly_page = 20;

/* Is the debugger window active (as opposed to the debugger itself)? */
int debugger_active;

#define STUB do { printf("STUB: %s()\n", __func__); fflush(stdout); } while(0)

const TCHAR*
format_8_bit( void )
{
  return debugger_output_base == 10 ? TEXT( "%3d" ) : TEXT( "0x%02X" );
}

const TCHAR*
format_16_bit( void )
{
  return debugger_output_base == 10 ? TEXT( "%5d" ) : TEXT( "0x%04X" );
}

int
ui_debugger_activate( void )
{
  int error;

  fuse_emulation_pause();

  /* create_dialog will create the dialog or activate if it exists */
  if( create_dialog() ) return 1;

  error = hide_hidden_panes(); if( error ) return error;
  
  EnableWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_BTN_CONT ), TRUE);
  EnableWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_BTN_BREAK ), FALSE );
  if( !debugger_active ) activate_debugger();

  return 0;
}

int
hide_hidden_panes( void ) /* FIXME: implement */
{
  STUB;
  return 0;
}

void
get_pane_menu_item( debugger_pane pane ) /* FIXME: implement */
{
  STUB;
  return;
}

void
get_pane( debugger_pane pane ) /* FIXME: implement */
{
  STUB;
  return;
}

int
ui_debugger_deactivate( int interruptable )
{
  if( debugger_active ) deactivate_debugger();

  EnableWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_BTN_CONT ),
                !interruptable ? TRUE : FALSE );
  EnableWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_BTN_BREAK ), 
                interruptable ? TRUE : FALSE );

  return 0;
}

int
create_dialog() /* FIXME: implement */
{
  int error;
  debugger_pane i;

  if ( fuse_hDBGWnd == NULL ) {

    fuse_hDBGWnd = CreateDialog( fuse_hInstance, MAKEINTRESOURCE( IDD_DBG ),
      fuse_hWnd, (DLGPROC) win32ui_debugger_proc );

    error = create_breakpoints(); if( error ) return error;

    error = create_disassembly(); if( error ) return error;

    error = create_stack_display(); if( error ) return error;

    error = create_events(); if( error ) return error;

    /* Initially, have all the panes visible */
    for( i = DEBUGGER_PANE_BEGIN; i < DEBUGGER_PANE_END; i++ ) {
      
    /*
      GtkCheckMenuItem *check_item;
  
      check_item = get_pane_menu_item( i ); if( !check_item ) break;
    */
      /* FIXME: set the menu checkbox for pane( i ) */
    }

  } else {
    SetActiveWindow( fuse_hDBGWnd );
  }
  return 0;
}

int
create_menu_bar( void ) /* FIXME: implement */
{
  STUB;
  return 0;
}

void
toggle_display( void ) /* FIXME: implement */
{
  STUB;
}

int
create_breakpoints()
{
  size_t i;
  
  TCHAR *breakpoint_titles[] = { "ID", "Type", "Value", "Ignore", "Life",
                                 "Condition" };
  /* set extended listview style to select full row, when an item is selected */
  DWORD lv_ext_style;
  lv_ext_style = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_BPS,
                                     LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 ); 
  lv_ext_style |= LVS_EX_FULLROWSELECT;
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_BPS,
                      LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lv_ext_style ); 

  /* create columns */
  LVCOLUMN lvc;
  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
  lvc.fmt = LVCFMT_LEFT;

  for( i = 0; i < 6; i++ ) {
    if( i != 0 )
      lvc.mask |= LVCF_SUBITEM;
    lvc.cx = _tcslen( breakpoint_titles[i] ) * 8 + 10;
    lvc.pszText = breakpoint_titles[i];
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_BPS, LVM_INSERTCOLUMN, i,
                        ( LPARAM ) &lvc );
  }
  
  return 0;
}

int
create_disassembly()
{
  size_t i;

  TCHAR *disassembly_titles[] = { TEXT( "Address" ), TEXT( "Instruction" ) };

  /* The disassembly listview itself */

  /* set extended listview style to select full row, when an item is selected */
  DWORD lv_ext_style;
  lv_ext_style = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_PC,
                                     LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 ); 
  lv_ext_style |= LVS_EX_FULLROWSELECT;
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_PC,
                      LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lv_ext_style ); 

  /* create columns */
  LVCOLUMN lvc;
  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
  lvc.fmt = LVCFMT_LEFT;

  for( i = 0; i < 2; i++ ) {
    if( i != 0 )
      lvc.mask |= LVCF_SUBITEM;
    lvc.cx = 60;
    lvc.pszText = disassembly_titles[i];
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_PC, LVM_INSERTCOLUMN, i,
                        ( LPARAM ) &lvc );
  }
  
  /* The disassembly scrollbar */
  SCROLLINFO si;
  si.cbSize = sizeof(si); 
  si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE; 
  si.nPos = 0;
  si.nMin = disassembly_min;
  si.nMax = disassembly_max;
  si.nPage = disassembly_page;
  SetScrollInfo( GetDlgItem( fuse_hDBGWnd, IDC_DBG_SB_PC ),
                 SB_CTL, &si, TRUE );
  
  return 0;
}

int
create_stack_display()
{
  size_t i;
  
  TCHAR *stack_titles[] = { "Address", "Value" };
  
  /* set extended listview style to select full row, when an item is selected */
  DWORD lv_ext_style;
  lv_ext_style = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_STACK,
                                     LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 ); 
  lv_ext_style |= LVS_EX_FULLROWSELECT;
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_STACK,
                      LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lv_ext_style ); 

  /* create columns */
  LVCOLUMN lvc;
  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
  lvc.fmt = LVCFMT_LEFT;
  lvc.cx = 45;

  for( i = 0; i < 2; i++ ) {
    if( i != 0 )
      lvc.mask |= LVCF_SUBITEM;
    lvc.pszText = stack_titles[i];
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_STACK, LVM_INSERTCOLUMN, i,
                        ( LPARAM ) &lvc );
  }
  
  return 0;
}

void
stack_click( LPNMITEMACTIVATE lpnmitem )
{
  libspectrum_word address, destination;
  int error, row;

  row = lpnmitem->iItem;
  if( row < 0 ) return;

  address = SP + ( 19 - row ) * 2;

  destination =
    readbyte_internal( address ) + readbyte_internal( address + 1 ) * 0x100;

  error = debugger_breakpoint_add_address(
    DEBUGGER_BREAKPOINT_TYPE_EXECUTE, -1, destination, 0,
    DEBUGGER_BREAKPOINT_LIFE_ONESHOT, NULL
  );
  if( error ) return;

  debugger_run();
}

int
create_events()
{
  size_t i;
  
  TCHAR *titles[] = { "Time", "Type" };
  
  /* set extended listview style to select full row, when an item is selected */
  DWORD lv_ext_style;
  lv_ext_style = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_EVENTS,
                                     LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 ); 
  lv_ext_style |= LVS_EX_FULLROWSELECT;
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_EVENTS,
                      LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lv_ext_style ); 

  /* create columns */
  LVCOLUMN lvc;
  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
  lvc.fmt = LVCFMT_LEFT;
  lvc.cx = 50;

  for( i = 0; i < 2; i++ ) {
    if( i != 0 )
      lvc.mask |= LVCF_SUBITEM;
    lvc.pszText = titles[i];
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_EVENTS, LVM_INSERTCOLUMN, i,
                        ( LPARAM ) &lvc );
  }
  
  return 0;
}

void
events_click( LPNMITEMACTIVATE lpnmitem )
{
  int got_text, error, row;
  TCHAR buffer[255];
  LVITEM li;
  libspectrum_dword tstates;

  row = lpnmitem->iItem;
  if( row < 0 ) return;
        
  li.iSubItem = 0;
  li.pszText = buffer;
  li.cchTextMax = 255;
  got_text = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_EVENTS,
                                 LVM_GETITEMTEXT, row, ( LPARAM ) &li );
  if( !got_text ) {
    ui_error( UI_ERROR_ERROR, "couldn't get text for row %d", row );
    return;
  }

  tstates = _ttoi( buffer );
  error = debugger_breakpoint_add_time(
    DEBUGGER_BREAKPOINT_TYPE_TIME, tstates, 0,
    DEBUGGER_BREAKPOINT_LIFE_ONESHOT, NULL
  );
  if( error ) return;

  debugger_run();
}

int
activate_debugger( void ) /* FIXME: implement */
{
  debugger_active = 1;

  ui_debugger_disassemble( PC );
  ui_debugger_update();

  /* FIXME: call gtk_main() requivalent here */
  return 0;
}

/* Update the debugger's display */
int
ui_debugger_update( void )
{
  size_t i;
  TCHAR buffer[1024], format_string[1024];
  TCHAR *disassembly_text[2] = { &buffer[0], &buffer[40] };
  libspectrum_word address;
  int capabilities; size_t length;
  int error;

  const char *register_name[] = { TEXT( "PC" ), TEXT( "SP" ),
				  TEXT( "AF" ), TEXT( "AF'" ),
				  TEXT( "BC" ), TEXT( "BC'" ),
				  TEXT( "DE" ), TEXT( "DE'" ),
				  TEXT( "HL" ), TEXT( "HL'" ),
				  TEXT( "IX" ), TEXT( "IY" ),
                                };

  libspectrum_word *value_ptr[] = { &PC, &SP,  &AF, &AF_,
				    &BC, &BC_, &DE, &DE_,
				    &HL, &HL_, &IX, &IY,
				  };

  if( fuse_hDBGWnd == NULL ) return 0;

  /* FIXME: verify all functions below are unicode compliant */
  for( i = 0; i < 12; i++ ) {
/* FIXME: should we implement it as one label similarily to GTK?
    _sntprintf( buffer, 5, "%3s ", register_name[i] );
    _sntprintf( &buffer[4], 76, format_16_bit(), *value_ptr[i] );
*/
    _sntprintf( buffer, 76, format_16_bit(), *value_ptr[i] );
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_REG_PC + i, 
                        WM_SETTEXT, (WPARAM) 0, (LPARAM) buffer );
  }

  _sntprintf( buffer, 76, format_8_bit(), I );
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_REG_I,
                      WM_SETTEXT, (WPARAM) 0, (LPARAM) buffer );

  _sntprintf( buffer, 80, format_8_bit(), ( R & 0x7f ) | ( R7 & 0x80 ) );
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_REG_R,
                      WM_SETTEXT, (WPARAM) 0, (LPARAM) buffer );

  _sntprintf( buffer, 80, "%5d", tstates );
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_REG_T_STATES,
                      WM_SETTEXT, (WPARAM) 0, (LPARAM) buffer );
  _sntprintf( buffer, 80, "%d", IM );
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_REG_IM,
                      WM_SETTEXT, (WPARAM) 0, (LPARAM) buffer );
  _sntprintf( buffer, 80, "%d", IFF1 );
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_REG_IFF1,
                      WM_SETTEXT, (WPARAM) 0, (LPARAM) buffer );
  _sntprintf( buffer, 80, "%d", IFF2 );
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_REG_IFF2,
                      WM_SETTEXT, (WPARAM) 0, (LPARAM) buffer );

  strcpy( buffer, "SZ5H3PNC\r\n" ); /* FIXME: \r\n doesn't do the magic here */
  for( i = 0; i < 8; i++ ) buffer[i+9] = ( F & ( 0x80 >> i ) ) ? '1' : '0';
  buffer[17] = '\0';
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_REG_SZ5H3PNC,
                      WM_SETTEXT, (WPARAM) 0, (LPARAM) buffer );

  capabilities = libspectrum_machine_capabilities( machine_current->machine );

  _stprintf( format_string, "   ULA %s", format_8_bit() );
  _sntprintf( buffer, 1024, format_string, ula_last_byte() );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_AY ) {
    _stprintf( format_string, "\n    AY %s", format_8_bit() );
    length = _tcslen( buffer );
    _sntprintf( &buffer[length], 1024-length, format_string,
	        machine_current->ay.current_register );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) {
    _stprintf( format_string, "\n128Mem %s", format_8_bit() );
    length = _tcslen( buffer );
    _sntprintf( &buffer[length], 1024-length, format_string,
	        machine_current->ram.last_byte );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY ) {
    _stprintf( format_string, "\n+3 Mem %s", format_8_bit() );
    length = _tcslen( buffer );
    _sntprintf( &buffer[length], 1024-length, format_string,
	        machine_current->ram.last_byte2 );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_VIDEO  ||
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY ||
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SE_MEMORY       ) {
    _stprintf( format_string, "\nTmxDec %s", format_8_bit() );
    length = _tcslen( buffer );
    _sntprintf( &buffer[length], 1024-length, format_string,
	        scld_last_dec.byte );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY ||
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SE_MEMORY       ) {
    _stprintf( format_string, "\nTmxHsr %s", format_8_bit() );
    length = _tcslen( buffer );
    _sntprintf( &buffer[length], 1024-length, format_string, scld_last_hsr );
  }

  if( settings_current.zxcf_active ) {
    _stprintf( format_string, "\n  ZXCF %s", format_8_bit() );
    length = _tcslen( buffer );
    _sntprintf( &buffer[length], 1024-length, format_string,
	        zxcf_last_memctl() );
  }

  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_REG_ULA,
                      WM_SETTEXT, (WPARAM) 0, (LPARAM) buffer );

  /* Update the memory map display */
  error = update_memory_map(); if( error ) return error;

  error = update_breakpoints(); if( error ) return error;

  /* Update the disassembly */
  error = update_disassembly(); if( error ) return error;

  /* And the stack display */
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_STACK,
                      LVM_DELETEALLITEMS, 0, 0 );
  
  LV_ITEM lvi;
  lvi.mask = LVIF_TEXT;

  for( i = 0, address = SP + 38; i < 20; i++, address -= 2 ) {
    
    libspectrum_word contents = readbyte_internal( address ) +
				0x100 * readbyte_internal( address + 1 );

    _sntprintf( disassembly_text[0], 40, format_16_bit(), address );
    _sntprintf( disassembly_text[1], 40, format_16_bit(), contents );

    /* add the item */
    lvi.iItem = i;
    lvi.iSubItem = 0;
    lvi.pszText = disassembly_text[0];
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_STACK, LVM_INSERTITEM, 0,
                        ( LPARAM ) &lvi );
    lvi.iSubItem = 1;
    lvi.pszText = disassembly_text[1];
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_STACK, LVM_SETITEM, 0,
                        ( LPARAM ) &lvi );
  }

  /* And the events display */
  error = update_events(); if( error ) return error;

  return 0;
}

int
update_memory_map( void )
{
  /* FIXME combine _TEXT_ and _REG_ objects, and apply monospace font */
  size_t i;
  TCHAR buffer[ 40 ];

  for( i = 0; i < 8; i++ ) {

    _sntprintf( buffer, 40, format_16_bit(), (unsigned)i * 0x2000 );
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_MAP11 + ( i * 4 ), 
                        WM_SETTEXT, ( WPARAM ) 0, ( LPARAM ) buffer );

    /* FIXME: memory_bank_name is not unicode */
    _sntprintf( buffer, 40, "%s %d", memory_bank_name( &memory_map_read[i] ),
	        memory_map_read[i].page_num );
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_MAP11 + ( i * 4 ) + 1, 
                        WM_SETTEXT, ( WPARAM ) 0, ( LPARAM ) buffer );

    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_MAP11 + ( i * 4 ) + 2, 
                        WM_SETTEXT, (WPARAM) 0,
                        ( LPARAM ) ( memory_map_read[i].writable
                        ? TEXT( "Y" ) : TEXT( "N" ) ) );

    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_MAP11 + ( i * 4 ) + 2, 
                        WM_SETTEXT, (WPARAM) 0,
                        ( LPARAM ) ( memory_map_read[i].contended
                        ? TEXT( "Y" ) : TEXT( "N" ) ) );
  }

  return 0;
}

int
update_breakpoints()
{
  /* FIXME: review this function for unicode compatibility */
  TCHAR buffer[ 1024 ],
    *breakpoint_text[6] = { &buffer[  0], &buffer[ 40], &buffer[80],
			    &buffer[120], &buffer[160], &buffer[200] };
  GSList *ptr;
  TCHAR format_string[ 1024 ], page[ 1024 ];

  LV_ITEM lvi;
  lvi.mask = LVIF_TEXT;
  int i;

  /* Create the breakpoint list */
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_BPS,
                      LVM_DELETEALLITEMS, 0, 0 );

  for( ptr = debugger_breakpoints; ptr; ptr = ptr->next ) {

    debugger_breakpoint *bp = ptr->data;

    _sntprintf( breakpoint_text[0], 40, "%lu", (unsigned long)bp->id );
    _sntprintf( breakpoint_text[1], 40, "%s",
	       debugger_breakpoint_type_text[ bp->type ] );

    switch( bp->type ) {

    case DEBUGGER_BREAKPOINT_TYPE_EXECUTE:
    case DEBUGGER_BREAKPOINT_TYPE_READ:
    case DEBUGGER_BREAKPOINT_TYPE_WRITE:
      if( bp->value.address.page == -1 ) {
	_sntprintf( breakpoint_text[2], 40, format_16_bit(),
		    bp->value.address.offset );
      } else {
	debugger_breakpoint_decode_page( page, 1024, bp->value.address.page );
	_sntprintf( format_string, 1024, "%%s:%s", format_16_bit() );
	_sntprintf( breakpoint_text[2], 40, format_string, page,
		    bp->value.address.offset );
      }
      break;

    case DEBUGGER_BREAKPOINT_TYPE_PORT_READ:
    case DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE:
      _stprintf( format_string, "%s:%s", format_16_bit(), format_16_bit() );
      _sntprintf( breakpoint_text[2], 40, format_string,
		  bp->value.port.mask, bp->value.port.port );
      break;

    case DEBUGGER_BREAKPOINT_TYPE_TIME:
      _sntprintf( breakpoint_text[2], 40, "%5d", bp->value.tstates );
      break;

    }

    _sntprintf( breakpoint_text[3], 40, "%lu", (unsigned long)bp->ignore );
    _sntprintf( breakpoint_text[4], 40, "%s",
	        debugger_breakpoint_life_text[ bp->life ] );
    if( bp->condition ) {
      debugger_expression_deparse( breakpoint_text[5], 80, bp->condition );
    } else {
      _tcscpy( breakpoint_text[5], "" );
    }

    /* get the count of items to insert as last element */
    lvi.iItem = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_BPS,
                                    LVM_GETITEMCOUNT, 0, 0 );

    /* append the breakpoint items */
    for( i = 0; i < 6; i++ ) {
      lvi.iSubItem = i;
      lvi.pszText = breakpoint_text[i];
      if( i == 0 )
        SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_BPS, LVM_INSERTITEM, 0,
                            ( LPARAM ) &lvi );
      else
        SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_BPS, LVM_SETITEM, 0,
                            ( LPARAM ) &lvi );
    }
  }

  return 0;
}

int
update_disassembly()
{
  size_t i, length; libspectrum_word address;
  TCHAR buffer[80];
  TCHAR *disassembly_text[2] = { &buffer[0], &buffer[40] };

  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_PC,
                      LVM_DELETEALLITEMS, 0, 0 );

  LV_ITEM lvi;
  lvi.mask = LVIF_TEXT;

  for( i = 0, address = disassembly_top; i < 20; i++ ) {
    int l;
    _sntprintf( disassembly_text[0], 40, format_16_bit(), address );
    debugger_disassemble( disassembly_text[1], 40, &length, address );

    /* pad to 16 characters (long instruction) to avoid varying width */
    l = _tcslen( disassembly_text[1] );
    while( l < 16 ) disassembly_text[1][l++] = ' ';
    disassembly_text[1][l] = 0;

    address += length;

    /* append the item */
    lvi.iItem = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_PC,
                                    LVM_GETITEMCOUNT, 0, 0 );
    lvi.iSubItem = 0;
    lvi.pszText = disassembly_text[0];
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_PC, LVM_INSERTITEM, 0,
                        ( LPARAM ) &lvi );
    lvi.iSubItem = 1;
    lvi.pszText = disassembly_text[1];
    SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_PC, LVM_SETITEM, 0,
                        ( LPARAM ) &lvi );
  }

  return 0;
}

int
update_events( void )
{
  /* clear the listview */
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_EVENTS,
                      LVM_DELETEALLITEMS, 0, 0 );

  event_foreach( add_event, NULL );

  return 0;
}

void
add_event( gpointer data, gpointer user_data GCC_UNUSED )
{
  event_t *ptr = data;

  TCHAR buffer[80];
  TCHAR *event_text[2] = { &buffer[0], &buffer[40] };

  LV_ITEM lvi;
  lvi.mask = LVIF_TEXT;

  /* Skip events which have been removed */
  if( ptr->type == EVENT_TYPE_NULL ) return;

  _sntprintf( event_text[0], 40, "%d", ptr->tstates );
  /* FIXME: event_name() is not unicode compliant */
  _tcsncpy( event_text[1], event_name( ptr->type ), 40 );

  /* append the item */
  lvi.iItem = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_EVENTS,
                                  LVM_GETITEMCOUNT, 0, 0 );
  lvi.iSubItem = 0;
  lvi.pszText = event_text[0];
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_EVENTS, LVM_INSERTITEM, 0,
                      ( LPARAM ) &lvi );
  lvi.iSubItem = 1;
  lvi.pszText = event_text[1];
  SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_LV_EVENTS, LVM_SETITEM, 0,
                      ( LPARAM ) &lvi );
}

int
deactivate_debugger( void ) /* FIXME: implement */
{
  /* FIXME: call gtk_main_quit() equivalent here */
  debugger_active = 0;
  fuse_emulation_unpause();
  return 0;
}

/* Set the disassembly to start at 'address' */
int
ui_debugger_disassemble( libspectrum_word address )
{
  SCROLLINFO si;
  si.cbSize = sizeof(si); 
  si.fMask = SIF_POS; 
  si.nPos = disassembly_top = address;
  SetScrollInfo( GetDlgItem( fuse_hDBGWnd, IDC_DBG_SB_PC ),
                 SB_CTL, &si, TRUE );
  return 0;
}

/* Called when the disassembly scrollbar is moved */
void
move_disassembly( WPARAM scroll_command )
{
  SCROLLINFO si;
  si.cbSize = sizeof(si); 
  si.fMask = SIF_POS; 
  GetScrollInfo( GetDlgItem( fuse_hDBGWnd, IDC_DBG_SB_PC ), SB_CTL, &si );

  int value = si.nPos;
  
  /* in Windows we have to read the command and scroll the scrollbar manually */
  switch( LOWORD( scroll_command ) ) {
    case SB_BOTTOM:
      value = disassembly_max;
      break;
    case SB_TOP:
      value = disassembly_min;
      break;
    case SB_LINEDOWN:
      value++;
      break;
    case SB_LINEUP:
      value--;
      break;
    case SB_PAGEUP:
      value -= disassembly_page;
      break;
    case SB_PAGEDOWN:
      value += disassembly_page;
      break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
      value = HIWORD( scroll_command );
      break;
  }
  if( value > disassembly_max ) value = disassembly_max;
  if( value < disassembly_min ) value = disassembly_min;
  /* FIXME when holding sb's down arrow at 0xffff, 0x0000 is showing */

  size_t length;

  /* disassembly_top < value < disassembly_top + 1 => 'down' button pressed
     Move the disassembly on by one instruction */
  if( value > disassembly_top && value - disassembly_top < 1 ) {

    debugger_disassemble( NULL, 0, &length, disassembly_top );
    ui_debugger_disassemble( disassembly_top + length );

  /* disassembly_top - 1 < value < disassembly_top => 'up' button pressed
     
     See notes regarding how this function works in GTK's move_disassembly
  */
  } else if( value < disassembly_top && disassembly_top - value < 1 ) {

    size_t i, longest = 1;

    for( i = 1; i <= 8; i++ ) {

      debugger_disassemble( NULL, 0, &length, disassembly_top - i );
      if( length == i ) longest = i;

    }

    ui_debugger_disassemble( disassembly_top - longest );

  /* Anything else, just set disassembly_top to that value */
  } else {

    ui_debugger_disassemble( value );

  }

  /* And update the disassembly if the debugger is active */
  if( debugger_active ) update_disassembly();
}

/* Evaluate the command currently in the entry box */
void
evaluate_command()
{
  TCHAR *buffer;
  int buffer_size; 

  /* poll the size of the value in Evaluate text box first */
  buffer_size = SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_ED_EVAL, WM_GETTEXTLENGTH,
                                   (WPARAM) 0, (LPARAM) 0 );
  buffer = malloc( ( buffer_size + 1 ) * sizeof( TCHAR ) );
  if( buffer == NULL ) {
    ui_error( UI_ERROR_ERROR, "Out of memory in %s.", __func__ );
    return;
  }

  /* get the value in Evaluate text box first */
  if( SendDlgItemMessage( fuse_hDBGWnd, IDC_DBG_ED_EVAL, WM_GETTEXT,
                          (WPARAM) ( buffer_size + 1 ),
                          (LPARAM) buffer ) != buffer_size ) {
    ui_error( UI_ERROR_ERROR,
              "Couldn't get the content of the Evaluate text box" );
    return;
  }

  /* FIXME: need to convert from TCHAR to char to comply with unicode */
  debugger_command_evaluate( buffer );

  free( buffer );
}

void
win32ui_debugger_done_step()
{
  debugger_step();
}

void
win32ui_debugger_done_continue()
{
  debugger_run();
}

void
win32ui_debugger_break()
{
  debugger_mode = DEBUGGER_MODE_HALTED;

  EnableWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_BTN_CONT ), TRUE);
  EnableWindow( GetDlgItem( fuse_hDBGWnd, IDC_DBG_BTN_BREAK ), FALSE );
}

void
win32ui_debugger_done_close() /* FIXME: implement */
{
  delete_dialog();
  
  win32ui_debugger_done_continue();
}

INT_PTR CALLBACK
win32ui_debugger_proc( HWND hWnd, UINT msg,
                       WPARAM wParam, LPARAM lParam )
{
/* FIXME: for whatever reason fuse.exe stays in memory after it was closed
          if debugger was used */
  switch( msg ) {
    case WM_COMMAND:
      switch( LOWORD( wParam ) ) {
        case IDC_DBG_BTN_CLOSE:
          win32ui_debugger_done_close();
          return TRUE;
        case IDC_DBG_BTN_CONT:
          win32ui_debugger_done_continue();
          return TRUE;
        case IDC_DBG_BTN_STEP:
          win32ui_debugger_done_step();
          return TRUE;
        case IDC_DBG_BTN_EVAL:
          evaluate_command();
          return TRUE;
      }
      return FALSE;
    case WM_CLOSE:
      delete_dialog();
      return TRUE;
    case WM_NOTIFY:
      switch ( ( ( LPNMHDR ) lParam )->code ) {

        case NM_DBLCLK:
          switch( ( ( LPNMHDR ) lParam)->idFrom ) {

            case IDC_DBG_LV_EVENTS:
              events_click( ( LPNMITEMACTIVATE ) lParam );
              return TRUE;
            case IDC_DBG_LV_STACK:
              stack_click( ( LPNMITEMACTIVATE ) lParam );
              return TRUE;
          }
      }
      return FALSE;
    case WM_VSCROLL:
      if( ( HWND ) lParam != NULL ) {
        move_disassembly( wParam );
        return 0;
      }
      return FALSE;
  }
  return FALSE;
}

void
delete_dialog() /* FIXME: implement */
{
  DestroyWindow( fuse_hDBGWnd );
  
  fuse_hDBGWnd = NULL;
}


