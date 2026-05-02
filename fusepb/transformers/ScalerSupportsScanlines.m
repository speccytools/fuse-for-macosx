/* ScalerSupportsScanlines.m: Transformer that returns YES when the selected
                               graphics filter supports the PAL TV scanlines
                               option
   Copyright (c) 2026 The FuseX Authors

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

*/

#import "ScalerSupportsScanlines.h"

#include "ui/scaler/scaler.h"

@implementation ScalerSupportsScanlines

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
  const char *id_string;

  if( value == nil ) return @(NO);

  if( [value respondsToSelector:@selector(UTF8String)] ) {
    /* handles NSString */
    id_string = [value UTF8String];
  } else if( [value respondsToSelector:@selector(stringValue)] ) {
    /* handles NSCell and NSNumber */
    id_string = [[value stringValue] UTF8String];
  } else {
    /* unexpected type (e.g. NSNull from a corrupt defaults entry); leave
       the checkbox disabled rather than raise from a binding evaluation */
    return @(NO);
  }

  /* Only PAL TV 2x, 3x and 4x expose a scanlines option; the 1x variant
     has no alternate row to darken */
  int type = scaler_get_type( id_string );
  BOOL supports = type == SCALER_PALTV2X ||
                  type == SCALER_PALTV3X ||
                  type == SCALER_PALTV4X;
  return @(supports);
}

@end
