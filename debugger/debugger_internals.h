/* debugger_internals.h: The internals of Fuse's monitor/debugger
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

#ifndef FUSE_DEBUGGER_INTERNALS_H
#define FUSE_DEBUGGER_INTERNALS_H

#ifndef FUSE_DEBUGGER_H
#include "debugger.h"
#endif				/* #ifndef FUSE_DEBUGGER_H */

int debugger_breakpoint_add( debugger_breakpoint_type type,
			     libspectrum_word value, size_t ignore,
			     debugger_breakpoint_life life );
int debugger_breakpoint_remove( size_t id );
int debugger_breakpoint_remove_all( void );
int debugger_breakpoint_clear( libspectrum_word address );
int debugger_breakpoint_exit( void );
int debugger_breakpoint_ignore( size_t id, size_t ignore );
int debugger_breakpoint_set_condition( size_t id,
				       debugger_expression *condition );
int debugger_poke( libspectrum_word address, libspectrum_byte value );
int debugger_port_write( libspectrum_word address, libspectrum_byte value );
int debugger_register_get( int which );
void debugger_register_set( int which, int value );

/* Utility functions called by the flex scanner */

int debugger_command_input( char *buf, int *result, int max_size );
int debugger_reg16_index( char *reg );

/* Numeric expression stuff */

debugger_expression* debugger_expression_new_number( int number );
debugger_expression* debugger_expression_new_register( int which );

void debugger_expression_delete( debugger_expression* expression );

int debugger_expression_evaluate( debugger_expression* expression );

#endif				/* #ifndef FUSE_DEBUGGER_INTERNALS_H */
