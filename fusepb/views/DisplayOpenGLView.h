/* DisplayOpenGLView.h: Implementation for the DisplayOpenGLView class
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

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <OpenGL/glu.h>

#include <libspectrum.h>

#include "input.h"
#include "machines/specplus3.h"
#include "peripherals/disk/beta.h"
#include "ui/cocoa/cocoadisplay.h"
#include "ui/ui.h"

#define MAX_SCREEN_BUFFERS 2

@class Emulator;
@class Texture;

@interface DisplayOpenGLView : NSOpenGLView
{
  /* Two backing textures */
  Cocoa_Texture screenTex[MAX_SCREEN_BUFFERS];
  GLuint screenTexId[MAX_SCREEN_BUFFERS];
  int currentScreenTex;

  Texture *redCassette;
  Texture *greenCassette;
  Texture *redMdr;
  Texture *greenMdr;
  Texture *redDisk;
  Texture *greenDisk;

  BOOL screenTexInitialised;

  ui_statusbar_state disk_state;
  ui_statusbar_state mdr_state;
  ui_statusbar_state tape_state;
  BOOL statusbar_updated;

  NSLock *view_lock;

  NSWindow *fullscreenWindow;
  NSWindow *windowedWindow;

  float target_ratio;

  Emulator *real_emulator;
  Emulator *proxy_emulator;
  NSConnection *kitConnection;

  CVDisplayLinkRef displayLink;
  CGDirectDisplayID mainViewDisplayID;
  BOOL displayLinkRunning;
}
+(DisplayOpenGLView *) instance;

-(IBAction) fullscreen:(id)sender;
-(IBAction) zoom:(id)sender;

-(void) createTexture:(Cocoa_Texture*)newScreen;
-(void) destroyTexture;
-(void) blitIcon:(Texture*)iconTexture;

-(void) setServer:(id)anObject;
-(id) initWithFrame:(NSRect)frameRect;
-(void) awakeFromNib;

-(void) loadPicture:(NSString *) name
           greenTex:(Texture*) greenTexture
             redTex:(Texture*) redTexture
            xOrigin:(int) x
            yOrigin:(int) y;

-(void) setNeedsDisplayYes;

-(void) openFile:(const char *)filename;
-(void) snapOpen:(const char *)filename;
-(void) tapeOpen:(const char *)filename;
-(void) tapeWrite:(const char *)filename;
-(void) tapeTogglePlay;
-(void) tapeToggleRecord;
-(void) tapeRewind;
-(void) tapeClear;
-(int) tapeClose;
-(void) tapeWindowInitialise;
-(void) cocoaBreak;
-(void) pause;
-(void) unpause;
-(void) reset;
-(void) hard_reset;
-(void) nmi;
-(int) checkMediaChanged;

-(void) diskInsertNew:(int)which;
-(void) diskInsert:(const char *)filename inDrive:(int)which;
-(void) diskEject:(int)drive;
-(void) diskSave:(int)drive saveAs:(bool)saveas;
-(int) diskWrite:(int)drive saveAs:(bool)saveas;
-(void) diskFlip:(int)drive side:(int)flip;
-(void) diskWriteProtect:(int)which protect:(int)write;

-(void) snapshotWrite:(const char *)filename;

-(void) screenshotScrRead:(const char *)filename;
-(void) screenshotScrWrite:(const char *)filename;
-(void) screenshotWrite:(const char *)filename;

-(void) profileStart;
-(void) profileFinish:(const char *)filename;

-(void) settingsSave;
-(void) settingsResetDefaults;

-(void) fullscreen;

-(void) joystickToggleKeyboard;

-(int) rzxStartPlayback:(const char *)filename;
-(void) rzxInsertSnap;
-(void) rzxRollback;
-(int) rzxStartRecording:(const char *)filename embedSnapshot:(int)flag;
-(void) rzxStop;
-(int) rzxContinueRecording:(const char *)filename;
-(int) rzxFinaliseRecording:(const char *)filename;

-(void) movieStartRecording:(const char *)filename;
-(void) movieTogglePause;
-(void) movieStop;

-(void) didaktik80Snap;

-(void) if1MdrNew:(int)drive;
-(void) if1MdrInsert:(const char *)filename inDrive:(int)drive;
-(void) if1MdrCartEject:(int)drive;
-(void) if1MdrCartSave:(int)drive saveAs:(bool)saveas;
-(void) if1MdrWriteProtect:(int)w inDrive:(int)drive;

-(int) if2Insert:(const char *)filename;
-(void) if2Eject;

-(int) dckInsert:(const char *)filename;
-(void) dckEject;

-(void) psgStart:(const char *)psgfile;
-(void) psgStop;

-(int) simpleideInsert:(const char *)filename inUnit:(libspectrum_ide_unit)unit;
-(int) simpleideCommit:(libspectrum_ide_unit)unit;
-(int) simpleideEject:(libspectrum_ide_unit)unit;

-(int) zxataspInsert:(const char *)filename inUnit:(libspectrum_ide_unit)unit;
-(int) zxataspCommit:(libspectrum_ide_unit)unit;
-(int) zxataspEject:(libspectrum_ide_unit)unit;

-(int) zxcfInsert:(const char *)filename;
-(int) zxcfCommit;
-(int) zxcfEject;

-(int) divideInsert:(const char *)filename inUnit:(libspectrum_ide_unit)unit;
-(int) divideCommit:(libspectrum_ide_unit)unit;
-(int) divideEject:(libspectrum_ide_unit)unit;

-(void) setDiskState:(NSNumber*)state;
-(void) setTapeState:(NSNumber*)state;
-(void) setMdrState:(NSNumber*)state;

-(ui_confirm_save_t) confirmSave:(NSString*)theMessage;
-(int) confirm:(NSString*)theMessage;
-(int) tapeWrite;
-(int) diskWrite:(int)which saveAs:(bool)saveas;
-(int) if1MdrWrite:(int)which saveAs:(bool)saveas;
-(ui_confirm_joystick_t) confirmJoystick:(libspectrum_joystick)type inputs:(int)theInputs;

-(void) debuggerActivate;

-(void) mouseMoved:(NSEvent *)theEvent;
-(void) mouseDown:(NSEvent *)theEvent;
-(void) mouseUp:(NSEvent *)theEvent;
-(void) rightMouseDown:(NSEvent *)theEvent;
-(void) rightMouseUp:(NSEvent *)theEvent;
-(void) otherMouseDown:(NSEvent *)theEvent;
-(void) otherMouseUp:(NSEvent *)theEvent;

-(void) flagsChanged:(NSEvent *)theEvent;
-(void) keyDown:(NSEvent *)theEvent;
-(void) keyUp:(NSEvent *)theEvent;

-(BOOL) acceptsFirstResponder;
-(BOOL) becomeFirstResponder;
-(BOOL) resignFirstResponder;

-(BOOL) isFlipped;

-(void) copyGLtoQuartz;
-(void) windowWillMiniaturize:(NSNotification *)aNotification;
-(void) windowDidMiniaturize:(NSNotification *)notification;
-(BOOL) windowShouldClose:(id)window;
-(void) windowDidResignKey:(NSNotification *)notification;

-(CVReturn) displayFrame:(const CVTimeStamp *)timeStamp;
-(void) windowChangedScreen:(NSNotification*)inNotification;
-(void) windowDidDeminiaturize:(NSNotification *)inNotification;

-(void) displayLinkStop;
-(void) displayLinkStart;

@end
