/* NumberFormatter.m: A formatter that only allows numbers to be entered
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
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <sys/types.h>
#include <regex.h>

#import "NumberFormatter.h"

@implementation NumberFormatter

- (BOOL)isPartialStringValid:(NSString *)partial
          newEditingString:(NSString **)newString
          errorDescription:(NSString **)error
{
  static regex_t num_test;
  static BOOL initialised = NO;
  BOOL retval = YES;

  if( [partial length] == 0 ){
    return YES;
  }

  if( initialised == NO ) {
    if( regcomp( &num_test, "[^[:digit:]]", REG_EXTENDED|REG_NOSUB ) )
      return NO;

    initialised = YES;
  }

  if( regexec( &num_test, [partial UTF8String], 0, (regmatch_t*)NULL, 0 ) == 0 ) {
    retval = NO;
  }

  return retval;
}

@end
