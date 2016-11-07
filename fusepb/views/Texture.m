/* Texture.m: Implementation for the Texture class
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

#import "Texture.h"

@implementation Texture

-(id) initWithImageFile:(NSString*)filename withXOrigin:(int)x
                     withYOrigin:(int)y
{
  if( ( self = [super init] ) ) {
    NSString *textureName = [[NSBundle mainBundle] pathForImageResource:filename];
    if( !textureName )
      NSLog(@"in initWithImageFile no textureName for filename:%@", filename);
    NSURL *textureFile = [NSURL fileURLWithPath:textureName];

    CGImageSourceRef image_source =
      CGImageSourceCreateWithURL( (CFURLRef)textureFile, nil );

    CGImageRef image =
      CGImageSourceCreateImageAtIndex( image_source, 0, nil );
      
    CFRelease( image_source );

    texture.image_width = CGImageGetWidth( image );
    texture.image_height = CGImageGetHeight( image );

    texture.pixels = malloc( texture.image_width * texture.image_height * 4 );

    CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();

    CGContextRef context =
      CGBitmapContextCreate( texture.pixels,
                             texture.image_width,
                             texture.image_height,
                             8,
                             texture.image_width * 4,
                             color_space,
                             kCGImageAlphaPremultipliedFirst );

    CGContextDrawImage( context,
                        CGRectMake(0, 0, texture.image_width, texture.image_height),
                        image );

    CGColorSpaceRelease( color_space );

    CGImageRelease( image );

    CGContextRelease( context );

    texture.image_xoffset = x;
    texture.image_yoffset = y;

    [self uploadIconTexture];
  }

  return self;
}

-(void) dealloc
{
  glDeleteTextures(1, &textureId);
  if (texture.pixels != NULL) {
    free( texture.pixels );
    texture.pixels = NULL;
  }
  [super dealloc];
}

-(Cocoa_Texture*) getTexture
{
  return &texture;
}

@synthesize textureId;

-(void) uploadIconTexture;
{
  glGenTextures( 1, &textureId );

  /* Set memory alignment parameters for unpacking the bitmap. */
  glPixelStorei( GL_UNPACK_ROW_LENGTH, texture.image_width );
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

  /* Specify the texture's properties. */
  glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textureId );
  glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );


  /* Upload the texture bitmap. */
  glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8, texture.image_width,
                texture.image_height, 0, GL_BGRA_EXT,
#ifdef WORDS_BIGENDIAN
                GL_UNSIGNED_INT_8_8_8_8_REV,
#else                           /* #ifdef WORDS_BIGENDIAN */
                GL_UNSIGNED_INT_8_8_8_8,
#endif                          /* #ifdef WORDS_BIGENDIAN */
                texture.pixels );
}

@end
