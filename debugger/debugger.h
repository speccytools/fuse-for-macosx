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

#ifndef FUSE_DEBUGGER_H
#define FUSE_DEBUGGER_H

#include <stdlib.h>

#ifndef FUSE_TYPES_H
#include "types.h"
#endif

/* The current state of the debugger */
enum debugger_mode_t
{
  DEBUGGER_MODE_INACTIVE,	/* No breakpoint set */

  DEBUGGER_MODE_ACTIVE,		/* Breakpoint set, but emulator running */
  DEBUGGER_MODE_STEP,		/* Execute one instruction and then enter
				   DEBUGGER_MODE_HALTED state */
  DEBUGGER_MODE_HALTED,		/* Execution not happening */
};

extern enum debugger_mode_t debugger_mode;

extern size_t debugger_breakpoint;
extern const size_t DEBUGGER_BREAKPOINT_UNSET;

int debugger_init( void );
int debugger_reset( void );

int debugger_end( void );

int debugger_check( void );	/* See if the debugger should become active */
int debugger_trap( void );	/* Activate the debugger */

int debugger_step( void );	/* Single step */
int debugger_next( void );	/* Go to next instruction, ignoring CALL etc */
int debugger_run( void ); /* Set debugger_mode so that emulation will occur */

/* Disassemble the instruction at 'address', returning its length in
   '*length' */
void debugger_disassemble( char *buffer, size_t buflen, size_t *length,
			   WORD address );

/* Parse a debugger command */
int debugger_command_parse( const char *command );

#endif				/* #ifndef FUSE_DEBUGGER_H */
