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

/* The current breakpoint */
size_t debugger_breakpoint;

/* Used to flag 'no breakpoint set' */
const size_t DEBUGGER_BREAKPOINT_UNSET = -1;

int
debugger_init( void )
{
  return debugger_reset();
}

int
debugger_reset( void )
{
  debugger_mode = DEBUGGER_MODE_INACTIVE;
  debugger_breakpoint = DEBUGGER_BREAKPOINT_UNSET;
  return 0;
}

int
debugger_end( void )
{
  return 0;
}

/* Check whether the debugger should become active at this point */
int
debugger_check( void )
{
  switch( debugger_mode ) {

  case DEBUGGER_MODE_INACTIVE: return 0;

  case DEBUGGER_MODE_ACTIVE:
    if( PC == debugger_breakpoint ) {
      debugger_mode = DEBUGGER_MODE_HALTED;
      return 1;
    }

  case DEBUGGER_MODE_STEP:	/* Stop after this instruction */
    debugger_mode = DEBUGGER_MODE_HALTED;
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

