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

int yylex( void );
void yyerror( char *s );

%}

%union {

  int integer;

}

%token		 BREAK
%token		 CLEAR
%token		 CONTINUE
%token		 EXIT
%token		 NEXT
%token		 READ
%token		 SET
%token		 SHOW
%token		 STEP
%token		 WRITE

%token <integer> REGISTER

%token <integer> NUMBER

%token		 ERROR

%%

input:   /* empty */
       | command
;

command:   BREAK    { debugger_breakpoint_add(
			DEBUGGER_BREAKPOINT_TYPE_EXECUTE, PC,
		        DEBUGGER_BREAKPOINT_LIFE_PERMANENT
                      );
                    }
         | BREAK CLEAR { debugger_breakpoint_remove_all(); }
         | BREAK CLEAR NUMBER { debugger_breakpoint_remove( $3 ); }
	 | BREAK NUMBER { debugger_breakpoint_add( 
			    DEBUGGER_BREAKPOINT_TYPE_EXECUTE, $2,
			    DEBUGGER_BREAKPOINT_LIFE_PERMANENT
                          );
                        }
	 | BREAK READ { debugger_breakpoint_add(
			  DEBUGGER_BREAKPOINT_TYPE_READ, PC,
			  DEBUGGER_BREAKPOINT_LIFE_PERMANENT
			);
	              }
	 | BREAK READ NUMBER { debugger_breakpoint_add(
				 DEBUGGER_BREAKPOINT_TYPE_READ, $3,
				 DEBUGGER_BREAKPOINT_LIFE_PERMANENT
                               );
			     }
	 | BREAK SHOW { debugger_breakpoint_show(); }
	 | BREAK WRITE { debugger_breakpoint_add(
			   DEBUGGER_BREAKPOINT_TYPE_WRITE, PC,
			   DEBUGGER_BREAKPOINT_LIFE_PERMANENT
			 );
	               }
	 | BREAK WRITE NUMBER { debugger_breakpoint_add(
				  DEBUGGER_BREAKPOINT_TYPE_WRITE, $3,
				  DEBUGGER_BREAKPOINT_LIFE_PERMANENT
                                );
			      }
	 | CONTINUE { debugger_run(); }
         | EXIT     { debugger_breakpoint_exit(); }
	 | NEXT	    { debugger_next(); }
         | SET REGISTER NUMBER { debugger_register_set( $2, $3 ); }
	 | STEP	    { debugger_step(); }
;

%%
