/* MachineNameToIdTransformer.m: A transformer that converts between machine
                                 names and ids
   Copyright (c) 2005 Fredrick Meunier

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

#import "MachineNameToIdTransformer.h"

#include "machine.h"

@implementation MachineNameToIdTransformer

+ (Class)transformedValueClass
{
  return [NSString class];
}

+ (BOOL)allowsReverseTransformation
{
  return YES;   
}

- (id)transformedValue:(id)value
{
  const char *machineInputValue = NULL;

  if (value == nil) return nil;

  if ([value respondsToSelector: @selector(UTF8String)]) {
    /* handles NSString */
    machineInputValue = [value UTF8String]; 
  } else if ([value respondsToSelector: @selector(stringValue)]) {
    /* handles NSCell and NSNumber */
    machineInputValue = [[value stringValue] UTF8String]; 
  } else {
    [NSException raise: NSInternalInconsistencyException
                format: @"Value (%@) does not respond to either -stringValue or -UTF8String.",
                [value class]];
  }

  return [NSString stringWithFormat:@"%d", machine_get_index(machine_get_type(machineInputValue))];
}

- (id)reverseTransformedValue:(id)value
{
  int machineInputValue = 0;

  if (value == nil) return nil;

  if ([value respondsToSelector: @selector(intValue)]) {
    // handles NSString and NSNumber
    machineInputValue = [value intValue]; 
  } else {
    [NSException raise: NSInternalInconsistencyException
                format: @"Value (%@) does not respond to intValue.",
                [value class]];
  }

  return @(machine_types[machineInputValue]->id);
}

@end
