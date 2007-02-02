/* error.c: handle errors
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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#ifdef UI_AALIB			/* Use this iff we're using aalib */

#include <stdarg.h>
#include <stdio.h>

#include "fuse.h"

int
ui_error( const char *format, ... )
{
  va_list ap;
  
  va_start( ap, format );

   fprintf( stderr, "%s: ", fuse_progname );
  vfprintf( stderr, format, ap );

  va_end( ap );

  return 0;
}

#endif				/* #ifdef UI_AALIB */
