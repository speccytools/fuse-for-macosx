/* expression.c: A numeric expression
   Copyright (c) 2003 Philip Kendall

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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>

#include "debugger_internals.h"
#include "fuse.h"
#include "ui/ui.h"

typedef enum expression_type {

  DEBUGGER_EXPRESSION_TYPE_INTEGER,
  DEBUGGER_EXPRESSION_TYPE_REGISTER,
  DEBUGGER_EXPRESSION_TYPE_UNARYOP,
  DEBUGGER_EXPRESSION_TYPE_BINARYOP,

} expression_type;

enum precedence {

  /* Lowest precedence */
  PRECEDENCE_EQUALITY,
  PRECEDENCE_COMPARISION,
  PRECEDENCE_ADDITION,
  PRECEDENCE_MULTIPLICATION,
  PRECEDENCE_UNARY_MINUS,
  PRECEDENCE_NOT,

  PRECEDENCE_ATOMIC,
  /* Highest precedence */

};

struct unaryop_type {

  int operation;
  debugger_expression *op;

};

struct binaryop_type {

  int operation;
  debugger_expression *op1, *op2;

};

struct debugger_expression {

  expression_type type;
  enum precedence lowest_precedence;

  union {
    int integer;
    int reg;
    struct unaryop_type unaryop;
    struct binaryop_type binaryop;
  } types;

};

static int evaluate_unaryop( struct unaryop_type *unaryop );
static int evaluate_binaryop( struct binaryop_type *binary );

static int deparse_unaryop( char *buffer, size_t length,
			    const struct unaryop_type *unaryop );
static int deparse_binaryop( char *buffer, size_t length,
			     const struct binaryop_type *binaryop );

static enum precedence
unaryop_precedence( int operation )
{
  switch( operation ) {

  case '!': return PRECEDENCE_NOT;
  case '-': return PRECEDENCE_UNARY_MINUS;

  default:
    ui_error( UI_ERROR_ERROR, "unknown unary operator %d", operation );
    fuse_abort();
  }

  return 0;			/* Keep gcc happy */
}

static enum precedence
binaryop_precedence( int operation )
{
  switch( operation ) {

  case    '+': case    '-': return PRECEDENCE_ADDITION;
  case    '*': case    '/': return PRECEDENCE_MULTIPLICATION;
  case 0x225f: case 0x2260: return PRECEDENCE_EQUALITY;
  case    '<': case    '>':
  case 0x2264: case 0x2265: return PRECEDENCE_COMPARISION;

  default:
    ui_error( UI_ERROR_ERROR, "unknown binary operator %d", operation );
    fuse_abort();
  }

  return 0;			/* Keep gcc happy */
}

debugger_expression*
debugger_expression_new_number( int number )
{
  debugger_expression *exp;

  exp = malloc( sizeof( debugger_expression ) );
  if( !exp ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return NULL;
  }

  exp->type = DEBUGGER_EXPRESSION_TYPE_INTEGER;
  exp->lowest_precedence = PRECEDENCE_ATOMIC;
  exp->types.integer = number;

  return exp;
}

debugger_expression*
debugger_expression_new_register( int which )
{
  debugger_expression *exp;

  exp = malloc( sizeof( debugger_expression ) );
  if( !exp ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return NULL;
  }

  exp->type = DEBUGGER_EXPRESSION_TYPE_REGISTER;
  exp->lowest_precedence = PRECEDENCE_ATOMIC;
  exp->types.reg = which;

  return exp;
}

debugger_expression*
debugger_expression_new_binaryop( int operation, debugger_expression *operand1,
				  debugger_expression *operand2 )
{
  debugger_expression *exp;

  exp = malloc( sizeof( debugger_expression ) );
  if( !exp ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return NULL;
  }

  exp->type = DEBUGGER_EXPRESSION_TYPE_BINARYOP;

  exp->lowest_precedence = binaryop_precedence( operation );

  if( operand1->lowest_precedence < exp->lowest_precedence )
    exp->lowest_precedence = operand1->lowest_precedence;

  if( operand2->lowest_precedence < exp->lowest_precedence )
    exp->lowest_precedence = operand2->lowest_precedence;

  exp->types.binaryop.operation = operation;
  exp->types.binaryop.op1 = operand1;
  exp->types.binaryop.op2 = operand2;

  return exp;
}


debugger_expression*
debugger_expression_new_unaryop( int operation, debugger_expression *operand )
{
  debugger_expression *exp;

  exp = malloc( sizeof( debugger_expression ) );
  if( !exp ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return NULL;
  }

  exp->type = DEBUGGER_EXPRESSION_TYPE_UNARYOP;

  exp->lowest_precedence = unaryop_precedence( operation );

  if( operand->lowest_precedence < exp->lowest_precedence )
    exp->lowest_precedence = operand->lowest_precedence;

  exp->types.unaryop.operation = operation;
  exp->types.unaryop.op = operand;

  return exp;
}


void
debugger_expression_delete( debugger_expression *exp )
{
  switch( exp->type ) {
    
  case DEBUGGER_EXPRESSION_TYPE_INTEGER:
  case DEBUGGER_EXPRESSION_TYPE_REGISTER:
    break;

  case DEBUGGER_EXPRESSION_TYPE_UNARYOP:
    debugger_expression_delete( exp->types.unaryop.op );
    break;

  case DEBUGGER_EXPRESSION_TYPE_BINARYOP:
    debugger_expression_delete( exp->types.binaryop.op1 );
    debugger_expression_delete( exp->types.binaryop.op2 );
    break;

  }
    
  free( exp );
}

int
debugger_expression_evaluate( debugger_expression *exp )
{
  switch( exp->type ) {

  case DEBUGGER_EXPRESSION_TYPE_INTEGER:
    return exp->types.integer;

  case DEBUGGER_EXPRESSION_TYPE_REGISTER:
    return debugger_register_get( exp->types.reg );

  case DEBUGGER_EXPRESSION_TYPE_UNARYOP:
    return evaluate_unaryop( &( exp->types.unaryop ) );

  case DEBUGGER_EXPRESSION_TYPE_BINARYOP:
    return evaluate_binaryop( &( exp->types.binaryop ) );

  }

  ui_error( UI_ERROR_ERROR, "unknown expression type %d", exp->type );
  fuse_abort();

  return 0;			/* Keep gcc happy */
}

static int
evaluate_unaryop( struct unaryop_type *unary )
{
  switch( unary->operation ) {

  case '!': return !debugger_expression_evaluate( unary->op );
  case '-': return -debugger_expression_evaluate( unary->op );

  }

  ui_error( UI_ERROR_ERROR, "unknown unary operator %d", unary->operation );
  fuse_abort();

  return 0;			/* Keep gcc happy */
}

static int
evaluate_binaryop( struct binaryop_type *binary )
{
  switch( binary->operation ) {

  case '+': return debugger_expression_evaluate( binary->op1 ) +
		   debugger_expression_evaluate( binary->op2 );

  case '-': return debugger_expression_evaluate( binary->op1 ) -
		   debugger_expression_evaluate( binary->op2 );

  case '*': return debugger_expression_evaluate( binary->op1 ) *
		   debugger_expression_evaluate( binary->op2 );

  case '/': return debugger_expression_evaluate( binary->op1 ) /
		   debugger_expression_evaluate( binary->op2 );

  case 0x225f: return debugger_expression_evaluate( binary->op1 ) ==
		      debugger_expression_evaluate( binary->op2 );

  case 0x2260: return debugger_expression_evaluate( binary->op1 ) !=
		      debugger_expression_evaluate( binary->op2 );

  case '>': return debugger_expression_evaluate( binary->op1 ) >
		   debugger_expression_evaluate( binary->op2 );

  case '<': return debugger_expression_evaluate( binary->op1 ) <
	           debugger_expression_evaluate( binary->op2 );

  case 0x2264: return debugger_expression_evaluate( binary->op1 ) <=
		      debugger_expression_evaluate( binary->op2 );

  case 0x2265: return debugger_expression_evaluate( binary->op1 ) >=
		      debugger_expression_evaluate( binary->op2 );

  }

  ui_error( UI_ERROR_ERROR, "unknown binary operator %d", binary->operation );
  fuse_abort();

  return 0;			/* Keep gcc happy */
}

int
debugger_expression_deparse( char *buffer, size_t length,
			     const debugger_expression *exp )
{
  switch( exp->type ) {

  case DEBUGGER_EXPRESSION_TYPE_INTEGER:
    if( debugger_output_base == 10 ) {
      snprintf( buffer, length, "%d", exp->types.integer );
    } else {
      snprintf( buffer, length, "0x%x", exp->types.integer );
    }
    return 0;

  case DEBUGGER_EXPRESSION_TYPE_REGISTER:
    snprintf( buffer, length, "%s", debugger_register_text( exp->types.reg ) );
    return 0;

  case DEBUGGER_EXPRESSION_TYPE_UNARYOP:
    return deparse_unaryop( buffer, length, &( exp->types.unaryop ) );

  case DEBUGGER_EXPRESSION_TYPE_BINARYOP:
    return deparse_binaryop( buffer, length, &( exp->types.binaryop ) );

  }

  ui_error( UI_ERROR_ERROR, "unknown expression type %d", exp->type );
  fuse_abort();

  return 0;			/* Keep gcc happy */
}
  
static int
deparse_unaryop( char *buffer, size_t length,
		 const struct unaryop_type *unaryop )
{
  char *operand_buffer, *operation_string = NULL;
  int brackets_necessary;

  int error;

  operand_buffer = malloc( length );
  if( !operand_buffer ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  error = debugger_expression_deparse( operand_buffer, length, unaryop->op );
  if( error ) { free( operand_buffer ); return error; }

  switch( unaryop->operation ) {
  case '!': operation_string = "!"; break;
  case '-': operation_string = "-"; break;

  default:
    ui_error( UI_ERROR_ERROR, "unknown unary operation %d",
	      unaryop->operation );
    fuse_abort();
  }

  brackets_necessary = ( unaryop->op->lowest_precedence           <
			 unaryop_precedence( unaryop->operation )   );
    
  snprintf( buffer, length, "%s%s%s%s", operation_string,
	    brackets_necessary ? "( " : "", operand_buffer,
	    brackets_necessary ? " )" : "" );

  free( operand_buffer );

  return 0;
}

static int
deparse_binaryop( char *buffer, size_t length,
		  const struct binaryop_type *binaryop )
{
  char *operand1_buffer, *operand2_buffer, *operation_string = NULL;
  int brackets_necessary1, brackets_necessary2;

  int error;

  operand1_buffer = malloc( 2 * length );
  if( !operand1_buffer ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }
  operand2_buffer = &operand1_buffer[ length ];

  error = debugger_expression_deparse( operand1_buffer, length,
				       binaryop->op1 );
  if( error ) { free( operand1_buffer ); return error; }

  error = debugger_expression_deparse( operand2_buffer, length,
				       binaryop->op2 );
  if( error ) { free( operand1_buffer ); return error; }

  switch( binaryop->operation ) {
  case    '+': operation_string = "+";  break;
  case    '-': operation_string = "-";  break;
  case    '*': operation_string = "*";  break;
  case    '/': operation_string = "/";  break;
  case 0x225f: operation_string = "=="; break;
  case 0x2260: operation_string = "!="; break;
  case    '<': operation_string = "<";  break;
  case    '>': operation_string = ">";  break;
  case 0x2264: operation_string = "<="; break;
  case 0x2265: operation_string = ">="; break;

  default:
    ui_error( UI_ERROR_ERROR, "unknown binary operation %d",
	      binaryop->operation );
    fuse_abort();
  }

  brackets_necessary1 = ( binaryop->op1->lowest_precedence          <
			  binaryop_precedence( binaryop->operation )   );
    
  brackets_necessary2 = ( binaryop->op2->lowest_precedence           <
			  binaryop_precedence( binaryop->operation )   );
    
  snprintf( buffer, length, "%s%s%s %s %s%s%s",
	    brackets_necessary1 ? "( " : "", operand1_buffer,
	    brackets_necessary1 ? " )" : "",
	    operation_string,
	    brackets_necessary2 ? "( " : "", operand2_buffer,
	    brackets_necessary2 ? " )" : "" );

  free( operand1_buffer );

  return 0;
}
