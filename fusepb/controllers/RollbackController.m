/* RollbackController.m: Routines for dealing with the Rollback Panel
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
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#import "RollbackController.h"

#import "DisplayOpenGLView.h"

#include "machine.h"
#include "rzx.h"
#include "snapshot.h"

static void
add_point_details( gpointer data, void *user_data );

@implementation RollbackController

- (id)init
{
  tableContents = nil;

  self = [super initWithWindowNibName:@"Rollback"];

  [self setWindowFrameAutosaveName:@"RollbackWindow"];

  return self;
}

- (void)awakeFromNib
{
  [rollbackPoints setTarget:self];
  [rollbackPoints setDoubleAction:@selector(rollbackPointsDoubleAction:)];
}

- (void)dealloc
{
  [tableContents removeAllObjects];
  
  [tableContents release];
  
  tableContents = nil;
  
  [rollbackPoints reloadData];

  [super dealloc];
}

- (IBAction)apply:(id)sender
{
  libspectrum_snap *snap;
  libspectrum_error error;

  if( [rollbackPoints numberOfSelectedRows] ) {

    error = libspectrum_rzx_rollback_to( rzx, &snap,
                                         [rollbackPoints selectedRow] );
    if( error ) [self cancel:self];

    error = rzx_start_after_rollback( snap );
    if( error ) [self cancel:self];
  }

  [self cancel:self];
}

- (IBAction)cancel:(id)sender
{
    [NSApp stopModal];
    [[self window] close];

    [[DisplayOpenGLView instance] unpause];
}

- (void)showWindow:(id)sender
{
  [[DisplayOpenGLView instance] pause];
  
  [super showWindow:sender];

  [self update];

  [okButton setEnabled:NO];

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

- (void)update
{
  GSList *rollback_points = rzx_get_rollback_list( rzx );

  if( tableContents ) [tableContents release];

  tableContents = [NSMutableArray arrayWithCapacity:10];
  
  [tableContents retain];

  g_slist_foreach( rollback_points, add_point_details, tableContents );

  [rollbackPoints reloadData];

  [rollbackPoints deselectAll:self];
}

- (void)tableViewSelectionDidChange:(NSNotification *)aNotification
{
  int row;
  
  row = [rollbackPoints selectedRow];

  if( row < 0 || row >= [tableContents count] ) {
    [okButton setEnabled:NO];
    return;
  }

  [okButton setEnabled:YES];
}

- (void)rollbackPointsDoubleAction:(id)sender
{
  int row = [rollbackPoints clickedRow];
 
  if( row < 0 || row >= [tableContents count] ) return;
  
  [self apply:self];
}

@end

static void
add_point_details( gpointer point, void *user_data )
{
  NSMutableArray *tableContents = (NSMutableArray *)user_data;
  NSString *seconds;

  seconds = [NSString stringWithFormat:@"%.2f", 
              GPOINTER_TO_INT( point ) / 50.0 ];
  [tableContents addObject:@{@"seconds": seconds}
  ];
}
