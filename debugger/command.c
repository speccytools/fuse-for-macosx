/* command.c: Parse a debugger command
   Copyright (c) 2002 Philip Kendall

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

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "debugger.h"
#include "ui/ui.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

/* The last debugger command we were given to execute */
static char *command_buffer = NULL;

/* And a pointer as to how much we've parsed */
static char *command_ptr;

/* Necessary declaration */
int yyparse( void );

/* Evaluate the debugger command given in 'command' */
int
debugger_command_evaluate( const char *command )
{
  if( command_buffer ) free( command_buffer );

  command_buffer = strdup( command );
  if( !command_buffer ) {
    ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  /* Start parsing at the start of the given command */
  command_ptr = command_buffer;
    
  yyparse();

  ui_debugger_update();

  return 0;
}

/* Utility functions called from the flex scanner */

/* Called to get up to 'max_size' bytes of the command to be parsed */
int
debugger_command_input( char *buf, int *result, int max_size )
{
  size_t length = strlen( command_ptr );

  if( !length ) {
    return 0;
  } else if( length < max_size ) {
    memcpy( buf, command_ptr, length );
    *result = length; command_ptr += length;
    return 1;
  } else {
    memcpy( buf, command_ptr, max_size );
    *result = max_size; command_ptr += max_size;
    return 1;
  }
}

/* Convert a 16-bit register name to a useful index value */
int
debugger_reg16_index( char *name )
{
  int hash = 0x100 * tolower( name[0] ) + tolower( name[1] );

  switch( hash ) {
    case 0x6166:	/* AF */
    case 0x6263:	/* BC */
    case 0x6465:	/* DE */
    case 0x686c:	/* HL */
    case 0x7370:	/* SP */
    case 0x7063:	/* PC */
    case 0x6978:	/* IX */
    case 0x6979:	/* IY */
      return hash;

    default: return -1;
  }
}

/* Utility functions called by the bison parser */

/* The error callback if yyparse finds an error */
void
yyerror( char *s )
{
  ui_error( UI_ERROR_ERROR, "Invalid debugger command: %s", s );
}

/* Called to set a register or similar */
void
debugger_register_set( int which, int value )
{
  switch( which ) {

    /* 8-bit registers */
    case 'a': A = value; break;
    case 'b': B = value; break;
    case 'c': C = value; break;
    case 'd': D = value; break;
    case 'e': E = value; break;
    case 'f': F = value; break;
    case 'h': H = value; break;
    case 'l': L = value; break;

    /* 16-bit registers */
    case 0x6166: AF = value; break;
    case 0x6263: BC = value; break;
    case 0x6465: DE = value; break;
    case 0x686c: HL = value; break;
    case 0x7370: SP = value; break;
    case 0x7063: PC = value; break;
    case 0x6978: IX = value; break;
    case 0x6979: IY = value; break;

  default:
    ui_error( UI_ERROR_ERROR, "attempt to set unknown register '%d'", which );
    break;
  }
}
