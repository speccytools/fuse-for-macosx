/* Texture.h: Implementation for the Texture class
   Copyright (c) 2007 Fredrick Meunier

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

   E-mail: fredm@spamcop.net
   Postal address: 3/66 Roslyn Gardens, Ruscutters Bay, NSW 2011, Australia

*/

#import <Cocoa/Cocoa.h>

#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <OpenGL/glu.h>

#include "ui/cocoa/cocoadisplay.h"

@interface Texture : NSObject
{
  Cocoa_Texture texture;
  GLuint textureId;
}
-(id) initWithImageFile:(NSString*)filename withXOrigin:(int)x
                     withYOrigin:(int)y;
-(void) dealloc;

-(Cocoa_Texture*) getTexture;
@property (getter=getTextureId,readonly) GLuint textureId;

-(void) uploadIconTexture;

@end
