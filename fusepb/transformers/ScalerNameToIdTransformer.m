/* ScalerNameToIdTransformer.m: A transformer that converts between scaler names
                                and ids
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
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#import "ScalerNameToIdTransformer.h"

#include "ui/scaler/scaler.h"

@implementation ScalerNameToIdTransformer

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
  const char *scalerInputValue = NULL;

  if (value == nil) return nil;

  if ([value respondsToSelector: @selector(UTF8String)]) {
    // handles NSString
    scalerInputValue = [value UTF8String]; 
  } else if ([value respondsToSelector: @selector(stringValue)]) {
    // handles NSCell and NSNumber
    scalerInputValue = [[value stringValue] UTF8String]; 
  } else {
    [NSException raise: NSInternalInconsistencyException
                format: @"Value (%@) does not respond to either -stringValue or -UTF8String.",
                [value class]];
  }

  return [NSString stringWithFormat:@"%d", scaler_get_type(scalerInputValue)];
}

- (id)reverseTransformedValue:(id)value
{
  int scalerInputValue = 0;

  if (value == nil) return nil;

  if ([value respondsToSelector: @selector(intValue)]) {
    // handles NSString and NSNumber
    scalerInputValue = [value intValue]; 
  } else {
    [NSException raise: NSInternalInconsistencyException
                format: @"Value (%@) does not respond to intValue.",
                [value class]];
  }

  return @(scaler_id(scalerInputValue));
}

@end
