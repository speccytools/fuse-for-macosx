/* Emulator.m: Implementation for the Emulator class
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

#import "DisplayOpenGLView.h"
#import "Emulator.h"

#include "dck.h"
#include "debugger/debugger.h"
#include "divide.h"
#include "event.h"
#include "fuse.h"
#include "fusepb/main.h"
#include "if1.h"
#include "if2.h"
#include "keyboard.h"
#include "keystate.h"
#include "machine.h"
#include "menu.h"
#include "movie.h"
#include "peripherals/disk/didaktik.h"
#include "profile.h"
#include "psg.h"
#include "rzx.h"
#include "settings.h"
#include "simpleide.h"
#include "screenshot.h"
#include "sound.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"
#include "ui/cocoa/cocoascreenshot.h"
#include "ui/ui.h"
#include "ui/uimedia.h"
#include "utils.h"
#include "z80/z80.h"
#include "zxatasp.h"
#include "zxcf.h"

extern keysyms_map_t unicode_keysyms_map[];
extern keysyms_map_t recreated_keysyms_map[];

#include "sound/sfifo.h"

extern sfifo_t sound_fifo;

@implementation Emulator

static Emulator *instance = nil;

+(Emulator *) instance
{
  return instance;
}

-(void) connectWithPorts:(NSArray *)portArray
{
  NSAutoreleasePool *pool;
  NSConnection *serverConnection;

  pool = [[NSAutoreleasePool alloc] init];

  serverConnection = [NSConnection
                      connectionWithReceivePort:[portArray objectAtIndex:0]
                      sendPort:[portArray objectAtIndex:1]];
  [serverConnection setRootObject:self];
  proxy_view = (id)[serverConnection rootProxy];
  [proxy_view setServer:self];

  if( fuse_init( ac, av ) ) {
    fprintf( stderr, "%s: error initialising -- giving up!\n", fuse_progname );
  }

  while( !fuse_exiting ) {
    [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
        beforeDate:[NSDate distantFuture]];
  }

  [serverConnection invalidate];

  fuse_end();

  instance = nil;
  [pool release];

  return;
}

-(void) stop
{
  [self pause];
  fuse_exiting = 1;
}

-(id) init
{
  if ( instance ) {
    [self dealloc];
    self = instance;
  } else {
    self = [super init];
    instance = self;

    [self initKeyboard];
  }

  timer = nil;
  timerInterval = 1.0f / 50.0f;
  isPaused = 0;

  optDown = NO;
  ctrlDown = NO;
  shiftDown = NO;
  commandDown = NO;

  time = CFAbsoluteTimeGetCurrent();  /* set emulation time start time */

  return self;
}

-(void) updateEmulation:(NSTimer*)theTimer
{
  CFTimeInterval nowTime = CFAbsoluteTimeGetCurrent();
  CFTimeInterval deltaTime = nowTime - time;
  if (deltaTime <= 1.0) { /* skip pauses */
    [self updateEmulationForTimeDelta:deltaTime];
  }
  time = nowTime;
}

/* given a delta time in seconds, update overall emulation state */
-(void) updateEmulationForTimeDelta:(CFTimeInterval)deltaTime
{
  if( sound_enabled ) {
    int too_long = 0;
    /* emulate until fifo is full or it takes more than a frames-worth of
       time to do a frame because we will never catch up */
    while( ( sfifo_space( &sound_fifo ) >=
             ((sound_stereo_ay != SOUND_STEREO_AY_NONE)+1) * 2 * sound_framesiz ) && !too_long ) {
      CFTimeInterval startTime = CFAbsoluteTimeGetCurrent();
      spectrum_do_frame();
      CFTimeInterval endTime = CFAbsoluteTimeGetCurrent();
      if( (endTime - startTime) > 
          (0.8 * sound_framesiz / (float)settings_current.sound_freq ) ) 
        too_long = 1;
    }
  /* If we're fastloading, keep running frames until we have used up 95% of
     the timer interval */
  } else if( settings_current.fastload && tape_is_playing() ) {
    int done = 0;
    CFTimeInterval startTime = CFAbsoluteTimeGetCurrent();
    while( !done ) {
      spectrum_do_frame();
      CFTimeInterval endTime = CFAbsoluteTimeGetCurrent();
      if( (endTime - startTime) > (0.95 * timerInterval) ) 
        done = 1;
    }
  } else {
    float speed = ( settings_current.emulation_speed < 1 ?
                    100.0                                :
                    settings_current.emulation_speed ) / 100.0;
    libspectrum_dword time_tstates = deltaTime *
                                     machine_current->timings.processor_speed *
                                     speed + 0.5;
    spectrum_do_timer( time_tstates );
  }
}

-(void) setEmulationHz:(float)hz
{
  [self pause];
  /* Update emulation at double the emulated machines' frame rate for smoother
     animation */
  hz = hz * 2.0f;
  timerInterval = 1.0f / hz;
  [self unpause];
}

-(void) openFile:(const char *)filename
{
  utils_open_file( filename, settings_current.auto_load, NULL );

  display_refresh_all();
}

-(void) snapOpen:(const char *)filename
{
  snapshot_read( filename );

  display_refresh_all();
}

-(void) tapeOpen:(const char *)filename
{
  tape_open( filename, 0 );
}

-(void) tapeWrite:(const char *)filename;
{
  tape_write( filename );
}

-(void) tapeTogglePlay
{
  tape_toggle_play( 0 );
}

-(void) tapeToggleRecord
{
  if( tape_recording ) tape_record_stop();
  else tape_record_start();
}

-(void) tapeRewind
{
  tape_select_block( 0 );
}

-(void) tapeClear
{
  tape_close();
}

-(int) tapeClose
{
  return tape_close();
}

-(void) tapeWindowInitialise
{
  ui_tape_browser_update( UI_TAPE_BROWSER_NEW_TAPE, NULL );
}

-(void) cocoaBreak
{
  debugger_mode = DEBUGGER_MODE_HALTED;
}

-(void) pause
{
  fuse_emulation_pause();

  if( isPaused++ ) return;

  if( timer != nil ) {
    [self stopEmulationTimer];
  }
}

-(void) unpause
{
  fuse_emulation_unpause();

  if( --isPaused ) return;

  [self startEmulationTimer];
}

-(void) reset
{
  machine_reset(0);
}

-(void) hard_reset
{
  machine_reset(1);
}

-(void) nmi
{
  event_add( 0, z80_nmi_event );
}

-(int) checkMediaChanged
{
  return menu_check_media_changed();
}

-(void) diskInsertNew:(int)which
{
  ui_media_drive_info_t *drive;
  
  drive = ui_media_drive_find( which );
  if( !drive )
    return;
  ui_media_drive_insert( drive, NULL, 0 );
}

-(void) diskInsert:(const char *)filename inDrive:(int)which
{
  ui_media_drive_info_t *drive;
  
  drive = ui_media_drive_find( which );
  if( !drive )
    return;
  ui_media_drive_insert( drive, filename, 0 );
}

-(void) diskEject:(int)drive
{
  ui_media_drive_eject( drive );
}

-(void) diskSave:(int)drive saveAs:(bool)saveas
{
  ui_media_drive_save( drive, saveas );
}

-(void) diskFlip:(int)which side:(int)flip
{
  ui_media_drive_flip( which, flip );
}

-(void) diskWriteProtect:(int)which protect:(int)write
{
  ui_media_drive_writeprotect( which, write );
}

-(void) snapshotWrite:(const char *)filename
{
  snapshot_write( filename );
}

-(void) screenshotScrRead:(const char *)filename
{
  screenshot_scr_read( filename );
}

-(void) screenshotScrWrite:(const char *)filename
{
  screenshot_scr_write( filename );
}

-(void) screenshotWrite:(const char *)filename
{
  screenshot_write( filename );
}

-(void) profileStart
{
  profile_start();
}

-(void) profileFinish:(const char *)filename
{
  profile_finish( filename );
}

-(void) settingsSave
{
  settings_write_config( &settings_current );
}

-(void) settingsResetDefaults
{
  [NSUserDefaults resetStandardUserDefaults];
  settings_defaults( &settings_current );
}

-(void) fullscreen
{
  settings_current.full_screen = 1;
}

-(void) joystickToggleKeyboard
{
  settings_current.joy_keyboard = !settings_current.joy_keyboard;
}

-(int) rzxStartPlayback:(const char *)filename
{
  return rzx_start_playback( filename, 0 );
}

-(void) rzxInsertSnap
{
  libspectrum_snap *snap;
  libspectrum_error error;

  if( !rzx_recording ) return;

  libspectrum_rzx_stop_input( rzx );

  snap = libspectrum_snap_alloc();

  error = snapshot_copy_to( snap );
  if( error ) { libspectrum_snap_free( snap ); return; }

  libspectrum_rzx_add_snap( rzx, snap, 0 );

  libspectrum_rzx_start_input( rzx, tstates );
}

-(void) rzxRollback
{
  libspectrum_snap *snap;
  libspectrum_error error;

  if( !rzx_recording ) return;

  [self pause];

  error = libspectrum_rzx_rollback( rzx, &snap );
  if( error ) { [self unpause]; return; }

  error = snapshot_copy_from( snap );
  if( error ) { [self unpause]; return; }

  libspectrum_rzx_start_input( rzx, tstates );

  [self unpause];
}

-(int) rzxStartRecording:(const char *)filename embedSnapshot:(int)flag
{
  return rzx_start_recording( filename, flag );
}

-(void) rzxStop
{
  if( rzx_recording ) rzx_stop_recording();
  if( rzx_playback  ) rzx_stop_playback( 1 );
}

-(int) rzxContinueRecording:(const char *)filename
{
  return rzx_continue_recording(filename);
}

-(int) rzxFinaliseRecording:(const char *)filename
{
  return rzx_finalise_recording(filename);
}

-(void) if1MdrNew:(int)drive
{
  if1_mdr_insert( drive, NULL );
}

-(void) if1MdrInsert:(const char *)filename inDrive:(int)drive
{
  if1_mdr_insert( drive, filename );
}

-(void) if1MdrCartEject:(int)drive
{
  if1_mdr_eject( drive );
}

-(void) if1MdrCartSave:(int)drive saveAs:(bool)saveas
{
  if1_mdr_save( drive, saveas );
}

-(void) if1MdrWriteProtect:(int)w inDrive:(int)drive
{
  if1_mdr_writeprotect( drive, w );
}

-(int) if2Insert:(const char *)filename
{
  return if2_insert( filename );
}

-(void) if2Eject
{
  if2_eject();
}

-(int) dckInsert:(const char *)filename
{
  return dck_insert( filename );
}

-(void) dckEject
{
  dck_eject();
}

-(void) psgStart:(const char *)psgfile
{
  psg_start_recording( psgfile );
}

-(void) psgStop
{
  psg_stop_recording();
}

-(int) simpleideInsert:(const char *)filename inUnit:(libspectrum_ide_unit)unit
{
  return simpleide_insert( filename, unit );
}

-(int) simpleideCommit:(libspectrum_ide_unit)unit
{
  return simpleide_commit( unit );
}

-(int) simpleideEject:(libspectrum_ide_unit)unit
{
  return simpleide_eject( unit );
}

-(int) zxataspInsert:(const char *)filename inUnit:(libspectrum_ide_unit)unit
{
  return zxatasp_insert( filename, unit );
}

-(int) zxataspCommit:(libspectrum_ide_unit)unit
{
  return zxatasp_commit( unit );
}

-(int) zxataspEject:(libspectrum_ide_unit)unit
{
  return zxatasp_eject( unit );
}

-(int) zxcfInsert:(const char *)filename
{
  return zxcf_insert( filename );
}

-(int) zxcfCommit
{
  return zxcf_commit();
}

-(int) zxcfEject
{
  return zxcf_eject();
}

-(int) divideInsert:(const char *)filename inUnit:(libspectrum_ide_unit)unit
{
  return divide_insert( filename, unit );
}

-(int) divideCommit:(libspectrum_ide_unit)unit
{
  return divide_commit( unit );
}

-(int) divideEject:(libspectrum_ide_unit)unit
{
  return divide_eject( unit );
}

-(void) startEmulationTimer
{
  if( timer == nil ) {
    timer = [[NSTimer scheduledTimerWithTimeInterval: timerInterval
                      target:self selector:@selector(updateEmulation:)
                      userInfo:self repeats:true] retain];
  }
}

-(void) stopEmulationTimer
{
  if( timer != nil ) {
    [timer invalidate];
    [timer release];
    timer = nil;
  }
}

-(void) mouseMoved:(NSEvent *)theEvent
{
  if( ui_mouse_grabbed ) {
    int dx = [theEvent deltaX];
    int dy = [theEvent deltaY];

    if( dx < -128 ) dx = -128;
    else if( dx > 128 ) dx = 128;

    if( dy < -128 ) dy = -128;
    else if( dy > 128 ) dy = 128;

    ui_mouse_motion( dx, dy );
  }
}

-(void) mouseDown:(NSEvent *)theEvent
{
  ui_mouse_button( 1, 1 );
}

-(void) mouseUp:(NSEvent *)theEvent
{
  ui_mouse_button( 1, 0 );
}

-(void) rightMouseDown:(NSEvent *)theEvent
{
  ui_mouse_button( 3, 1 );
}

-(void) rightMouseUp:(NSEvent *)theEvent
{
  ui_mouse_button( 3, 0 );
}

-(void) otherMouseDown:(NSEvent *)theEvent
{
  ui_mouse_button( 2, 1 );
}

-(void) otherMouseUp:(NSEvent *)theEvent
{
  ui_mouse_button( 2, 0 );
}

-(GHashTable*) initKeySymsHash:(keysyms_map_t *)keysyms_map
{
  keysyms_map_t *ptr3;
  GHashTable* keysyms_hash = g_hash_table_new( g_int_hash, g_int_equal );

  for( ptr3 = keysyms_map; ptr3->ui; ptr3++ )
    g_hash_table_insert( keysyms_hash, &( ptr3->ui ), &( ptr3->fuse ) );

  return keysyms_hash;
}

-(void) initKeyboard
{
  unicode_keysyms_hash = [self initKeySymsHash:unicode_keysyms_map];
  recreated_keysyms_hash = [self initKeySymsHash:recreated_keysyms_map];
}

-(void) modifierChange:(input_event_type)theType oldState:(BOOL)old newState:(BOOL)new
{
  if( old != new ) {
    input_event_t fuse_event;
    fuse_event.types.key.spectrum_key = theType;
    if( new == YES )
      fuse_event.type = INPUT_EVENT_KEYPRESS;
    else
      fuse_event.type = INPUT_EVENT_KEYRELEASE;
    input_event( &fuse_event );
  }
}

-(void) flagsChanged:(NSEvent *)theEvent
{
  int flags = [theEvent modifierFlags];
  BOOL optDownNew = (flags & NSAlternateKeyMask) ? YES : NO;
  BOOL ctrlDownNew = (flags & NSControlKeyMask) ? YES : NO;
  BOOL shiftDownNew = ( flags & NSShiftKeyMask ) ? YES : NO;
  BOOL commandDownNew = ( flags & NSCommandKeyMask ) ? YES : NO;

  if( NO == commandDownNew ) {
    [self modifierChange:INPUT_KEY_Alt_L oldState:optDown newState:optDownNew];
    [self modifierChange:INPUT_KEY_Control_L oldState:ctrlDown newState:ctrlDownNew];
    [self modifierChange:INPUT_KEY_Shift_L oldState:shiftDown newState:shiftDownNew];
    
    optDown = optDownNew;
    ctrlDown = ctrlDownNew;
    shiftDown = shiftDownNew;
  } else {
    keyboard_release_all();
    reset_keystate( );
    
    optDown = NO;
    ctrlDown = NO;
    shiftDown = NO;
  }

  commandDown = commandDownNew;
}

-(input_key) otherKeysymsRemap:(libspectrum_dword)ui_keysym inHash:(GHashTable*)hash
{
  const input_key *ptr;

  ptr = g_hash_table_lookup( hash, &ui_keysym );

  return ptr ? *ptr : INPUT_KEY_NONE;
}

// Things that will be implemented as multiple key presses in the emulation
// core
-(BOOL) isSpecial:(input_key)type
{
    switch( type ) {
    case INPUT_KEY_BackSpace:
    case INPUT_KEY_minus:
    case INPUT_KEY_underscore:
    case INPUT_KEY_equal:
    case INPUT_KEY_plus:
    case INPUT_KEY_semicolon:
    case INPUT_KEY_colon:
    case INPUT_KEY_apostrophe:
    case INPUT_KEY_quotedbl:
    case INPUT_KEY_numbersign:
    case INPUT_KEY_comma:
    case INPUT_KEY_less:
    case INPUT_KEY_period:
    case INPUT_KEY_greater:
    case INPUT_KEY_slash:
    case INPUT_KEY_question:
    case INPUT_KEY_exclam:
    case INPUT_KEY_at:
    case INPUT_KEY_dollar:
    case INPUT_KEY_percent:
    case INPUT_KEY_ampersand:
    case INPUT_KEY_parenleft:
    case INPUT_KEY_parenright:
    case INPUT_KEY_asciicircum:
    case INPUT_KEY_asterisk:
      return YES;
      break;
    default:
      return NO;
    }
}

-(void) keyboardReleaseAll
{
  keyboard_release_all();
}

-(void) keyChange:(NSEvent *)theEvent type:(input_event_type)type
{
  if( [theEvent isARepeat] == YES ) return;
  unsigned short keyCode = [theEvent keyCode];
  NSString *characters = [theEvent charactersIgnoringModifiers];
  if( NO == commandDown && [characters length] ) {
    input_key fuse_keysym;
    enum events event_type;

    // If recreated ZX spectrum is enabled, look up unshifted key in recreated_keysyms_map,
    // else move on to normal path
    if( !( settings_current.recreated_spectrum &&
           (fuse_keysym = [self otherKeysymsRemap:keyCode inHash:recreated_keysyms_hash]) != INPUT_KEY_NONE) ) {
      fuse_keysym = keysyms_remap( keyCode );
      if( fuse_keysym == INPUT_KEY_NONE ) {
        fuse_keysym = [self otherKeysymsRemap:[characters characterAtIndex:0]
                                       inHash:unicode_keysyms_hash];
      }
    }
    
    if( [self isSpecial:fuse_keysym] == YES ) {
      event_type = type == INPUT_EVENT_KEYPRESS ? PRESS_SPECIAL :
        RELEASE_SPECIAL;
    } else {
      event_type = type == INPUT_EVENT_KEYPRESS ? PRESS_NORMAL : RELEASE_NORMAL;
    }

    process_keyevent( event_type, fuse_keysym );
  }
}

-(void) keyDown:(NSEvent *)theEvent
{
  [self keyChange:theEvent type:INPUT_EVENT_KEYPRESS];
}

-(void) keyUp:(NSEvent *)theEvent
{
  [self keyChange:theEvent type:INPUT_EVENT_KEYRELEASE];
}

-(ui_confirm_save_t) confirmSave:(NSString*)theMessage
{
  return [proxy_view confirmSave:theMessage];
}

-(int) confirm:(NSString*)theMessage
{
  return [proxy_view confirm:theMessage];
}

-(int) tapeWrite
{
  return [proxy_view tapeWrite];
}

-(int) diskWrite:(int)which saveAs:(bool)saveas
{
  return [proxy_view diskWrite:which saveAs:saveas];
}

-(int) if1MdrWrite:(int)which saveAs:(bool)saveas
{
  return [proxy_view if1MdrWrite:which saveAs:saveas];
}

-(ui_confirm_joystick_t) confirmJoystick:(libspectrum_joystick)type inputs:(int)theInputs
{
  return [proxy_view confirmJoystick:type inputs:theInputs];
}

-(void) debuggerActivate
{
  [proxy_view debuggerActivate];
}

-(void)movieStartRecording:(const char *)filename
{
  movie_start( filename );
}

-(void)movieTogglePause
{
  movie_pause();
}

-(void)movieStop
{
  movie_stop();
}

-(void) didaktik80Snap
{
  didaktik80_snap = 1;
  event_add( 0, z80_nmi_event );
}


@end
