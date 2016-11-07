/* cocoaui.c: Routines for dealing with the Mac OS X user interface
   Copyright (c) 2006 Fredrick Meunier

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

#include <stdio.h>

#import "FuseController.h"
#import "Emulator.h"

#include "display.h"
#include "fuse.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "pokefinder/pokefinder.h"
#include "pokefinder/pokemem.h"
#include "settings.h"
#include "cocoaui.h"
#include "tape.h"
#include "ui/scaler/scaler.h"

#define MESSAGE_MAX_LENGTH 256

int 
ui_init( int *argc, char ***argv )
{
  ui_mouse_present = 1;

  return 0;
}

int 
ui_event( void )
{
  return 0;
}

int 
ui_end( void )
{
  return 0;
}

int
ui_statusbar_update_speed( float speed )
{
  [[FuseController singleton] performSelectorOnMainThread:@selector(setTitle:)
              withObject:[NSString stringWithFormat:@"Fuse - %3.0f%%", speed]
              waitUntilDone:NO];

  return 0;
}

int
ui_mouse_grab( int startup )
{
  if( startup ) return 0;

  /* Lock the mouse pointer at its current position */
  CGAssociateMouseAndMouseCursorPosition(false);

  [NSCursor hide];

  [[FuseController singleton] setAcceptsMouseMovedEvents:YES];

  return 1;
}

int
ui_mouse_release( int suspend GCC_UNUSED )
{
  [[FuseController singleton] setAcceptsMouseMovedEvents:NO];

  /* Unlock the mouse pointer */
  CGAssociateMouseAndMouseCursorPosition(true);

  [NSCursor unhide];

  return 0;
}

/* Called on machine selection */
int
ui_widgets_reset( void )
{
  pokefinder_clear();

  return 0;
}

ui_confirm_save_t
ui_confirm_save_specific( const char *message )
{
  return [[Emulator instance] confirmSave:@(message)];
}

int
ui_query( const char *message )
{
  return [[Emulator instance] confirm:@(message)];
}

int
ui_tape_write( void )
{
  return [[Emulator instance] tapeWrite];
}

int
ui_disk_write( int which, int saveas )
{
  return [[Emulator instance] diskWrite:which saveAs: saveas ? YES : NO];
}

int
ui_mdr_write( int which, int saveas )
{
  return [[Emulator instance] if1MdrWrite:which saveAs: saveas ? YES : NO];
}

ui_confirm_joystick_t
ui_confirm_joystick( libspectrum_joystick libspectrum_type, int inputs )
{
  return [[Emulator instance] confirmJoystick:libspectrum_type inputs:inputs];
}

void
ui_pokemem_selector( const char *filename )
{
  pokemem_read_from_file( filename );
  
  [[FuseController singleton] performSelectorOnMainThread:@selector(showPokeMemoryPane:)
                                               withObject:nil
                                            waitUntilDone:NO];
}

int
ui_get_rollback_point( GSList *points )
{
  return -1;
}
