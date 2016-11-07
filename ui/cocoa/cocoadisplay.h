/* cocoadisplay.h: Routines for dealing with the Cocoa display
   Copyright (c) 2000-2003 Philip Kendall, Fredrick Meunier

   $Id$

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

#ifndef FUSE_COCOADISPLAY_H
#define FUSE_COCOADISPLAY_H

#import <Foundation/NSLock.h>

#include "ui/ui.h"
#include "dirty.h"

typedef struct Cocoa_Texture {
  void *pixels;
  PIG_dirtytable *dirty;
  int full_height;
  int full_width;
  int image_height;
  int image_width;
  int image_xoffset;
  int image_yoffset;
  int pitch;
} Cocoa_Texture;

/* Screen texture */
extern Cocoa_Texture* screen;

extern NSLock *buffered_screen_lock;
extern Cocoa_Texture buffered_screen;

void copy_area( Cocoa_Texture *dest_screen, Cocoa_Texture *src_screen, PIG_rect *r );

#endif			/* #ifndef FUSE_COCOADISPLAY_H */
