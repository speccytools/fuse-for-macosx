/* MachineScalerIsEnabled.m: A transformer that tells whether a Timex machine
                                is enabled
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

#import "MachineScalerIsEnabled.h"

#include "machine.h"

@implementation MachineScalerIsEnabled

+ (MachineScalerIsEnabled *)machineScalerIsEnabledWithInt:(int)value
{ 
  return [[[MachineScalerIsEnabled alloc] initWithInt:value] autorelease];
}

- (id)initWithInt:(int)value
{
  [super init];
  timex = value;
  return self;
}

+ (Class)transformedValueClass
{
  return [NSNumber class];
}

+ (BOOL)allowsReverseTransformation
{
  return NO;   
}

- (id)transformedValue:(id)value
{
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  fuse_machine_info *fmi =
    machine_get_machine_info([[defaults stringForKey:@"machine"] UTF8String]);

  if( fmi ) {
    BOOL retval = fmi->timex == timex ? YES : NO;
    return @(retval);
  }

  return @(NO);
}

@end
