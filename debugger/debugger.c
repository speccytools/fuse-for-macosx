/* debugger.h: Fuse's monitor/debugger
   Copyright (c) 2002 Darren Salt, Philip Kendall

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

   Darren: linux@youmustbejoking.demon.co.uk

   Philip: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <stdio.h> 

#include "debugger.h"
#include "ui/ui.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

/* The current activity state of the debugger */
enum debugger_mode_t debugger_mode;

/* The current breakpoints */
GSList *debugger_breakpoints;

static void free_breakpoint( gpointer data, gpointer user_data );

int
debugger_init( void )
{
  debugger_breakpoints = NULL;
  return debugger_reset();
}

int
debugger_reset( void )
{
  g_slist_foreach( debugger_breakpoints, free_breakpoint, NULL );
  debugger_mode = DEBUGGER_MODE_INACTIVE;

  return 0;
}

static void
free_breakpoint( gpointer data, gpointer user_data GCC_UNUSED )
{
  free( data );
}

int
debugger_end( void )
{
  g_slist_foreach( debugger_breakpoints, free_breakpoint, NULL );
  return 0;
}

/* Check whether the debugger should become active at this point */
int
debugger_check( void )
{
  GSList *ptr; debugger_breakpoint *bp, *active;

  switch( debugger_mode ) {

  case DEBUGGER_MODE_INACTIVE: return 0;

  case DEBUGGER_MODE_ACTIVE:
    active = NULL;
    for( ptr = debugger_breakpoints; ptr; ptr = ptr->next ) {
      bp = ptr->data;
      if( bp->pc == PC ) { active = bp; break; }
    }
    if( active ) {
      if( active->type == DEBUGGER_BREAKPOINT_TYPE_ONESHOT ) {
	debugger_breakpoints = g_slist_remove( debugger_breakpoints, active );
	free( active );
      }
      debugger_mode = DEBUGGER_MODE_HALTED;
      return 1;
    }
    return 0;

  case DEBUGGER_MODE_HALTED: return 1;

  }
  return 0;	/* Keep gcc happy */
}

/* Activate the debugger */
int
debugger_trap( void )
{
  return ui_debugger_activate();
}

/* Step one instruction */
int
debugger_step( void )
{
  debugger_mode = DEBUGGER_MODE_HALTED;
  ui_debugger_deactivate( 0 );
  return 0;
}

/* Step to the next instruction, ignoring CALLs etc */
int
debugger_next( void )
{
  size_t length;

  /* Find out how long the current instruction is */
  debugger_disassemble( NULL, 0, &length, PC );

  /* And add a breakpoint after that */
  debugger_breakpoint_add( PC + length, DEBUGGER_BREAKPOINT_TYPE_ONESHOT );

  debugger_run();

  return 0;
}

/* Set debugger_mode so that emulation will occur */
int
debugger_run( void )
{
  debugger_mode = debugger_breakpoints ?
                  DEBUGGER_MODE_ACTIVE :
                  DEBUGGER_MODE_INACTIVE;
  ui_debugger_deactivate( 1 );
  return 0;
}

/* Add a breakpoint */
int
debugger_breakpoint_add( WORD pc, debugger_breakpoint_type type )
{
  debugger_breakpoint *bp;

  bp = malloc( sizeof( debugger_breakpoint ) );
  if( !bp ) {
    ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  bp->pc = pc; bp->type = type;

  debugger_breakpoints = g_slist_append( debugger_breakpoints, bp );

  if( debugger_mode == DEBUGGER_MODE_INACTIVE )
    debugger_mode = DEBUGGER_MODE_ACTIVE;

  return 0;
}
