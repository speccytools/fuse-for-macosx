/* paths.c: Path-related compatibility routines
   Copyright (c) 1999-2012 Philip Kendall, Fredrick Meunier

   $Id$

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

#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFBundle.h>
#include <CoreServices/CoreServices.h>

#include "compat.h"
#include "fuse.h"
#include "ui/ui.h"

const char*
compat_get_temp_path( void )
{
  const char *dir;

  /* Use TMPDIR if specified, if not /tmp */
  dir = getenv( "TMPDIR" ); if( dir ) return dir;
  return "/tmp";
}

const char*
compat_get_home_path( void )
{
  const char *dir;
  dir = getenv( "HOME" ); if( dir ) return dir;
  return ".";
}

int
compat_is_absolute_path( const char *path )
{
  return path[0] == '/';
}

int
compat_get_next_path( path_context *ctx )
{
  CFURLRef    resource_url = NULL;
  CFBundleRef fuse_bundle  = NULL;
  int retval = 1;

  switch( (ctx->state)++ ) {

    /* Look relative to the resource bundle */
  case 0:
    fuse_bundle = CFBundleGetMainBundle();
    if( fuse_bundle == NULL ) return 0;

    resource_url = CFBundleCopyResourcesDirectoryURL( fuse_bundle );
    if( resource_url == NULL ) return 0;

    if( !CFURLGetFileSystemRepresentation( resource_url, TRUE,
                                           (unsigned char*)ctx->path,
                                           PATH_MAX ) ) {
      retval = 0;
    }

    CFRelease( resource_url );
    return retval;

    /* There is no second option */
  case 1: return 0;
  }

  ui_error( UI_ERROR_ERROR, "unknown path_context state %d", ctx->state );
  fuse_abort();
}
