/* commandy.y: Parse a debugger command
   Copyright (c) 2002-2003 Philip Kendall

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
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

#include <stdio.h>		/* Needed by the OS X build */
#include <stdlib.h>
#include <string.h>

#include "debugger.h"
#include "debugger_internals.h"
#include "ui/ui.h"
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

%token		 BASE
%token		 BREAK
%token		 CLEAR
%token		 CONTINUE
%token		 DELETE
%token		 DISASSEMBLE
%token		 FINISH
%token		 IGNORE
%token		 NEXT
%token		 DEBUGGER_OUT	/* Different name to avoid clashing with
				   OUT from z80/z80_macros.h */
%token		 PORT
%token		 READ
%token		 SET
%token		 SHOW
%token		 STEP
%token		 WRITE

%token <integer> REGISTER

%token <integer> NUMBER

%token		 ERROR

%%

input:	 /* empty */
       | command
;

command:   BASE NUMBER { debugger_output_base = $2; }
	 | BREAK    { debugger_breakpoint_add(
			DEBUGGER_BREAKPOINT_TYPE_EXECUTE, PC, 0,
			DEBUGGER_BREAKPOINT_LIFE_PERMANENT
		      );
		    }
	 | BREAK NUMBER { debugger_breakpoint_add( 
			    DEBUGGER_BREAKPOINT_TYPE_EXECUTE, $2, 0,
			    DEBUGGER_BREAKPOINT_LIFE_PERMANENT
			  );
			}
	 | BREAK PORT READ NUMBER {
             debugger_breakpoint_add(
               DEBUGGER_BREAKPOINT_TYPE_PORT_READ, $4, 0,
               DEBUGGER_BREAKPOINT_LIFE_PERMANENT
	     );
           }
	 | BREAK PORT WRITE NUMBER {
             debugger_breakpoint_add(
	       DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE, $4, 0,
	       DEBUGGER_BREAKPOINT_LIFE_PERMANENT
	     );
	   }
	 | BREAK READ { debugger_breakpoint_add(
			  DEBUGGER_BREAKPOINT_TYPE_READ, PC, 0,
			  DEBUGGER_BREAKPOINT_LIFE_PERMANENT
			);
		      }
	 | BREAK READ NUMBER { debugger_breakpoint_add(
				 DEBUGGER_BREAKPOINT_TYPE_READ, $3, 0,
				 DEBUGGER_BREAKPOINT_LIFE_PERMANENT
			       );
			     }
	 | BREAK SHOW { debugger_breakpoint_show(); }
	 | BREAK WRITE { debugger_breakpoint_add(
			   DEBUGGER_BREAKPOINT_TYPE_WRITE, PC, 0,
			   DEBUGGER_BREAKPOINT_LIFE_PERMANENT
			 );
		       }
	 | BREAK WRITE NUMBER { debugger_breakpoint_add(
				  DEBUGGER_BREAKPOINT_TYPE_WRITE, $3, 0,
				  DEBUGGER_BREAKPOINT_LIFE_PERMANENT
				);
			      }
	 | CLEAR { debugger_breakpoint_clear( PC ); }
	 | CLEAR NUMBER { debugger_breakpoint_clear( $2 ); }
	 | CONTINUE { debugger_run(); }
	 | DELETE { debugger_breakpoint_remove_all(); }
	 | DELETE NUMBER { debugger_breakpoint_remove( $2 ); }
	 | DISASSEMBLE NUMBER { ui_debugger_disassemble( $2 ); }
	 | FINISH   { debugger_breakpoint_exit(); }
	 | IGNORE NUMBER NUMBER { debugger_breakpoint_ignore( $2, $3 ); }
	 | NEXT	    { debugger_next(); }
	 | DEBUGGER_OUT NUMBER NUMBER { debugger_port_write( $2, $3 ); }
	 | SET NUMBER NUMBER { debugger_poke( $2, $3 ); }
	 | SET REGISTER NUMBER { debugger_register_set( $2, $3 ); }
	 | STEP	    { debugger_step(); }
;

%%
