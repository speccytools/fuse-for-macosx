/* PokeFinderController.h: Routines for dealing with the Poke Finder Panel
   Copyright (c) 2003 Fredrick Meunier

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
#import <AppKit/AppKit.h>


@interface PokeFinderController : NSWindowController
{
  IBOutlet NSFormCell *searchFor;
  IBOutlet NSFormCell *locations;
  IBOutlet NSTableView *matchList;
  NSMutableArray *tableContents;
}
- (IBAction)apply:(id)sender;
- (IBAction)search:(id)sender;
- (IBAction)incremented:(id)sender;
- (IBAction)decremented:(id)sender;
- (IBAction)reset:(id)sender;
- (void)showWindow:(id)sender;
- (int)numberOfRowsInTableView:(NSTableView *)table;
- (id)tableView:(NSTableView *)table objectValueForTableColumn:(NSTableColumn *)col row:(int)row;
- (void)update_pokefinder;
- (void)handleWillClose:(NSNotification *)note;
- (void)matchListDoubleAction:(id)sender;
@end
