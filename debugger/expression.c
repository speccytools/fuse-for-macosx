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

  union {
    int integer;
    int reg;
    struct unaryop_type unaryop;
    struct binaryop_type binaryop;
  } types;

};

static int evaluate_unaryop( struct unaryop_type *unaryop );
static int evaluate_binaryop( struct binaryop_type *binary );

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
