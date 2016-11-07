/* TapeBrowserController.h: Routines for dealing with the Tape Browser Panel
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

*/
#import <AppKit/AppKit.h>

#include "ui/ui.h"

@interface TapeBrowserController : NSWindowController
{
  IBOutlet NSTableView *tapeBrowser;
  IBOutlet NSArrayController *tapeController;
  IBOutlet NSArrayController *infoController;
  
  BOOL initialising;
}

+ (TapeBrowserController *)singleton;

- (IBAction)apply:(id)sender;
- (void)showWindow:(id)sender;

- (void)clearContents;
- (void)addObjectToTapeContents:(NSDictionary*)info;
- (void)addObjectToInfoContents:(NSDictionary*)info;
- (void)setTapeIndex:(NSNumber*)index;
- (void)setInitialising:(NSNumber*)value;

- (void)tableViewSelectionDidChange:(NSNotification *)aNotification;

@property (retain,getter=tapeController,readonly) NSArrayController *tapeController;
@property (retain,getter=infoController,readonly) NSArrayController *infoController;
@end
