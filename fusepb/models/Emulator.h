/* Emulator.h: Implementation for the Emulator class
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

*/

#import <Cocoa/Cocoa.h>

#include <libspectrum.h>

#include "input.h"
#include "machines/specplus3.h"
#include "peripherals/disk/beta.h"
#include "ui/ui.h"

@class DisplayOpenGLView;

@interface Emulator : NSObject
{
  int isPaused;
  NSTimer* timer;
  CFAbsoluteTime time;
  float timerInterval;

  GHashTable *unicode_keysyms_hash;
  GHashTable *recreated_keysyms_hash;

  BOOL optDown;
  BOOL ctrlDown;
  BOOL shiftDown;
  BOOL commandDown;

  DisplayOpenGLView *proxy_view;
}
+(Emulator *) instance;

-(void) connectWithPorts:(NSArray *)portArray;
-(void) stop;

-(void) startEmulationTimer;
-(void) stopEmulationTimer;

-(void) updateEmulation:(NSTimer*)theTimer;
-(void) updateEmulationForTimeDelta:(CFTimeInterval)deltaTime;
-(void) setEmulationHz:(float)hz;

/* FIXME: Could do with a setSettings? maybe we just update settings when
   emulation is paused? */
-(id) init;

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

-(void) mouseMoved:(NSEvent *)theEvent;
-(void) mouseDown:(NSEvent *)theEvent;
-(void) mouseUp:(NSEvent *)theEvent;
-(void) rightMouseDown:(NSEvent *)theEvent;
-(void) rightMouseUp:(NSEvent *)theEvent;
-(void) otherMouseDown:(NSEvent *)theEvent;
-(void) otherMouseUp:(NSEvent *)theEvent;

-(void) initKeyboard;
-(void) modifierChange:(input_event_type)theType oldState:(BOOL)old newState:(BOOL)new;
-(void) flagsChanged:(NSEvent *)theEvent;
-(input_key) otherKeysymsRemap:(libspectrum_dword)ui_keysym inHash:(GHashTable*)hash;
-(void) keyboardReleaseAll;
-(void) keyChange:(NSEvent *)theEvent type:(input_event_type)type;
-(void) keyDown:(NSEvent *)theEvent;
-(void) keyUp:(NSEvent *)theEvent;

-(ui_confirm_save_t) confirmSave:(NSString*)theMessage;
-(int) confirm:(NSString*)theMessage;
-(int) tapeWrite;
-(int) diskWrite:(int)which saveAs:(bool)saveas;
-(int) if1MdrWrite:(int)which saveAs:(bool)saveas;
-(ui_confirm_joystick_t) confirmJoystick:(libspectrum_joystick)type inputs:(int)theInputs;

-(void) debuggerActivate;

-(void)movieStartRecording:(const char *)filename;
-(void)movieTogglePause;
-(void)movieStop;

-(void) didaktik80Snap;

@property (getter=isEmulating,readonly) BOOL isEmulating;
@end
