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

#ifndef FUSE_DEBUGGER_H
#define FUSE_DEBUGGER_H

#include <stdlib.h>

#ifdef HAVE_LIB_GLIB
#include <glib.h>
#endif				/* #ifdef HAVE_LIB_GLIB */

#include <libspectrum.h>

/* The current state of the debugger */
enum debugger_mode_t
{
  DEBUGGER_MODE_INACTIVE,	/* No breakpoint set */
  DEBUGGER_MODE_ACTIVE,		/* Breakpoint set, but emulator running */
  DEBUGGER_MODE_HALTED,		/* Execution not happening */
};

extern enum debugger_mode_t debugger_mode;

/* Types of breakpoint */
typedef enum debugger_breakpoint_type {
  DEBUGGER_BREAKPOINT_TYPE_EXECUTE,
  DEBUGGER_BREAKPOINT_TYPE_READ,
  DEBUGGER_BREAKPOINT_TYPE_WRITE,
  DEBUGGER_BREAKPOINT_TYPE_PORT_READ,
  DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE,
} debugger_breakpoint_type;

extern const char *debugger_breakpoint_type_text[];

/* Lifetime of a breakpoint */
typedef enum debugger_breakpoint_life {
  DEBUGGER_BREAKPOINT_LIFE_PERMANENT,
  DEBUGGER_BREAKPOINT_LIFE_ONESHOT,
} debugger_breakpoint_life;

extern const char *debugger_breakpoint_life_text[];

typedef struct debugger_breakpoint_value {

  int page;
  libspectrum_word value;

} debugger_breakpoint_value;

typedef struct debugger_expression debugger_expression;

/* The breakpoint structure */
typedef struct debugger_breakpoint {
  size_t id;
  enum debugger_breakpoint_type type;

  int page;
  libspectrum_word value;

  size_t ignore;		/* Ignore this breakpoint this many times */
  enum debugger_breakpoint_life life;
  debugger_expression *condition; /* Conditional expression to activate this
				     breakpoint */
} debugger_breakpoint;

/* The current breakpoints */
extern GSList *debugger_breakpoints;

/* Which base should we display things in */
extern int debugger_output_base;

int debugger_init( void );
int debugger_reset( void );

int debugger_end( void );

int debugger_check( debugger_breakpoint_type type, libspectrum_word value );

int debugger_trap( void );	/* Activate the debugger */

int debugger_step( void );	/* Single step */
int debugger_next( void );	/* Go to next instruction, ignoring CALL etc */
int debugger_run( void ); /* Set debugger_mode so that emulation will occur */

/* Add a new breakpoint */
int
debugger_breakpoint_add( debugger_breakpoint_type type, int page,
			 libspectrum_word value, size_t ignore,
			 debugger_breakpoint_life life,
			 debugger_expression *condition );

/* Disassemble the instruction at 'address', returning its length in
   '*length' */
void debugger_disassemble( char *buffer, size_t buflen, size_t *length,
			   libspectrum_word address );

/* Evaluate a debugger command */
int debugger_command_evaluate( const char *command );

/* Get a deparsed expression */
int debugger_expression_deparse( char *buffer, size_t length,
				 const debugger_expression *exp );

#endif				/* #ifndef FUSE_DEBUGGER_H */
