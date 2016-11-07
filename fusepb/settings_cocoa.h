/* settings_cocoa.h: Handling configuration settings
   Copyright (c) Copyright (c) 2001-2003 Philip Kendall, Fredrick Meunier

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#ifndef FUSE_SETTINGS_COCOA_H
#define FUSE_SETTINGS_COCOA_H

#import <Foundation/NSArray.h>

#include "settings.h"

struct settings_cocoa {

  NSMutableArray *recent_snapshots;

};

#define NUM_RECENT_ITEMS 10

NSMutableArray*
settings_set_rom_array( settings_info *settings );
void
settings_get_rom_array( settings_info *settings, NSArray *machineroms );
NSInteger
machineroms_compare( id dict1, id dict2, void *context );

#endif				/* #ifndef FUSE_SETTINGS_COCOA_H */
