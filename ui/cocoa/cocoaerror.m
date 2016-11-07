/* cocoaerror.m: handle errors for Mac OS X Cocoa UI
   Copyright (c) 2002 Philip Kendall
   Copyright (c) 2002-2004 Fredrick Meunier

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

#import <Foundation/NSString.h>
#import <AppKit/NSPanel.h>

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

#import "FuseController.h"

#include "fuse.h"
#include "ui/ui.h"
#include "settings.h"

static int
aqua_verror( ui_error_level severity, const char *message )
{
  NSString *alertString = @(message);

  switch( severity ) {
  case UI_ERROR_INFO:
    [[FuseController singleton]
              performSelectorOnMainThread:@selector(showAlertPanel:)
              withObject:alertString
              waitUntilDone:NO];
    break;
  case UI_ERROR_ERROR:   
  default:
    [[FuseController singleton]
              performSelectorOnMainThread:@selector(showCriticalAlertPanel:)
              withObject:alertString
              waitUntilDone:NO];
    break;
  }

  return 0;
}

int
ui_error_specific( ui_error_level severity, const char *message )
{
  if ( !settings_current.full_screen ) {
    return aqua_verror( severity, message );
  }
  return 0;
}
