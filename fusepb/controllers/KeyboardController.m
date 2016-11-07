/* KeyboardController.m: Routines for dealing with the Keyboard Panel
   Copyright (c) 2002 Fredrick Meunier

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

#import "KeyboardController.h"

#include <config.h>
#include <libspectrum.h>

#include "machine.h"

@implementation KeyboardController

- (id)init
{
  self = [super initWithWindowNibName:@"Keyboard"];

  [self setWindowFrameAutosaveName:@"KeyboardWindow"];

  window_open = 0;

  return self;
}

- (void)dealloc
{
  NSNotificationCenter *nc;

  nc = [NSNotificationCenter defaultCenter];
  [nc removeObserver:self];

  [super dealloc];
}

-(void) awakeFromNib
{
  /* keep the window in the standard aspect ratio if the user resizes */
  [[self window] setContentAspectRatio:NSMakeSize(541.0,201.0)];
}

- (void)showWindow:(id)sender
{
  NSNotificationCenter *nc;

  nc = [NSNotificationCenter defaultCenter];
  [nc addObserver:self
         selector:@selector(handleWillClose:)
             name:@"NSWindowWillCloseNotification"
           object:[self window]];

  [super showWindow:sender];

  /* The TC2068 has a different keyboard layout */
  if( machine_current->machine == LIBSPECTRUM_MACHINE_TC2068 ||
    machine_current->machine == LIBSPECTRUM_MACHINE_TS2068 )
    [keyboardView setImage:[NSImage imageNamed:@"ts2068"]];
  else
    [keyboardView setImage:[NSImage imageNamed:@"48k"]];

  window_open = 1;
}

- (void)showCloseWindow:(id)sender
{
  if( window_open ) {
    [[self window] close];
  } else {
    [self showWindow:self];
  }
}

- (void)handleWillClose:(NSNotification *)note
{
    window_open = 0;
}

@end
