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

   Darren: linux@youmustbejoking.demon.co.uk

   Philip: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <stdio.h>
#include <string.h>

#include "debugger.h"

static void
show_breakpoint( gpointer data, gpointer user_data )
{
  debugger_breakpoint *bp = data;

  printf( "%04x %d\n", bp->pc, bp->type );
}

int
debugger_command_parse( const char *command )
{
  unsigned breakpoint;
  int found;

  if( !strncmp( command, "b ", 2 ) ) {

    if( !strcmp( &command[2], "show" ) ) {
      g_slist_foreach( debugger_breakpoints, show_breakpoint, NULL );
    } else {
      found = sscanf( &command[2], "%x", &breakpoint );
      if( found == 1 )
	debugger_breakpoint_add( breakpoint,
				 DEBUGGER_BREAKPOINT_TYPE_PERMANENT );
    }

  } else if( !strcmp( command, "c" ) ) {
    debugger_run();
  } else if( !strcmp( command, "s" ) ) {
    debugger_step();
  } else if( !strcmp( command, "n" ) ) {
    debugger_next();
  } else {
    return 1;
  }

  return 0;
}
