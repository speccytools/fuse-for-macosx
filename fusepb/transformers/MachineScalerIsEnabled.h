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

#import <Foundation/Foundation.h>

@interface MachineScalerIsEnabled : NSValueTransformer {
  int timex;
}
+ (MachineScalerIsEnabled *)machineScalerIsEnabledWithInt:(int)value;
+ (Class)transformedValueClass;
+ (BOOL)allowsReverseTransformation;
- (id)initWithInt:(int)value;
- (id)transformedValue:(id)value;

@end
