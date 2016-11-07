/* DisplayOpenGLView.m: Implementation for the DisplayOpenGLView class
   Copyright (c) 2006-2007 Fredrick Meunier

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

#import "DisplayOpenGLView.h"
#import "Emulator.h"
#import "FuseController.h"
#import "DebuggerController.h"
#import "Texture.h"

#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <OpenGL/glu.h>

#include "fuse.h"
#include "fusepb/main.h"
#include "settings.h"
#include "ui/cocoa/cocoaui.h"
#include "ui/cocoa/dirty.h"

#define QZ_f 0x03
#define QZ_0 0x1D
#define QZ_1 0x12
#define QZ_2 0x13
#define QZ_3 0x14
#define QZ_4 0x15
#define QZ_5 0x16
#define QZ_m 0x2E

static const void *
get_byte_pointer(void *bitmap)
{
  return bitmap;
}

static CVReturn MyDisplayLinkCallback (
    CVDisplayLinkRef displayLink,
    const CVTimeStamp *inNow,
    const CVTimeStamp *inOutputTime,
    CVOptionFlags flagsIn,
    CVOptionFlags *flagsOut,
    void *displayLinkContext)
{
  CVReturn error =
        [(DisplayOpenGLView*) displayLinkContext displayFrame:inOutputTime];
  return error;
}

static int
get_offset( int window_width, int window_height,
            int image_width, int image_height, float* width_adjustment ) {
  static const float FULL_IMAGE_RATIO = 4./3.;
  static const float TOP_BORDER_HAIRCUT = 24.;
  static const float MINIMAL_HEIGHT = 240.-TOP_BORDER_HAIRCUT*2.;
  static const float MINIMAL_TOP_BOTTOM_RATIO = 320./MINIMAL_HEIGHT;
  static const float SIDE_BORDER_HAIRCUT = 31.;
  static const float MINIMAL_WIDTH = 320.-SIDE_BORDER_HAIRCUT*2.;
  static const float MINIMAL_LEFT_RIGHT_RATIO = MINIMAL_WIDTH/240.;
  float ratio = window_width/(float)window_height;
  *width_adjustment = 0.;
  if( fabs( ratio - FULL_IMAGE_RATIO ) < 0.01f ) {
    /* no change, we are already at 4:3 */
    return 0;
  }

  if( !settings_current.full_screen_panorama ) {
    // if wider then desirable then use max height and have black bars on left
    // and right borders
    if( ratio > FULL_IMAGE_RATIO ) {
      float height_scale = window_height / (float)image_height;
      float height_ratio = window_height * FULL_IMAGE_RATIO;
      *width_adjustment = (window_width - height_ratio)/(2.*height_scale);
      return 0;
    }
    // if narrower than desirable then use max width and have black bars on top
    // and bottom of frame
    float width_scale = window_width / (float)image_width;
    float width_ratio = window_width / FULL_IMAGE_RATIO;
    return (width_ratio - window_height)/(2.*width_scale);
  }
  if( ratio > MINIMAL_TOP_BOTTOM_RATIO ) {
    // Truncate the top and bottom borders as much as possible and put black
    // bars on the left and right borders to cover the remainder
    float height_scale = window_height / MINIMAL_HEIGHT;
    float height_ratio = window_height * MINIMAL_TOP_BOTTOM_RATIO;
    *width_adjustment = (window_width - height_ratio)/(2.*height_scale);
    return TOP_BORDER_HAIRCUT;
  } else if( ratio > FULL_IMAGE_RATIO ) {
    // Similar to above but need to appropriately scale the amount of border
    // truncation assuming that full width will be used
    float width_scale = window_width / (float)image_width;
    float height_pixels = window_width / FULL_IMAGE_RATIO;
    return (height_pixels - window_height) / (2.*width_scale);
  } else if( ratio < MINIMAL_LEFT_RIGHT_RATIO ) {
    // here we have maximum border truncation and black bars top and bottom
    float width_scale = window_width / MINIMAL_WIDTH;
    float width_ratio = window_width / MINIMAL_LEFT_RIGHT_RATIO;
    *width_adjustment = -SIDE_BORDER_HAIRCUT;
    return (width_ratio - window_height)/(2.*width_scale);
  }
  // here we have a little bit of truncation of the left and right borders and
  // leave the top and bottom borders alone
  float height_scale = window_height / (float)image_height;
  float width_pixels = window_height * FULL_IMAGE_RATIO;
  *width_adjustment = (window_width - width_pixels) / (2.*height_scale);
  return 0;
}

@implementation DisplayOpenGLView

static DisplayOpenGLView *instance = nil;

+(DisplayOpenGLView *) instance
{
  return instance;
}

-(IBAction) fullscreen:(id)sender
{
  /* don't want to get a callback to display the screen while we are
   * fiddling with the window to draw into!
   */
  [self displayLinkStop];

  if( settings_current.full_screen ) {
    /* we need to go back to non-full screen */
    [fullscreenWindow close];
    [windowedWindow setContentView: self];
    [windowedWindow makeKeyAndOrderFront: self];
    [windowedWindow makeFirstResponder: self];
    settings_current.full_screen = 0;
    if( ui_mouse_grabbed ) ui_mouse_grabbed = ui_mouse_release( 0 );
  } else {
    unsigned int windowStyle;
    NSRect       contentRect;

    windowedWindow = [self window];
    windowStyle    = NSBorderlessWindowMask;
    contentRect    = [[NSScreen mainScreen] frame];
    fullscreenWindow = [[NSWindow alloc] initWithContentRect:contentRect
                                         styleMask: windowStyle
                                         backing:NSBackingStoreBuffered
                                         defer: NO];
    if( fullscreenWindow != nil ) {
      settings_current.full_screen = 1;
      [fullscreenWindow setTitle: @"Fuse"];
      [fullscreenWindow setReleasedWhenClosed: YES];
      [fullscreenWindow setContentView: self];
      [fullscreenWindow makeKeyAndOrderFront:self ];
      [fullscreenWindow setLevel: NSScreenSaverWindowLevel - 1];
      [fullscreenWindow makeFirstResponder:self];
      if( !ui_mouse_grabbed ) ui_mouse_grabbed = ui_mouse_grab( 0 );
    }
  }

  [self displayLinkStart];

  [view_lock lock];
  statusbar_updated = YES;
  [view_lock unlock];
  [[FuseController singleton] releaseCmdKeys:@"f" withCode:QZ_f];
}

-(IBAction) zoom:(id)sender
{
  NSSize size;

  switch( [sender tag] ) {
  case 1: /* 320x240 */
    size.width = 320;
    size.height = 240;
    [[FuseController singleton] releaseCmdKeys:@"1" withCode:QZ_1];
    break;
  case 2: /* 640x480 */
    size.width = 640;
    size.height = 480;
    [[FuseController singleton] releaseCmdKeys:@"2" withCode:QZ_2];
    break;
  case 3: /* 960x720 */
    size.width = 960;
    size.height = 720;
    [[FuseController singleton] releaseCmdKeys:@"3" withCode:QZ_3];
    break;
  case 4: /* 1280x960 */
    size.width = 1280;
    size.height = 960;
    [[FuseController singleton] releaseCmdKeys:@"4" withCode:QZ_4];
    break;
  case 5: /* 1600x1200 */
    size.width = 1600;
    size.height = 1200;
    [[FuseController singleton] releaseCmdKeys:@"5" withCode:QZ_5];
    break;
  case 0:
  default: /* Actual size */
    size.width = screenTex[0].image_width;
    size.height = screenTex[0].image_height;
    [[FuseController singleton] releaseCmdKeys:@"0" withCode:QZ_0];
  }

  [[self window] setContentSize:size];
}

-(void)setServer:(id)anObject
{
  proxy_emulator = [anObject retain];
}

-(id) initWithFrame:(NSRect)frameRect
{
  /* Init pixel format attribs */
  NSOpenGLPixelFormatAttribute attrs[] = {
                                           NSOpenGLPFANoRecovery,
                                           NSOpenGLPFAAccelerated,
                                           NSOpenGLPFADoubleBuffer,
                                           0
                                         };

  /* Get pixel format from OpenGL */
  NSOpenGLPixelFormat* pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
  if (!pixFmt) {
    NSLog(@"No pixel format -- exiting");
    exit(1);
  }

  if ( instance ) {
    [self dealloc];
    self = instance;
  } else {
    self = [super initWithFrame:frameRect pixelFormat:pixFmt];
    instance = self;

    buffered_screen_lock = [[NSLock alloc] init];

    real_emulator = [[Emulator alloc] init];
  }

  [pixFmt release];

  [[self openGLContext] makeCurrentContext];

  // Synchronize buffer swaps with vertical refresh rate
  GLint swapInt = 1;
  [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval]; 
    
  /* Setup some basic OpenGL stuff */
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
  glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glColor4f(0.0f, 0.0f, 0.0f, 0.0f);

  greenCassette = [Texture alloc];
  redCassette = [Texture alloc];
  [self loadPicture: @"cassette" greenTex:greenCassette
                                   redTex:redCassette
                                  xOrigin:285
                                  yOrigin:220];
  greenMdr = [Texture alloc];
  redMdr = [Texture alloc];
  [self loadPicture: @"microdrive" greenTex:greenMdr
                                     redTex:redMdr
                                    xOrigin:264
                                    yOrigin:218];
  greenDisk = [Texture alloc];
  redDisk = [Texture alloc];
  [self loadPicture: @"plus3disk" greenTex:greenDisk
                                    redTex:redDisk
                                   xOrigin:243
                                   yOrigin:218];
  screenTexInitialised = NO;

  target_ratio = 4.0f/3.0f;

  NSPort *port1;
  NSPort *port2;
  NSArray *portArray;

  port1 = [NSPort port];
  port2 = [NSPort port];

  kitConnection = [[NSConnection alloc] initWithReceivePort:port1 sendPort:port2];
  [kitConnection setRootObject:self];

  [kitConnection enableMultipleThreads];

  /* Ports switched here */
  portArray = @[port2, port1];

  [NSThread detachNewThreadSelector:@selector(connectWithPorts:)
            toTarget:real_emulator withObject:portArray]; 

  currentScreenTex = 0;

  statusbar_updated = NO;

  return self;
}

-(void)dealloc
{
  if (view_lock)
    [view_lock release];
  view_lock = nil;

  if (buffered_screen_lock)
    [buffered_screen_lock release];
  buffered_screen_lock = nil;
  
  [super dealloc];
}

-(void) awakeFromNib
{
  /* keep the window in the standard aspect ratio if the user resizes */
  [[self window] setContentAspectRatio:NSMakeSize(4.0,3.0)];

  view_lock = [[NSLock alloc] init];

  CVReturn            error = kCVReturnSuccess;
  CGDirectDisplayID   displayID = CGMainDisplayID();
 
  mainViewDisplayID = displayID;

  error = CVDisplayLinkCreateWithCGDisplay( displayID, &displayLink );
  if( error ) {
    NSLog( @"DisplayLink created with error:%d", error );
    displayLink = NULL;
    return;
  }
  error = CVDisplayLinkSetOutputCallback( displayLink,
                                          MyDisplayLinkCallback, self );
  if( error ) {
    NSLog( @"Callback created with error:%d", error );
    return;
  }

  displayLinkRunning = NO;
}

- (void)windowWillClose:(NSNotification *)notification
{
  [[self window] setDelegate:nil];
  [proxy_emulator stop];
  [proxy_emulator release];
  proxy_emulator = nil;
  [real_emulator release];
  real_emulator = nil;

  [redCassette release];
  redCassette = nil;
  [greenCassette release];
  greenCassette = nil;
  [redMdr release];
  redMdr = nil;
  [greenMdr release];
  greenMdr = nil;
  [redDisk release];
  redDisk = nil;
  [greenDisk release];
  greenDisk = nil;

  [self release];
}

- (void)windowDidResignKey:(NSNotification *)notification
{
  [proxy_emulator keyboardReleaseAll];
}

-(void) loadPicture: (NSString *) name
                      greenTex:(Texture*) greenTexture
                      redTex:(Texture*) redTexture
                      xOrigin:(int) x
                      yOrigin:(int) y
{
  NSString *filename;

  filename = [NSString stringWithFormat:@"%@_green", name];

  /* Colour first image green */
  (void)[greenTexture initWithImageFile:filename withXOrigin:x
                          withYOrigin:y];

  filename = [NSString stringWithFormat:@"%@_red", name];

  /* Colour second image red */
  (void)[redTexture initWithImageFile:filename withXOrigin:x
                        withYOrigin:y];
}

-(void) setNeedsDisplayYes
{
  [super setNeedsDisplay:YES];
}

-(void) blitIcon:(Texture*)iconTexture
{
  Cocoa_Texture* texture = [iconTexture getTexture];
  GLuint textureName = [iconTexture getTextureId];

  /* Map pixel icon position to appropriate position on -1.0 to 1.0 canvas */
  float target_x1 = texture->image_xoffset * 2.0f / (float)DISPLAY_ASPECT_WIDTH
                    - 1.0f;
  float target_x2 = ( ( texture->image_xoffset + texture->image_width ) * 2.0f
                      / (float)DISPLAY_ASPECT_WIDTH ) - 1.0f;
  float target_y1 = 1.0f - texture->image_yoffset * 2.0f /
                    (float)DISPLAY_SCREEN_HEIGHT;
  float target_y2 = 1.0f - ( texture->image_yoffset + texture->image_height )
                    * 2.0f / (float)DISPLAY_SCREEN_HEIGHT;

  /* Bind and draw icon */
  glBindTexture( GL_TEXTURE_RECTANGLE_ARB, textureName );

  glBegin( GL_QUADS );

    glTexCoord2f( (float)texture->image_width, 0.0f );
    glVertex2f( target_x2, target_y1 );

    glTexCoord2f( (float)texture->image_width, (float)texture->image_height );
    glVertex2f( target_x2, target_y2 );

    glTexCoord2f( 0.0f, (float)texture->image_height );
    glVertex2f( target_x1, target_y2 );

    glTexCoord2f( 0.0f, 0.0f );
    glVertex2f( target_x1, target_y1 );

  glEnd();
}

-(void) iconOverlay
{
  switch( disk_state ) {
  case UI_STATUSBAR_STATE_ACTIVE:
    [self blitIcon:greenDisk];
    break;
  case UI_STATUSBAR_STATE_INACTIVE:
    [self blitIcon:redDisk];
    break;
  case UI_STATUSBAR_STATE_NOT_AVAILABLE:
    break;
  }

  switch( mdr_state ) {
  case UI_STATUSBAR_STATE_ACTIVE:
    [self blitIcon:greenMdr];
    break;
  case UI_STATUSBAR_STATE_INACTIVE:
    [self blitIcon:redMdr];
    break;
  case UI_STATUSBAR_STATE_NOT_AVAILABLE:
    break;
  }

  switch( tape_state ) {
  case UI_STATUSBAR_STATE_ACTIVE:
    [self blitIcon:greenCassette];
    break;
  case UI_STATUSBAR_STATE_INACTIVE:
  case UI_STATUSBAR_STATE_NOT_AVAILABLE:
    [self blitIcon:redCassette];
    break;
  }
}

-(void) drawRect:(NSRect)aRect
{
  [view_lock lock];

  [[self openGLContext] makeCurrentContext];

  /* Clear buffer, needs to be done each frame */
  glClear( GL_COLOR_BUFFER_BIT );

  if (!screenTexInitialised) {
    [view_lock unlock];
    return;
  }

  int border_x_offset = 0;
  int border_y_offset = 0;
  if( settings_current.full_screen ) {
    /* how much of the top and bottom borders should be eliminated? */
    NSRect rect = [self bounds];
    float width_adjustment = 0.0;

    border_x_offset =
      get_offset( rect.size.width, rect.size.height,
                  screenTex[currentScreenTex].image_width,
                  screenTex[currentScreenTex].image_height,
                  &width_adjustment );
    border_y_offset = width_adjustment;
  }

  /* Bind, update and draw new image */
  glBindTexture( GL_TEXTURE_RECTANGLE_ARB, screenTexId[currentScreenTex] );
  
  glBegin( GL_QUADS );
    glTexCoord2f( (float)(screenTex[currentScreenTex].image_width +
                          screenTex[currentScreenTex].image_xoffset + border_y_offset),
                  (float)(screenTex[currentScreenTex].image_yoffset + border_x_offset)
                  );
    glVertex2f( 1.0f, 1.0f );

    glTexCoord2f( (float)(screenTex[currentScreenTex].image_width +
                          screenTex[currentScreenTex].image_xoffset + border_y_offset),
                  (float)(screenTex[currentScreenTex].image_height +
                          screenTex[currentScreenTex].image_yoffset - border_x_offset)
                  );
    glVertex2f( 1.0f, -1.0f );

    glTexCoord2f( (float)screenTex[currentScreenTex].image_xoffset - border_y_offset,
                  (float)(screenTex[currentScreenTex].image_height +
                          screenTex[currentScreenTex].image_yoffset - border_x_offset)
                  );
    glVertex2f( -1.0f, -1.0f );

    glTexCoord2f( (float)screenTex[currentScreenTex].image_xoffset - border_y_offset,
                  (float)(screenTex[currentScreenTex].image_yoffset + border_x_offset)
                  );
    glVertex2f( -1.0f, 1.0f );
  glEnd();

  if ( settings_current.statusbar ) [self iconOverlay];

  /* Swap buffer to screen */
  [[self openGLContext] flushBuffer];

  statusbar_updated = NO;

  [view_lock unlock];
}

/* scrolled, moved or resized */
-(void) reshape
{
  [view_lock lock];
  NSRect rect;

  [super reshape];

  [[self openGLContext] makeCurrentContext];
  [[self openGLContext] update];

  rect = [self bounds];

  glViewport( 0, 0, (int) rect.size.width, (int) rect.size.height );

  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();

  statusbar_updated = YES;
  [view_lock unlock];
}

-(void) destroyTexture
{
  GLuint i;

  if (!screenTexInitialised)
    return;

  [view_lock lock];

  [self displayLinkStop];

  glDeleteTextures( MAX_SCREEN_BUFFERS, screenTexId );
  for(i = 0; i < MAX_SCREEN_BUFFERS; i++)
  {
    free( screenTex[i].pixels );
    screenTex[i].pixels = NULL;
    if( screenTex[i].dirty )
      pig_dirty_close( screenTex[i].dirty );
    screenTex[i].dirty = NULL;
  }
  screenTexInitialised = NO;
  [view_lock unlock];
}

-(void) createTexture:(Cocoa_Texture*)newScreen
{
  [view_lock lock];
  GLuint i;

  [[self openGLContext] makeCurrentContext];
  [[self openGLContext] update];

  glGenTextures( MAX_SCREEN_BUFFERS, screenTexId );

  for(i = 0; i < MAX_SCREEN_BUFFERS; i++)
  {
    screenTex[i].full_width = newScreen->full_width;
    screenTex[i].full_height = newScreen->full_height;
    screenTex[i].image_width = newScreen->image_width;
    screenTex[i].image_height = newScreen->image_height;
    screenTex[i].image_xoffset = newScreen->image_xoffset;
    screenTex[i].image_yoffset = newScreen->image_yoffset;
    screenTex[i].pixels = calloc( screenTex[i].full_width * screenTex[i].full_height,
                                  sizeof(uint16_t) );
    if( !screenTex[i].pixels ) {
      NSLog( @"%s: couldn't create screenTex[%ud].pixels\n", fuse_progname,
             (unsigned int)i );
      return;
    }
    screenTex[i].pitch = screenTex[i].full_width * sizeof(uint16_t);

    glDisable( GL_TEXTURE_2D );
    glEnable( GL_TEXTURE_RECTANGLE_ARB );
    glBindTexture( GL_TEXTURE_RECTANGLE_ARB, screenTexId[i] );

#if 0
    // These should increase texture upload performance, but instead seem to cause
    // issues with some ATI drivers (and perhaps GMA too), so I'm disabling for now
    // maybe revisit come 10.7
    glTextureRangeAPPLE( GL_TEXTURE_RECTANGLE_ARB,
                         screenTex[i].full_width * screenTex[i].pitch,
                         screenTex[i].pixels );

    glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_STORAGE_HINT_APPLE,
                     GL_STORAGE_CACHED_APPLE );
    glPixelStorei( GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE );
#endif
    GLint filter = settings_current.bilinear_filter ? GL_LINEAR : GL_NEAREST;
    glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, filter );
    glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, filter );
    glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );

    glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, screenTex[i].full_width,
                  screenTex[i].full_height, 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV,
                  screenTex[i].pixels );
  }
  screenTexInitialised = YES;

  [self displayLinkStart];

  [view_lock unlock];
}

-(void) openFile:(const char *)filename
{
  [proxy_emulator openFile:filename];
}

-(void) snapOpen:(const char *)filename
{
  [proxy_emulator snapOpen:filename];
}

-(void) tapeOpen:(const char *)filename
{
  [view_lock lock];
  [proxy_emulator tapeOpen:filename];
  [view_lock unlock];
}

-(void) tapeWrite:(const char *)filename;
{
  [proxy_emulator tapeWrite:filename];
}

-(void) tapeTogglePlay
{
  [proxy_emulator tapeTogglePlay];
}

-(void) tapeToggleRecord
{
  [proxy_emulator tapeToggleRecord];
}

-(void) tapeRewind
{
  [proxy_emulator tapeRewind];
}

-(void) tapeClear
{
  [proxy_emulator tapeClear];
}

-(int) tapeClose
{
  return [proxy_emulator tapeClose];
}

-(void) tapeWindowInitialise
{
  [proxy_emulator tapeWindowInitialise];
}

-(void) cocoaBreak
{
  [proxy_emulator cocoaBreak];
}

-(void) pause
{
  [proxy_emulator pause];

  [self displayLinkStop];

  /* FIXME: Show paused status somehow */
}

-(void) unpause
{
  [proxy_emulator unpause];

  [self displayLinkStart];
}

-(void) reset
{
  [proxy_emulator reset];
}

-(void) hard_reset
{
  [proxy_emulator hard_reset];
}

-(void) nmi
{
  [proxy_emulator nmi];
}

-(int) checkMediaChanged
{
  return [proxy_emulator checkMediaChanged];
}

-(void) diskInsertNew:(int)which
{
  [proxy_emulator diskInsertNew:which];
}

-(void) diskInsert:(const char *)filename inDrive:(int)which
{
  [proxy_emulator diskInsert:filename inDrive:which];
}

-(void) diskEject:(int)drive
{
  [proxy_emulator diskEject:drive];
}

-(void) diskSave:(int)drive saveAs:(bool)saveas
{
  [proxy_emulator diskSave:drive saveAs:saveas];
}

//-(int) diskWrite:(int)drive saveAs:(bool)saveas
//{
//  [proxy_emulator diskWrite:drive saveAs:saveas];
//}

-(void) diskFlip:(int)which side:(int)flip
{
  [proxy_emulator diskFlip:which side:flip];
}

-(void) diskWriteProtect:(int)which protect:(int)write
{
  [proxy_emulator diskWriteProtect:which protect:write];
}

-(void) snapshotWrite:(const char *)filename
{
  [proxy_emulator snapshotWrite:filename];
}

-(void) screenshotScrRead:(const char *)filename
{
  [proxy_emulator screenshotScrRead:filename];
}

-(void) screenshotScrWrite:(const char *)filename
{
  [proxy_emulator screenshotScrWrite:filename];
}

-(void) screenshotWrite:(const char *)filename
{
  [proxy_emulator screenshotWrite:filename];
}

-(void) profileStart
{
  [proxy_emulator profileStart];
}

-(void) profileFinish:(const char *)filename
{
  [proxy_emulator profileFinish:filename];
}

-(void) settingsSave
{
  [proxy_emulator settingsSave];
}

-(void) settingsResetDefaults
{
  [proxy_emulator settingsResetDefaults];
}

-(void) fullscreen
{
  [proxy_emulator fullscreen];
}

-(void) joystickToggleKeyboard
{
  [proxy_emulator joystickToggleKeyboard];
}

-(int) rzxStartPlayback:(const char *)filename
{
  return [proxy_emulator rzxStartPlayback:filename];
}

-(void) rzxInsertSnap
{
  [proxy_emulator rzxInsertSnap];
}

-(void) rzxRollback
{
  [proxy_emulator rzxRollback];
}

-(int) rzxStartRecording:(const char *)filename embedSnapshot:(int)flag
{
  return [proxy_emulator rzxStartRecording:filename embedSnapshot:flag];
}

-(void) rzxStop
{
  [proxy_emulator rzxStop];
}

-(int) rzxContinueRecording:(const char *)filename
{
  return [proxy_emulator rzxContinueRecording:filename];
}

-(int) rzxFinaliseRecording:(const char *)filename
{
  return [proxy_emulator rzxFinaliseRecording:filename];
}

-(void)movieStartRecording:(const char *)filename
{
  [proxy_emulator movieStartRecording:filename];
}

-(void)movieTogglePause
{
  [proxy_emulator movieTogglePause];
}

-(void)movieStop
{
  [proxy_emulator movieStop];
}

-(void) didaktik80Snap
{
  [proxy_emulator didaktik80Snap];
}

-(void) if1MdrNew:(int)drive
{
  [proxy_emulator if1MdrNew:drive];
}

-(void) if1MdrInsert:(const char *)filename inDrive:(int)drive
{
  [proxy_emulator if1MdrInsert:filename inDrive:drive];
}

-(void) if1MdrCartEject:(int)drive
{
  [proxy_emulator if1MdrCartEject:drive];
}

-(void) if1MdrCartSave:(int)drive saveAs:(bool)saveas
{
  [proxy_emulator if1MdrCartSave:drive saveAs:saveas];
}

-(void) if1MdrWriteProtect:(int)w inDrive:(int)drive
{
  [proxy_emulator if1MdrWriteProtect:w inDrive:drive];
}

-(int) if2Insert:(const char *)filename
{
  return [proxy_emulator if2Insert:filename];
}

-(void) if2Eject
{
  [proxy_emulator if2Eject];
}

-(int) dckInsert:(const char *)filename
{
  return [proxy_emulator dckInsert:filename];
}

-(void) dckEject
{
  [proxy_emulator dckEject];
}

-(void) psgStart:(const char *)psgfile
{
  [proxy_emulator psgStart:psgfile];
}

-(void) psgStop
{
  [proxy_emulator psgStop];
}

-(int) simpleideInsert:(const char *)filename inUnit:(libspectrum_ide_unit)unit
{
  return [proxy_emulator simpleideInsert:filename inUnit:unit];
}

-(int) simpleideCommit:(libspectrum_ide_unit)unit
{
  return [proxy_emulator simpleideCommit:unit];
}

-(int) simpleideEject:(libspectrum_ide_unit)unit
{
  return [proxy_emulator simpleideEject:unit];
}

-(int) zxataspInsert:(const char *)filename inUnit:(libspectrum_ide_unit)unit
{
  return [proxy_emulator zxataspInsert:filename inUnit:unit];
}

-(int) zxataspCommit:(libspectrum_ide_unit)unit
{
  return [proxy_emulator zxataspCommit:unit];
}

-(int) zxataspEject:(libspectrum_ide_unit)unit
{
  return [proxy_emulator zxataspEject:unit];
}

-(int) zxcfInsert:(const char *)filename
{
  return [proxy_emulator zxcfInsert:filename];
}

-(int) zxcfCommit
{
  return [proxy_emulator zxcfCommit];
}

-(int) zxcfEject
{
  return [proxy_emulator zxcfEject];
}

-(int) divideInsert:(const char *)filename inUnit:(libspectrum_ide_unit)unit
{
  return [proxy_emulator divideInsert:filename inUnit:unit];
}

-(int) divideCommit:(libspectrum_ide_unit)unit
{
  return [proxy_emulator divideCommit:unit];
}

-(int) divideEject:(libspectrum_ide_unit)unit
{
  return [proxy_emulator divideEject:unit];
}

-(void) setDiskState:(NSNumber*)state
{
  disk_state = [state unsignedCharValue];
  [view_lock lock];
  statusbar_updated = YES;
  [view_lock unlock];
  [[FuseController singleton] setDiskState:state];
}

-(void) setTapeState:(NSNumber*)state
{
  tape_state = [state unsignedCharValue];
  [view_lock lock];
  statusbar_updated = YES;
  [view_lock unlock];
  [[FuseController singleton] setTapeState:state];
}

-(void) setMdrState:(NSNumber*)state
{
  mdr_state = [state unsignedCharValue];
  [view_lock lock];
  statusbar_updated = YES;
  [view_lock unlock];
  [[FuseController singleton] setMdrState:state];
}

-(ui_confirm_save_t) confirmSave:(NSString*)theMessage
{
  return [[FuseController singleton] confirmSave:theMessage];
}

-(int) confirm:(NSString*)theMessage
{
  return [[FuseController singleton] confirm:theMessage];
}

-(int) tapeWrite
{
  return [[FuseController singleton] tapeWrite];
}

-(int) diskWrite:(int)which saveAs:(bool)saveas
{
  return [[FuseController singleton] diskWrite:which saveAs:saveas];
}

-(int) if1MdrWrite:(int)which saveAs:(bool)saveas
{
  return [[FuseController singleton] if1MdrWrite:which saveAs:saveas];
}

-(ui_confirm_joystick_t) confirmJoystick:(libspectrum_joystick)type inputs:(int)theInputs
{
  return [[FuseController singleton] confirmJoystick:type inputs:theInputs];
}

-(void) debuggerActivate
{
  [[DebuggerController singleton] debugger_activate:nil];
}

-(void) mouseMoved:(NSEvent *)theEvent
{
  [proxy_emulator mouseMoved:theEvent];
}

-(void) mouseDown:(NSEvent *)theEvent
{
  [proxy_emulator mouseDown:theEvent];
}

-(void) mouseUp:(NSEvent *)theEvent
{
  [proxy_emulator mouseUp:theEvent];
}

-(void) rightMouseDown:(NSEvent *)theEvent
{
  [proxy_emulator rightMouseDown:theEvent];
}

-(void) rightMouseUp:(NSEvent *)theEvent
{
  [proxy_emulator rightMouseUp:theEvent];
}

-(void) otherMouseDown:(NSEvent *)theEvent
{
  [proxy_emulator otherMouseDown:theEvent];
}

-(void) otherMouseUp:(NSEvent *)theEvent
{
  [proxy_emulator otherMouseUp:theEvent];
}

-(void) flagsChanged:(NSEvent *)theEvent
{
  [proxy_emulator flagsChanged:theEvent];
}

-(void) keyDown:(NSEvent *)theEvent
{
  if( settings_current.full_screen ) {
    unichar c = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
    switch (c) {
    /* [Esc] exits fullScreen mode */
    case 27:
      [self fullscreen:nil];
      return;
      break;
    }
  }
  [proxy_emulator keyDown:theEvent];
}

-(void) keyUp:(NSEvent *)theEvent
{
  [proxy_emulator keyUp:theEvent];
}

-(BOOL) acceptsFirstResponder
{
  return YES;
}

-(BOOL) becomeFirstResponder
{
  return YES;
}

-(BOOL) resignFirstResponder
{
  return YES;
}

-(BOOL) isFlipped
{
  return YES;
}

/* Minimise code from example code posted by user arekkusu
 * (http://www.idevgames.com) at http://www.idevgames.com in thread
 * "Properly minimizing an OpenGL view"
 */
-(void) copyGLtoQuartz
{
  NSSize  size = [self frame].size;
  GLfloat zero = 0.0f;
  long    rowbytes = size.width * 4;
  rowbytes = (rowbytes + 3)& ~3;      // ctx rowbytes is always multiple of 4, per glGrab
  unsigned char* bitmap = malloc(rowbytes * size.height);
 
  // Stuffing around with OpenGL context - lock view while we do 
  [view_lock lock];

  [[NSOpenGLContext currentContext] makeCurrentContext];
  glFinish();                         // finish any pending OpenGL commands
  glPushAttrib(GL_ALL_ATTRIB_BITS);   // reset all properties that affect glReadPixels, in case app was using them
  glDisable(GL_COLOR_TABLE);
  glDisable(GL_CONVOLUTION_1D);
  glDisable(GL_CONVOLUTION_2D);
  glDisable(GL_HISTOGRAM);
  glDisable(GL_MINMAX);
  glDisable(GL_POST_COLOR_MATRIX_COLOR_TABLE);
  glDisable(GL_POST_CONVOLUTION_COLOR_TABLE);
  glDisable(GL_SEPARABLE_2D);
  
  glPixelMapfv(GL_PIXEL_MAP_R_TO_R, 1, &zero);
  glPixelMapfv(GL_PIXEL_MAP_G_TO_G, 1, &zero);
  glPixelMapfv(GL_PIXEL_MAP_B_TO_B, 1, &zero);
  glPixelMapfv(GL_PIXEL_MAP_A_TO_A, 1, &zero);
  
  glPixelStorei(GL_PACK_SWAP_BYTES, 0);
  glPixelStorei(GL_PACK_LSB_FIRST, 0);
  glPixelStorei(GL_PACK_IMAGE_HEIGHT, 0);
  glPixelStorei(GL_PACK_ALIGNMENT, 4); // force 4-byte alignment from RGBA framebuffer
  glPixelStorei(GL_PACK_ROW_LENGTH, 0);
  glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_PACK_SKIP_ROWS, 0);
  glPixelStorei(GL_PACK_SKIP_IMAGES, 0);
  
  glPixelTransferi(GL_MAP_COLOR, 0);
  glPixelTransferf(GL_RED_SCALE, 1.0f);
  glPixelTransferf(GL_RED_BIAS, 0.0f);
  glPixelTransferf(GL_GREEN_SCALE, 1.0f);
  glPixelTransferf(GL_GREEN_BIAS, 0.0f);
  glPixelTransferf(GL_BLUE_SCALE, 1.0f);
  glPixelTransferf(GL_BLUE_BIAS, 0.0f);
  glPixelTransferf(GL_ALPHA_SCALE, 1.0f);
  glPixelTransferf(GL_ALPHA_BIAS, 0.0f);
  glPixelTransferf(GL_POST_COLOR_MATRIX_RED_SCALE, 1.0f);
  glPixelTransferf(GL_POST_COLOR_MATRIX_RED_BIAS, 0.0f);
  glPixelTransferf(GL_POST_COLOR_MATRIX_GREEN_SCALE, 1.0f);
  glPixelTransferf(GL_POST_COLOR_MATRIX_GREEN_BIAS, 0.0f);
  glPixelTransferf(GL_POST_COLOR_MATRIX_BLUE_SCALE, 1.0f);
  glPixelTransferf(GL_POST_COLOR_MATRIX_BLUE_BIAS, 0.0f);
  glPixelTransferf(GL_POST_COLOR_MATRIX_ALPHA_SCALE, 1.0f);
  glPixelTransferf(GL_POST_COLOR_MATRIX_ALPHA_BIAS, 0.0f);
  glReadPixels(0, 0, size.width, size.height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, bitmap);
  glPopAttrib();

  [view_lock unlock];

  [self lockFocus];
  // create a CGImageRef from the memory block
  CGDataProviderDirectCallbacks gProviderCallbacks = { 0, get_byte_pointer, NULL, NULL, NULL };
  CGDataProviderRef provider = CGDataProviderCreateDirect(bitmap, rowbytes * size.height, &gProviderCallbacks);
  CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
  CGImageRef cgImage = CGImageCreate(size.width, size.height, 8, 32, rowbytes, cs,
      kCGBitmapByteOrder32Host | kCGImageAlphaNoneSkipFirst, provider, NULL, NO,
      kCGRenderingIntentDefault);

  // composite the CGImage into the view
  CGContextRef gc = [[NSGraphicsContext currentContext] graphicsPort];
  CGContextDrawImage(gc, CGRectMake(0, 0, size.width, size.height), cgImage);

  // clean up
  CGImageRelease(cgImage);
  CGDataProviderRelease(provider);
  CGColorSpaceRelease(cs);
  free(bitmap);

  [self unlockFocus];
  [[self window] flushWindow];
}

-(void) windowWillMiniaturize:(NSNotification *)aNotification
{
  [self copyGLtoQuartz];	
  [[self window] setOpaque:NO]; // required to make the Quartz underlay and the window shadow appear correctly
}

-(void) windowDidMiniaturize:(NSNotification *)notification
{
  [[self window] setOpaque:YES];
}

-(BOOL) windowShouldClose:(id)window
{
  if( cocoaui_confirm( "Exit Fuse?" ) ) {
    int error = [self checkMediaChanged];
    if( error ) return NO;

    [self displayLinkStop];

    return YES;
  }
  return NO;
}

-(CVReturn) displayFrame:(const CVTimeStamp *)timeStamp
{
  int i;
  PIG_dirtytable *workdirty = NULL;
 
  // Is it possible that while waiting for a lock the emulator is stopped?
  // or already holds the lock? If so give up on updating the frame rather
  // than deadlock on getting the lock - may mean that we miss some screen
  // updates if we are invoked while the buffered screen is being updated
  if( !buffered_screen_lock || [buffered_screen_lock tryLock] == NO ) {
    return kCVReturnSuccess;
  }

  if( buffered_screen.dirty->count == 0 && !statusbar_updated ) {
    [buffered_screen_lock unlock];
    return kCVReturnSuccess;
  }

  if( buffered_screen.dirty->count > 0 ) {

    // Make sure we lock the view if we are going to update the textures so
    // there is no concurrent access to the OpenGL context as the displaylink
    // callback is not on the main thread where resizing-related drawing will
    // occur, also cover the screen texture swap
    [view_lock lock];

    if (screenTex[currentScreenTex].dirty)
      pig_dirty_copy( &workdirty, screenTex[currentScreenTex].dirty );

    currentScreenTex = !currentScreenTex;

    pig_dirty_copy( &screenTex[currentScreenTex].dirty, buffered_screen.dirty );
    
    if( workdirty )
      pig_dirty_merge(workdirty, screenTex[currentScreenTex].dirty);
    else
      pig_dirty_copy(&workdirty, screenTex[currentScreenTex].dirty);
    
    /* Draw texture to screen */
    for(i = 0; i < workdirty->count; ++i)
      copy_area( &screenTex[currentScreenTex], &buffered_screen,
                 workdirty->rects + i );
    
    buffered_screen.dirty->count = 0;
    
    pig_dirty_close( workdirty );

    [[self openGLContext] makeCurrentContext];
    
    /* Bind, update and draw new image */
    glBindTexture( GL_TEXTURE_RECTANGLE_ARB, screenTexId[currentScreenTex] );
    
    glTexSubImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0,
                     screenTex[currentScreenTex].full_width,
                     screenTex[currentScreenTex].full_height, GL_BGRA,
                     GL_UNSIGNED_SHORT_1_5_5_5_REV,
                     screenTex[currentScreenTex].pixels );

    [view_lock unlock];
  }

  [buffered_screen_lock unlock];

  NSAutoreleasePool *pool = [NSAutoreleasePool new];
  [self drawRect:NSZeroRect];
  [pool release];

  return kCVReturnSuccess;
}

-(void) windowChangedScreen:(NSNotification*)inNotification
{
  NSWindow *window = [self window];
  CGDirectDisplayID displayID = (CGDirectDisplayID)[[[[window screen]
         deviceDescription] objectForKey:@"NSScreenNumber"] intValue];
  if((displayID != 0) && (mainViewDisplayID != displayID))
  {
    CVDisplayLinkSetCurrentCGDisplay(displayLink, displayID);
    mainViewDisplayID = displayID;
  }
}

-(void) windowDidDeminiaturize:(NSNotification *)inNotification
{
  [[FuseController singleton] releaseCmdKeys:@"m" withCode:QZ_m];
}

-(void) displayLinkStop
{
  if( displayLinkRunning == YES ) {
    CVReturn error = CVDisplayLinkStop( displayLink );
    if( error ) {
      NSLog( @"error stopping displayLink:%d", error );
    }
    displayLinkRunning = NO;
  }
}

-(void) displayLinkStart
{
  if( displayLinkRunning == NO ) {
    CVReturn error = CVDisplayLinkStart( displayLink );
    if( error ) {
      NSLog( @"error starting displayLink:%d", error );
    }
    displayLinkRunning = YES;
  }
}

@end
