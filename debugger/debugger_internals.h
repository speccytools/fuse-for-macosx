/* debugger_internals.h: The internals of Fuse's monitor/debugger
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

#ifndef FUSE_DEBUGGER_INTERNALS_H
#define FUSE_DEBUGGER_INTERNALS_H

/* Add a breakpoint */
int debugger_breakpoint_add( WORD pc, enum debugger_breakpoint_type type );

/* Callback used by the flex parser to get input */
int debugger_command_input( char *buf, int *result, int max_size );

#endif				/* #ifndef FUSE_DEBUGGER_INTERNALS_H */
