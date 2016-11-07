/* SaveBinaryController.m: Routines for dealing with the Save Binary Panel
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

*/

#include <config.h>

#import "SaveBinaryController.h"
#import "DisplayOpenGLView.h"
#import "NumberFormatter.h"

#include "memory.h"
#include "spectrum.h"
#include <libspectrum.h>
#include "ui/ui.h"
#include "utils.h"

@implementation SaveBinaryController

- (id)init
{
  self = [super initWithWindowNibName:@"SaveBinary"];

  [self setWindowFrameAutosaveName:@"SaveBinaryWindow"];

  return self;
}

- (void)awakeFromNib
{
  NumberFormatter *startFormatter = [[NumberFormatter alloc] init];
  NumberFormatter *lengthFormatter = [[NumberFormatter alloc] init];
  
  [startFormatter setMinimum:[NSDecimalNumber zero]];
  [startFormatter setFormat:@"0"];
  [start setFormatter:startFormatter];
  
  [lengthFormatter setMinimum:[NSDecimalNumber one]];
  [lengthFormatter setFormat:@"0"];
  [length setFormatter:lengthFormatter];
  
  // The formatter is retained by the text field
  [startFormatter release];
  [lengthFormatter release];
}

- (IBAction)apply:(id)sender
{
  libspectrum_word s, l; size_t i;
  libspectrum_byte *buffer;

  int error;

  s = [start intValue];
  l = [length intValue];
  
  buffer = malloc( l * sizeof( libspectrum_byte ) );
  if( !buffer ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return;
  }

  for( i = 0; i < l; i++ )
    buffer[ i ] = readbyte( s + i );

  error = utils_write_file( [[file stringValue] UTF8String], buffer, l );
  if( error ) { free( buffer ); return; }

  free( buffer );
  
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

  [file setStringValue:@""];
  [start setStringValue:@""];
  [length setStringValue:@""];
  
  [apply setEnabled:NO];

  [NSApp runModalForWindow:[self window]];
}

- (IBAction)chooseFile:(id)sender
{
  int result;
  NSSavePanel *sPanel = [NSSavePanel savePanel];

  [sPanel setAllowedFileTypes:nil];

  result = [sPanel runModal];
  if (result == NSOKButton) {
    char buffer[PATH_MAX+1];
    NSString *sFile = [[sPanel URL] path];

    [sFile getFileSystemRepresentation:buffer maxLength:PATH_MAX];
    
    [file setStringValue:sFile];
  }
}

- (void)controlTextDidChange:(NSNotification *)notification
{
  if (([[file stringValue] length] == 0) ||
      ([[start stringValue] length] == 0) ||
      ([[length stringValue] length] == 0)) {
    [apply setEnabled:NO];
    return;
  }
    
  if (([start intValue] < 0 || [start intValue] > 65535) ||
       ([length intValue] < 1 || [length intValue] > 65536)) {
    [apply setEnabled:NO];
    return;
  }

  [apply setEnabled:YES];
}

@end
