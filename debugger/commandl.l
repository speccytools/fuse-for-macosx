/* commandl.l: Debugger command lexical scanner
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

#include "commandy.h"
#include "debugger.h"
#include "debugger_internals.h"

#define YY_INPUT(buf,result,max_size) \
{ \
  if( !debugger_command_input( buf, &result, max_size ) ) result = YY_NULL; \
}

%}

DIGIT	[0-9]
HEX	[0-9a-fA-F]

%option noyywrap

%%

b|br|bre|brea|break { return BREAK; }
c|co|con|cont|contin|continu|continue { return CONTINUE; }
cl|cle|clea|clear { return CLEAR; }
n|ne|nex|next { return NEXT; }
sh|sho|show { return SHOW; }
s|st|ste|step { return STEP; }

${HEX}+		{ yylval = strtol( yytext+1, NULL, 16 ); return NUMBER; }
0x{HEX}+	{ yylval = strtol( yytext+2, NULL, 16 ); return NUMBER; }
{DIGIT}+	{ yylval = atoi( yytext ); return NUMBER; }

[ \n]+		/* Swallow whitespace */

.		{ return ERROR; }
