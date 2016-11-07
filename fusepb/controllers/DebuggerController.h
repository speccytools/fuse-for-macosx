/* DebuggerController.h: Routines for dealing with the Debugger Panel
   Copyright (c) 2002-2003 Fredrick Meunier

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

#include <config.h>

#include <libspectrum.h>
#include "event.h"

@interface DebuggerController : NSWindowController {
  IBOutlet NSButton *continueButton;
  IBOutlet NSButton *breakButton;
  IBOutlet NSTextField *PCText;
  IBOutlet NSTextField *SPText;
  IBOutlet NSTextField *AFText;
  IBOutlet NSTextField *AF_Text;
  IBOutlet NSTextField *BCText;
  IBOutlet NSTextField *BC_Text;
  IBOutlet NSTextField *DEText;
  IBOutlet NSTextField *DE_Text;
  IBOutlet NSTextField *HLText;
  IBOutlet NSTextField *HL_Text;
  IBOutlet NSTextField *IXText;
  IBOutlet NSTextField *IYText;
  IBOutlet NSTextField *IText;
  IBOutlet NSTextField *RText;
  IBOutlet NSTextField *tStates;
  IBOutlet NSTextField *IMText;
  IBOutlet NSTextField *ULAText;
  IBOutlet NSTextField *IFF1Text;
  IBOutlet NSTextField *IFF2Text;
  IBOutlet NSTextField *AYText;
  IBOutlet NSTextField *mem128;
  IBOutlet NSTextField *memPlus3;
  IBOutlet NSTextField *timexDEC;
  IBOutlet NSTextField *timexHSR;
  IBOutlet NSTextField *zxcf;
  IBOutlet NSTextField *flags;
  IBOutlet NSTextField *entry;
  IBOutlet NSTableView *dissasembly;
  NSMutableArray *dissasemblyContents;
  IBOutlet NSTableView *stack;
  NSMutableArray *stackContents;
  IBOutlet NSTableView *breakpoints;
  NSMutableArray *breakpointsContents;
  IBOutlet NSTableView *events;
  NSMutableArray *eventsContents;
  IBOutlet NSTableView *memoryMap;
  NSMutableArray *memoryMapContents;
  IBOutlet NSTextField *z80Halted;
}
+ (DebuggerController *)singleton;
- (void)awakeFromNib;
- (void)debugger_activate:(id)sender;
- (void)debugger_deactivate:(int)interruptable; 
- (void)debugger_update_breakpoints;
- (void)debugger_update:(id)sender;
- (void)add_event:(event_t *)ptr;
- (void)debugger_disassemble:(libspectrum_word)address;
- (IBAction)debugger_cmd_evaluate:(id)sender;
- (IBAction)debugger_done_step:(id)sender;
- (IBAction)debugger_done_continue:(id)sender;
- (IBAction)debugger_break:(id)sender;
- (int)numberOfRowsInTableView:(NSTableView *)table;
- (id)tableView:(NSTableView *)table objectValueForTableColumn:(NSTableColumn *)col row:(int)row;
- (void)handleWillClose:(NSNotification *)note;
- (void)eventsDoubleAction:(id)sender;
- (void)stackDoubleAction:(id)sender;
- (BOOL)tableView:(NSTableView *)aTableView shouldSelectRow:(int)rowIndex;
@end
