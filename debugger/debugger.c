/* debugger.h: Fuse's monitor/debugger
   Copyright (c) 2002-2003 Philip Kendall

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

#include <stdio.h> 

#include "debugger.h"
#include "debugger_internals.h"
#include "spectrum.h"
#include "ui/ui.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

/* The current activity state of the debugger */
enum debugger_mode_t debugger_mode;

/* The current breakpoints */
GSList *debugger_breakpoints;

/* The next breakpoint ID to use */
static size_t next_breakpoint_id;

/* Which base should we display things in */
int debugger_output_base;

static gint find_breakpoint_by_id( gconstpointer data,
				   gconstpointer user_data );
static gint find_breakpoint_by_address( gconstpointer data,
					gconstpointer user_data );
static void free_breakpoint( gpointer data, gpointer user_data );
static void show_breakpoint( gpointer data, gpointer user_data );

/* Textual represenations of the breakpoint types and lifetimes */
char *debugger_breakpoint_type_text[] = {
  "Execute", "Read", "Write", "Port Read", "Port Write",
};

char *debugger_breakpoint_life_text[] = {
  "Permanent", "One Shot",
};

int
debugger_init( void )
{
  debugger_breakpoints = NULL;
  next_breakpoint_id = 1;
  debugger_output_base = 16;
  return debugger_reset();
}

int
debugger_reset( void )
{
  debugger_breakpoint_remove_all();
  debugger_mode = DEBUGGER_MODE_INACTIVE;

  return 0;
}

int
debugger_end( void )
{
  debugger_breakpoint_remove_all();
  return 0;
}

/* Check whether the debugger should become active at this point */
int
debugger_check( debugger_breakpoint_type type, WORD value )
{
  GSList *ptr; debugger_breakpoint *bp, *active;

  switch( debugger_mode ) {

  case DEBUGGER_MODE_INACTIVE: return 0;

  case DEBUGGER_MODE_ACTIVE:
    active = NULL;
    for( ptr = debugger_breakpoints; ptr; ptr = ptr->next ) {
      bp = ptr->data;
      if( bp->type == type && bp->value == value ) {
	if( bp->ignore ) {
	  bp->ignore--;
	} else {
	  active = bp; break;
	}
      }
    }
    if( active ) {
      if( active->life == DEBUGGER_BREAKPOINT_LIFE_ONESHOT ) {
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
  debugger_breakpoint_add( DEBUGGER_BREAKPOINT_TYPE_EXECUTE,
			   PC + length, 0, DEBUGGER_BREAKPOINT_LIFE_ONESHOT );

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
debugger_breakpoint_add( debugger_breakpoint_type type, WORD value,
			 size_t ignore, debugger_breakpoint_life life )
{
  debugger_breakpoint *bp;

  bp = malloc( sizeof( debugger_breakpoint ) );
  if( !bp ) {
    ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  bp->id = next_breakpoint_id++;
  bp->type = type; bp->value = value; bp->ignore = ignore; bp->life = life;

  debugger_breakpoints = g_slist_append( debugger_breakpoints, bp );

  if( debugger_mode == DEBUGGER_MODE_INACTIVE )
    debugger_mode = DEBUGGER_MODE_ACTIVE;

  return 0;
}

/* Remove breakpoint with the given ID */
int
debugger_breakpoint_remove( size_t id )
{
  GSList *ptr;

  ptr = g_slist_find_custom( debugger_breakpoints, &id,
			     find_breakpoint_by_id );
  if( !ptr ) {
    ui_error( UI_ERROR_ERROR, "Breakpoint %ld does not exist",
	      (unsigned long)id );
    return 1;
  }

  debugger_breakpoints = g_slist_remove( debugger_breakpoints, ptr->data );
  if( debugger_mode == DEBUGGER_MODE_ACTIVE && !debugger_breakpoints )
    debugger_mode = DEBUGGER_MODE_INACTIVE;

  free( ptr->data );

  return 0;
}

static gint
find_breakpoint_by_id( gconstpointer data, gconstpointer user_data )
{
  const debugger_breakpoint *bp = data;
  size_t id = *(size_t*)user_data;

  return bp->id - id;
}

/* Remove all breakpoints at the given address */
int
debugger_breakpoint_clear( WORD address )
{
  GSList *ptr;

  int found = 0;

  while( 1 ) {

    ptr = g_slist_find_custom( debugger_breakpoints, &address,
			       find_breakpoint_by_address );
    if( !ptr ) break;

    found++;

    debugger_breakpoints = g_slist_remove( debugger_breakpoints, ptr->data );
    if( debugger_mode == DEBUGGER_MODE_ACTIVE && !debugger_breakpoints )
      debugger_mode = DEBUGGER_MODE_INACTIVE;

    free( ptr->data );
  }

  if( !found ) {
    if( debugger_output_base == 10 ) {
      ui_error( UI_ERROR_ERROR, "No breakpoint at %d", address );
    } else {
      ui_error( UI_ERROR_ERROR, "No breakpoint at 0x%04x", address );
    }
  }

  return 0;
}

static gint
find_breakpoint_by_address( gconstpointer data, gconstpointer user_data )
{
  const debugger_breakpoint *bp = data;
  WORD address = *(WORD*)user_data;

  if( bp->type == DEBUGGER_BREAKPOINT_TYPE_EXECUTE ||
      bp->type == DEBUGGER_BREAKPOINT_TYPE_READ ||
      bp->type == DEBUGGER_BREAKPOINT_TYPE_WRITE ) {
    return( bp->value - address );
  }

  return 1;
}

/* Remove all breakpoints */
int
debugger_breakpoint_remove_all( void )
{
  g_slist_foreach( debugger_breakpoints, free_breakpoint, NULL );
  g_slist_free( debugger_breakpoints ); debugger_breakpoints = NULL;

  if( debugger_mode == DEBUGGER_MODE_ACTIVE )
    debugger_mode = DEBUGGER_MODE_INACTIVE;

  /* Restart the breakpoint numbering */
  next_breakpoint_id = 1;

  return 0;
}

static void
free_breakpoint( gpointer data, gpointer user_data GCC_UNUSED )
{
  free( data );
}

/* Show all breakpoints */
int
debugger_breakpoint_show( void )
{
  printf( "Current breakpoints:\n" );

  g_slist_foreach( debugger_breakpoints, show_breakpoint, NULL );

  return 0;
}

static void
show_breakpoint( gpointer data, gpointer user_data )
{
  debugger_breakpoint *bp = data;

  printf( "%lu: %d 0x%04x %lu %d\n", (unsigned long)bp->id, bp->type,
	  bp->value, (unsigned long)bp->ignore, bp->life );
}

/* Exit from the last CALL etc by setting a oneshot breakpoint at
   (SP) and then starting emulation */
int
debugger_breakpoint_exit( void )
{
  WORD target = readbyte_internal( SP ) + 0x100 * readbyte_internal( SP+1 );

  if( debugger_breakpoint_add( DEBUGGER_BREAKPOINT_TYPE_EXECUTE, target, 0,
			       DEBUGGER_BREAKPOINT_LIFE_ONESHOT ) )
    return 1;

  if( debugger_run() ) return 1;

  return 0;
}

/* Ignore breakpoint 'id' the next 'ignore' times it hits */
int
debugger_breakpoint_ignore( size_t id, size_t ignore )
{
  GSList *ptr; debugger_breakpoint *bp;

  ptr = g_slist_find_custom( debugger_breakpoints, &id,
			     find_breakpoint_by_id );
  if( !ptr ) {
    ui_error( UI_ERROR_ERROR, "Breakpoint %ld does not exist",
	      (unsigned long)id );
    return 1;
  }

  bp = ptr->data;

  bp->ignore = ignore;

  return 0;
}

/* Poke a value into RAM */
int
debugger_poke( WORD address, BYTE value )
{
  writebyte_internal( address, value );
  return 0;
}
