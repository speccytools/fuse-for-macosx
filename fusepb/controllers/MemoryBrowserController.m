/* MemoryBrowserController.m: Routines for dealing with the Memory Browser Panel
   Copyright (c) 2004 Fredrick Meunier

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

#import "MemoryBrowserController.h"

#import "DisplayOpenGLView.h"

#include "memory.h"

@implementation MemoryBrowserController

id notificationObserver;

- (id)init
{
  tableContents = nil;
  self = [super initWithWindowNibName:@"MemoryBrowser"];

  [self setWindowFrameAutosaveName:@"MemoryBrowserWindow"];

  return self;
}

- (void)dealloc
{
  NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
  [nc removeObserver:notificationObserver];

  [super dealloc];
}

- (void)showWindow:(id)sender
{
  NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
  NSOperationQueue *mainQueue = [NSOperationQueue mainQueue];
  size_t i, j;
  NSString *address;
  NSMutableString *hex, *data;

  notificationObserver =
    [nc addObserverForName:@"NSWindowWillCloseNotification"
                    object:[self window]
                     queue:mainQueue
                usingBlock:^(NSNotification *note) {
                  [nc removeObserver:notificationObserver];
                
                  [NSApp stopModal];
                
                  [tableContents removeAllObjects];
                
                  [tableContents release];
                
                  tableContents = nil;
                
                  [memoryBrowser reloadData];
                
                  [[DisplayOpenGLView instance] unpause];
                }];
  
  [[DisplayOpenGLView instance] pause];

  tableContents = [NSMutableArray arrayWithCapacity:0xfff];

  [tableContents retain];

  for( i = 0; i<= 0xfff; i++ ) {
    address = [NSString stringWithFormat:@"%04lX", i * 0x10];

    hex = [NSMutableString stringWithCapacity:64];
    data = [NSMutableString stringWithCapacity:20];

    for( j = 0; j < 0x10; j++ ) {
      libspectrum_byte b = readbyte_internal( (i * 0x10) + j );

      [hex appendFormat:@"%02X ", b ];
      [data appendFormat:@"%c", ( b >= 32 && b < 127 ) ? b : '.' ];
    }

    [tableContents addObject:
      @{@"address": address, @"data": data, @"hex": hex}
    ];
  }

  [super showWindow:sender];

  [memoryBrowser reloadData];
  
  [NSApp runModalForWindow:[self window]];
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
	
  NSParameterAssert(row >= 0 && row < [tableContents count]);
  record = [tableContents objectAtIndex:row];
  value = [record valueForKey:[col identifier]];
  return value;
}

- (BOOL)tableView:(NSTableView *)aTableView shouldSelectRow:(int)rowIndex
{
  return NO;
}

@end
