/* PokeFinderController.m: Routines for dealing with the Poke Finder Panel
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

#include <config.h>

#import "NumberFormatter.h"
#import "PokeFinderController.h"
#import "DisplayOpenGLView.h"

#include "fuse.h"
#include "debugger/debugger.h"
#include "pokefinder/pokefinder.h"
#include "spectrum.h"

@implementation PokeFinderController

- (id)init
{
  [super init];
  self = [super initWithWindowNibName:@"PokeFinder"];

  [self setWindowFrameAutosaveName:@"PokeFinderWindow"];

  tableContents = nil;

  return self;
}

- (void)awakeFromNib
{
  NumberFormatter *searchForFormatter = [[[NumberFormatter alloc] init] autorelease];

  [searchForFormatter setMinimum:[NSDecimalNumber zero]];
  [searchForFormatter setMaximum:[NSDecimalNumber decimalNumberWithString:@"255"]];
  [searchForFormatter setFormat:@"0"];
  [searchFor setFormatter:searchForFormatter];

  [matchList setTarget:self];
  [matchList setDoubleAction:@selector(matchListDoubleAction:)];
}

- (void)matchListDoubleAction:(id)sender
{
  int row;
  id record, value;
  unsigned long page, offset;
  
  row = [matchList clickedRow];
  
  if( row < 0 || row >= [tableContents count] ) return;
  
  record = [tableContents objectAtIndex:row];
  value  = [record valueForKey:@"page"];
  page   = [value unsignedLongValue];
  value  = [record valueForKey:@"offset_number"];
  offset = [value unsignedLongValue];
  
  (void)debugger_breakpoint_add_address( DEBUGGER_BREAKPOINT_TYPE_WRITE,
                                         memory_source_ram, page, offset, 0,
                                         DEBUGGER_BREAKPOINT_LIFE_PERMANENT, NULL);
}

- (void) dealloc
{
  NSNotificationCenter *nc;
  
  nc = [NSNotificationCenter defaultCenter];
  [nc removeObserver:self];

  [super dealloc];
}

- (void)handleWillClose:(NSNotification *)note
{
  [NSApp stopModal];

  [tableContents removeAllObjects];
  [tableContents release];
  
  tableContents = nil;
  
  [matchList reloadData];

  [[DisplayOpenGLView instance] unpause];
}

- (void)showWindow:(id)sender
{
  NSNotificationCenter *nc;

  nc = [NSNotificationCenter defaultCenter];
  [nc addObserver:self
	     selector:@selector(handleWillClose:)
		     name:@"NSWindowWillCloseNotification"
	       object:[self window]];
  
  [[DisplayOpenGLView instance] pause];

  [super showWindow:sender];

  [self update_pokefinder];
  
  [NSApp runModalForWindow:[self window]];
}

- (IBAction)search:(id)sender
{
  pokefinder_search( [searchFor intValue] );
  [self update_pokefinder];
}

- (IBAction)incremented:(id)sender
{
  pokefinder_incremented();
  [self update_pokefinder];
}

- (IBAction)decremented:(id)sender
{
  pokefinder_decremented();
  [self update_pokefinder];
}

- (IBAction)reset:(id)sender
{
  pokefinder_clear();
  [searchFor setStringValue:@""];
  [self update_pokefinder];
}

- (IBAction)apply:(id)sender
{
  [[self window] close];
}

- (int)numberOfRowsInTableView:(NSTableView *)table
{
  if( tableContents != nil )
    return [tableContents count];
    
  return 0;
}

- (id)tableView:(NSTableView *)table objectValueForTableColumn:(NSTableColumn *)col row:(int)row
{
  id record, value;

  if( row < 0 || row >= [tableContents count] ) return nil;
  record = [tableContents objectAtIndex:row];
  value = [record valueForKey:[col identifier]];
  return value;
}

- (void)update_pokefinder
{
  unsigned long page, offset;

  if( pokefinder_count && pokefinder_count <= 20 ) {
	tableContents = [NSMutableArray arrayWithCapacity:pokefinder_count];
	[tableContents retain];

    for( page = 0; page < 2 * SPECTRUM_RAM_PAGES; page++ )
	  for( offset = 0; offset < 0x2000; offset++ )
	    if( !(pokefinder_impossible[page][offset/8] & 1 << (offset & 7)) ) {
          NSNumber *p = @(page/2);
          NSString *o = [NSString stringWithFormat:@"0x%04lX", offset + 8192 * (page & 1)];
          NSNumber *on = @(offset + 8192 * (page & 1));
          [tableContents addObject: @{@"page": p, @"offset": o, @"offset_number": on}];
		}
  } else {
    [tableContents release];
    tableContents = nil;
  }

  [matchList reloadData];

  [locations setIntValue:pokefinder_count];
}

@end
