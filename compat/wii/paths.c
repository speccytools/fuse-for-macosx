/* paths.c: Path-related compatibility routines
   Copyright (c) 1999-2007 Philip Kendall
   Wii port Copyright (c) 2008 Bjoern Giesler

   $Id: paths.c 3253 2007-10-26 02:02:17Z zubzero $

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <stdlib.h>

#include "compat.h"

const char*
compat_get_temp_path( void )
{
  return "/tmp";
}

const char*
compat_get_home_path( void )
{
  return ".";
}

int
compat_is_absolute_path( const char *path )
{
  if(strlen(path) >= 1 && path[0] == '/')
    return 1;
  if(strlen(path) >= strlen("fat:/") &&
     strncmp(path, "fat:/", strlen("fat:/")) == 0)
    return 1;
  return 0;
}
