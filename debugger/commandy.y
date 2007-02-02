/* commandy.y: Parse a debugger command
   Copyright (c) 2002-2004 Philip Kendall

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

%{

#include <config.h>

#include <stdio.h>		/* Needed by NetBSD yacc */
#include <stdlib.h>
#include <string.h>

#include "debugger.h"
#include "debugger_internals.h"
#include "ui/ui.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE

%}

%union {

  int token;
  int reg;

  libspectrum_dword integer;
  debugger_breakpoint_type bptype;
  debugger_breakpoint_life bplife;
  struct { int value1; libspectrum_word value2; } pair;

  debugger_expression* exp;

}

/* Tokens as returned from the Flex scanner (commandl.l) */

%token		 LOGICAL_OR	/* || */
%token		 LOGICAL_AND	/* && */
%token <token>	 COMPARISION	/* < > <= >= */
%token <token>   EQUALITY	/* == != */
%token <token>   NEGATE		/* ! ~ */
%token <token>	 TIMES_DIVIDE	/* * / */

%token		 BASE
%token		 BREAK
%token		 TBREAK
%token		 CLEAR
%token		 CONDITION
%token		 CONTINUE
%token		 DELETE
%token		 DISASSEMBLE
%token		 FINISH
%token		 IF
%token		 IGNORE
%token		 NEXT
%token		 DEBUGGER_OUT	/* Different name to avoid clashing with
				   OUT from windows.h */
%token		 PORT
%token		 READ
%token		 SET
%token		 STEP
%token		 TIME
%token		 WRITE

%token <integer> PAGE
%token <reg>	 REGISTER

%token <integer> NUMBER

%token		 ERROR

/* Derived types */

%type  <bplife>  breakpointlife
%type  <bptype>  breakpointtype
%type  <integer> pageornumber
%type  <pair>    breakpointpair
%type  <bptype>  portbreakpointtype
%type  <integer> numberorpc
%type  <integer> number

%type  <exp>     optionalcondition

%type  <exp>     expressionornull
%type  <exp>     expression;

/* Operator precedences */

/* Low precedence */

%left LOGICAL_OR
%left LOGICAL_AND
%left '|'
%left '^'
%left '&'
%left EQUALITY
%left COMPARISION
%left '+' '-'
%left TIMES_DIVIDE
%right NEGATE		/* Unary minus, unary plus, !, ~ */

/* High precedence */

%%

input:	 /* empty */
       | command
       | error
;

command:   BASE number { debugger_output_base = $2; }
	 | breakpointlife breakpointtype breakpointpair optionalcondition {
             debugger_breakpoint_add_address( $2, $3.value1, $3.value2, 0, $1,
					      $4 );
	   }
	 | breakpointlife PORT portbreakpointtype breakpointpair optionalcondition {
	     int mask; libspectrum_word port;
	     mask = $4.value1; port = $4.value2;
	     if( mask == -1 ) mask = ( port < 0x100 ? 0x00ff : 0xffff );
	     debugger_breakpoint_add_port( $3, port, mask, 0, $1, $5 );
           }
	 | breakpointlife TIME number optionalcondition {
	     debugger_breakpoint_add_time( DEBUGGER_BREAKPOINT_TYPE_TIME,
					   $3, 0, $1, $4 );
	   }
	 | CLEAR numberorpc { debugger_breakpoint_clear( $2 ); }
	 | CONDITION NUMBER expressionornull {
	     debugger_breakpoint_set_condition( $2, $3 );
           }
	 | CONTINUE { debugger_run(); }
	 | DELETE { debugger_breakpoint_remove_all(); }
	 | DELETE number { debugger_breakpoint_remove( $2 ); }
	 | DISASSEMBLE number { ui_debugger_disassemble( $2 ); }
	 | FINISH   { debugger_breakpoint_exit(); }
	 | IGNORE NUMBER number { debugger_breakpoint_ignore( $2, $3 ); }
	 | NEXT	    { debugger_next(); }
	 | DEBUGGER_OUT number NUMBER { debugger_port_write( $2, $3 ); }
	 | SET NUMBER number { debugger_poke( $2, $3 ); }
	 | SET REGISTER number { debugger_register_set( $2, $3 ); }
	 | STEP	    { debugger_step(); }
;

breakpointlife:   BREAK  { $$ = DEBUGGER_BREAKPOINT_LIFE_PERMANENT; }
		| TBREAK { $$ = DEBUGGER_BREAKPOINT_LIFE_ONESHOT; }
;

breakpointtype:   /* empty */ { $$ = DEBUGGER_BREAKPOINT_TYPE_EXECUTE; }
	        | READ        { $$ = DEBUGGER_BREAKPOINT_TYPE_READ; }
                | WRITE       { $$ = DEBUGGER_BREAKPOINT_TYPE_WRITE; }
;

breakpointpair:   numberorpc { $$.value1 = -1; $$.value2 = $1; }
		| pageornumber ':' number { $$.value1 = $1; $$.value2 = $3; }
;

pageornumber:   PAGE { $$ = $1; }
	      | number { $$ = $1; }

portbreakpointtype:   READ  { $$ = DEBUGGER_BREAKPOINT_TYPE_PORT_READ; }
		    | WRITE { $$ = DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE; }
;

optionalcondition:   /* empty */   { $$ = NULL; }
		   | IF expression { $$ = $2; }
;

numberorpc:   /* empty */ { $$ = PC; }
	    | number      { $$ = $1; }
;

expressionornull:   /* empty */ { $$ = NULL; }
	          | expression  { $$ = $1; }
;

number:   expression { $$ = debugger_expression_evaluate( $1 ); }
;

expression:   NUMBER { $$ = debugger_expression_new_number( $1 );
		       if( !$$ ) YYABORT;
		     }
	    | REGISTER { $$ = debugger_expression_new_register( $1 );
			 if( !$$ ) YYABORT;
		       }
	    | '(' expression ')' { $$ = $2; }
	    | '+' expression %prec NEGATE { $$ = $2; }
	    | '-' expression %prec NEGATE {
	        $$ = debugger_expression_new_unaryop( '-', $2 );
		if( !$$ ) YYABORT;
	      }
	    | NEGATE expression {
	        $$ = debugger_expression_new_unaryop( $1, $2 );
		if( !$$ ) YYABORT;
	      }
	    | expression '+' expression {
	        $$ = debugger_expression_new_binaryop( '+', $1, $3 );
		if( !$$ ) YYABORT;
	      }
	    | expression '-' expression {
	        $$ = debugger_expression_new_binaryop( '-', $1, $3 );
		if( !$$ ) YYABORT;
	      }
	    | expression TIMES_DIVIDE expression {
	        $$ = debugger_expression_new_binaryop( $2, $1, $3 );
		if( !$$ ) YYABORT;
	      }
	    | expression EQUALITY expression {
	        $$ = debugger_expression_new_binaryop( $2, $1, $3 );
		if( !$$ ) YYABORT;
	      }
	    | expression COMPARISION expression {
	        $$ = debugger_expression_new_binaryop( $2, $1, $3 );
		if( !$$ ) YYABORT;
	      }
	    | expression '&' expression {
	        $$ = debugger_expression_new_binaryop( '&', $1, $3 );
		if( !$$ ) YYABORT;
	      }
	    | expression '^' expression {
	        $$ = debugger_expression_new_binaryop( '^', $1, $3 );
		if( !$$ ) YYABORT;
	      }
	    | expression '|' expression {
	        $$ = debugger_expression_new_binaryop( '|', $1, $3 );
		if( !$$ ) YYABORT;
	      }
	    | expression LOGICAL_AND expression {
	        $$ = debugger_expression_new_binaryop(
                  DEBUGGER_TOKEN_LOGICAL_AND, $1, $3
                );
		if( !$$ ) YYABORT;
	      }
	    | expression LOGICAL_OR expression {
	        $$ = debugger_expression_new_binaryop(
		  DEBUGGER_TOKEN_LOGICAL_OR, $1, $3
		);
		if( !$$ ) YYABORT;
	      }
;

%%
