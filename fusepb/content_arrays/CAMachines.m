/* CAMachines.m: Object encapsulating libspectrum machines
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

#import "CAMachines.h"

#include <libspectrum.h>

#include "machine.h"

@implementation Machine

// Predefined global list of machines
+ (NSArray *)allMachines
{
    static NSMutableArray *machines;
    if (!machines) {
      size_t i;

      machines = [NSMutableArray arrayWithCapacity:machine_count];

      for( i=0; i<machine_count; i++ ) {
        [machines addObject:
            [Machine machineWithName:@(libspectrum_machine_name( machine_types[i]->machine ))
                      andType:machine_types[i]->machine]
          ];
      }
    }
    return machines;
}

// Retrieve machine with given name from 'allMachines'
// (see NSCoding methods).
+ (Machine *)machineForName:(NSString *)theName
{
  NSEnumerator *machineEnumerator = [[Machine allMachines] objectEnumerator];
  Machine *machine;
  while ((machine = [machineEnumerator nextObject])) {
    if ([[machine machineName] isEqual:theName]) {
      return machine;			
    }
  }
  return nil;
}

// Retrieve machine with given type from 'allMachines'
// (see NSCoding methods).
+ (Machine *)machineForType:(int)theType
{
  NSEnumerator *machineEnumerator = [[Machine allMachines] objectEnumerator];
  Machine *machine;
  while ((machine = [machineEnumerator nextObject])) {
    if (theType == [machine machineType]) {
      return machine;			
    }
  }
  return nil;
}


// Convenience constructor
+ (id)machineWithName:(NSString *)aName andType:(int)aValue
{
  id newMachine = [[[self alloc] init] autorelease];
  [newMachine setMachineName:aName];
  [newMachine setMachineType:aValue];

  return newMachine;
}

/*
 NSCoding methods
 To encode, simply save 'type'; on decode, replace self with
 the existing instance from 'allMachines' with the same type
 */
- (void)encodeWithCoder:(NSCoder *)encoder
{
  if ( [encoder allowsKeyedCoding] ) {
    [encoder encodeInt: type forKey:@"priority"];
  } else {
    [encoder encodeValueOfObjCType:@encode(int) at:&type];
  }
}

- (id)initWithCoder:(NSCoder *)decoder
{
  int theType = 0;
  if ( [decoder allowsKeyedCoding] ) {
    theType = [decoder decodeIntForKey:@"type"];
  } else {
    [decoder decodeValueOfObjCType:@encode(int) at:&type];
  }
  [self autorelease];
  // returning "static" object from init method -- ensure retain count maintained
  return [[Machine machineForType:theType] retain];
}


// Accessors
@synthesize name;
@synthesize type;

- (void)dealloc
{
  [self setMachineName:nil];   
  [super dealloc];
}

- (id)copyWithZone:(NSZone *)zone
{
  [self retain];
  return self;
}

- (id)valueForUndefinedKey:(NSString *)key
{
  fprintf(stderr,"Undefined key:'%s'\n",[key UTF8String]);
  return nil;
}

@end
