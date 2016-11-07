/* Joysticks.m: Object encapsulating libspectrum joysticks
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

#import "Joysticks.h"

#include <libspectrum.h>

#include "joystick.h"

@implementation Joystick

// Predefined global list of joysticks
+ (NSArray *)allJoysticks
{
  static NSMutableArray *joysticks;
  if (!joysticks) {
    size_t i;

    joysticks = [NSMutableArray arrayWithCapacity:JOYSTICK_TYPE_COUNT];

    for( i=0; i<JOYSTICK_TYPE_COUNT; i++ ) {
      [joysticks addObject:
          [Joystick joystickWithName:@(joystick_name[i]) andType:i]
        ];
    }
  }
  return joysticks;
}

// Retrieve joystick with given name from 'allJoysticks'
// (see NSCoding methods).
+ (Joystick *)joystickForName:(NSString *)theName
{
  NSEnumerator *joystickEnumerator = [[Joystick allJoysticks] objectEnumerator];
  Joystick *joystick;
  while ((joystick = [joystickEnumerator nextObject])) {
    if ([[joystick joystickName] isEqual:theName]) {
      return joystick;			
    }
  }
  return nil;
}

// Retrieve joystick with given type from 'allJoysticks'
// (see NSCoding methods).
+ (Joystick *)joystickForType:(int)theType
{
  NSEnumerator *joystickEnumerator = [[Joystick allJoysticks] objectEnumerator];
  Joystick *joystick;
  while ((joystick = [joystickEnumerator nextObject])) {
    if (theType == [joystick joystickType]) {
      return joystick;			
    }
  }
  return nil;
}


// Convenience constructor
+ (id)joystickWithName:(NSString *)aName andType:(int)aValue
{
  id newJoystick = [[[self alloc] init] autorelease];
  [newJoystick setJoystickName:aName];
  [newJoystick setJoystickType:aValue];

  return newJoystick;
}

/*
 NSCoding methods
 To encode, simply save 'type'; on decode, replace self with
 the existing instance from 'allJoysticks' with the same type
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
  return [[Joystick joystickForType:theType] retain];
}


// Accessors
@synthesize name;
@synthesize type;

- (void)dealloc
{
  [self setJoystickName:nil];   
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
