/* commandy.y: Parse a debugger command
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

%{

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include "debugger.h"
#include "debugger_internals.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE

#define YYSTYPE int

int yylex( void );
void yyerror( char *s );

%}

%token BREAK
%token CLEAR
%token CONTINUE
%token NEXT
%token SHOW
%token STEP

%token NUMBER

%token ERROR

%%

input:   /* empty */
       | command
;

command:   BREAK    { debugger_breakpoint_add( PC,
		        DEBUGGER_BREAKPOINT_TYPE_PERMANENT ); }
         | BREAK CLEAR { debugger_breakpoint_remove_all(); }
         | BREAK CLEAR NUMBER { debugger_breakpoint_remove( $3 ); }
	 | BREAK NUMBER { debugger_breakpoint_add( $2,
			    DEBUGGER_BREAKPOINT_TYPE_PERMANENT ); }
	 | BREAK SHOW { debugger_breakpoint_show(); }
	 | CONTINUE { debugger_run(); }
	 | NEXT	    { debugger_next(); }
	 | STEP	    { debugger_step(); }
;

%%
