/* fuse.c: The Free Unix Spectrum Emulator
   Copyright (c) 1999-2003 Philip Kendall

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

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#ifdef UI_SDL
#include <SDL.h>		/* Needed on MacOS X and Windows */
#endif				/* #ifdef UI_SDL */

#include "debugger/debugger.h"
#include "display.h"
#include "event.h"
#include "fuse.h"
#include "keyboard.h"
#include "machine.h"
#include "printer.h"
#include "rzx.h"
#include "settings.h"
#include "snapshot.h"
#include "sound.h"
#include "spectrum.h"
#include "specplus3.h"
#include "tape.h"
#include "timer.h"
#include "trdos.h"
#include "ui/ui.h"
#include "ui/scaler/scaler.h"
#include "utils.h"

#ifdef USE_WIDGET
#include "widget/widget.h"
#endif                          /* #ifdef USE_WIDGET */

#include "z80/z80.h"

/* What name were we called under? */
char* fuse_progname;

/* A flag to say when we want to exit the emulator */
int fuse_exiting;

/* Is Spectrum emulation currently paused, and if so, how many times? */
int fuse_emulation_paused;

/* Are we going to try and use the sound card; this differs from
   sound.c:sound_enabled in that this gives a desire, whereas sound_enabled
   is an actual state; when the Spectrum emulation is not running, this
   stores whether we try to reenable the sound card afterwards */
int fuse_sound_in_use;

static int fuse_init(int argc, char **argv);

static void fuse_show_copyright(void);
static void fuse_show_version( void );
static void fuse_show_help( void );

static int parse_nonoption_args( int argc, char **argv, int first_arg,
				 int autoload );

static int fuse_end(void);

int main(int argc, char **argv)
{
  if(fuse_init(argc,argv)) {
    fprintf(stderr,"%s: error initalising -- giving up!\n", fuse_progname);
    return 1;
  }

  if( settings_current.show_help ||
      settings_current.show_version ) return 0;

  while( !fuse_exiting ) {
    z80_do_opcodes();
    event_do_events();
  }

  fuse_end();
  
  return 0;

}

static int fuse_init(int argc, char **argv)
{
  int error, first_arg;
  int autoload;			/* Should we autoload tapes? */
  char *start_scaler;

  fuse_progname=argv[0];
  libspectrum_error_function = ui_libspectrum_error;
  
  if( settings_init( &first_arg, argc, argv ) ) return 1;
  autoload = settings_current.auto_load;

  if( settings_current.show_version ) {
    fuse_show_version();
    return 0;
  } else if( settings_current.show_help ) {
    fuse_show_help();
    return 0;
  }

  start_scaler = strdup( settings_current.start_scaler_mode );
  if( !start_scaler ) {
    ui_error( UI_ERROR_ERROR, "Out of memory at %s: %d", __FILE__, __LINE__ );
    return 1;
  }

  fuse_show_copyright();

  /* FIXME: order of these initialisation calls. Work out what depends on
     what */
  /* FIXME FIXME 20030407: really do this soon. This is getting *far* too
     hairy */
  fuse_keyboard_init();

  if( tape_init() ) return 1;

#ifdef USE_WIDGET
  if( widget_init() ) return 1;
#endif				/* #ifdef USE_WIDGET */

  if( event_init() ) return 1;
  
  if( display_init(&argc,&argv) ) return 1;

  /* Drop root privs if we have them */
  if( !geteuid() ) { setuid( getuid() ); }

  if( debugger_init() ) return 1;

  if( printer_init() ) return 1;
  if( rzx_init() ) return 1;

  z80_init();

  fuse_sound_in_use = 0;
  if( settings_current.sound && settings_current.emulation_speed == 100 )
    sound_init( settings_current.sound_device );

  /* Timing: if we're not producing sound, or we're using the SDL sound
     routines (which don't do timing for us), use the SIGALRM based timer */
#ifdef UI_SDL
  if( sound_enabled ) fuse_sound_in_use = 1;
  if( timer_init() ) return 1;
#else			/* #ifdef UI_SDL */
  if(sound_enabled) {
    fuse_sound_in_use = 1;
  } else {
    settings_current.sound = 0;
    if(timer_init()) return 1;
  }
#endif			/* #ifdef UI_SDL */

  error = machine_init_machines();
  if( error ) return error;

  error = machine_select_id( settings_current.start_machine );
  if( error ) return error;

  error = scaler_select_id( start_scaler ); free( start_scaler );
  if( error ) return error;

  if( settings_current.snapshot ) {
    snapshot_read( settings_current.snapshot ); autoload = 0;
  }

  /* Insert any tape file; if no snapshot file already specified,
     autoload the tape */
  if( settings_current.tape_file )
    tape_open( settings_current.tape_file, autoload );

  if( settings_current.playback_file ) {
    rzx_start_playback( settings_current.playback_file, NULL );
  } else if( settings_current.record_file ) {
    rzx_start_recording( settings_current.record_file, 1 );
  }

#ifdef HAVE_765_H
  if( settings_current.plus3disk_file ) {
    error = machine_select( LIBSPECTRUM_MACHINE_PLUS3 );
    if( error ) return error;

    specplus3_disk_insert( SPECPLUS3_DRIVE_A, settings_current.plus3disk_file );
  }
#endif				/* #ifdef HAVE_765_H */

  if( settings_current.trdosdisk_file ) {
    error = machine_select( LIBSPECTRUM_MACHINE_PENT );
    if( error ) return error;

    trdos_disk_insert( TRDOS_DRIVE_A, settings_current.trdosdisk_file );
  }

  if( parse_nonoption_args( argc, argv, first_arg, autoload ) ) return 1;

  fuse_emulation_paused = 0;

  return 0;

}

static void fuse_show_copyright(void)
{
  printf( "\n" );
  fuse_show_version();
  printf(
   "Copyright (c) 1999-2003 Philip Kendall <pak21-fuse@srcf.ucam.org> and others;\n"
   "See the file `AUTHORS' for more details.\n"
   "\n"
   "This program is distributed in the hope that it will be useful,\n"
   "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
   "GNU General Public License for more details.\n\n");
}

static void fuse_show_version( void )
{
  printf( "The Free Unix Spectrum Emulator (Fuse) version " VERSION ".\n" );
}

static void fuse_show_help( void )
{
  printf( "\n" );
  fuse_show_version();
  printf(
   "\nAvailable command-line options:\n\n"
   "Boolean options (use `--no-<option>' to turn off):\n\n"
   "--auto-load            Automatically load tape files when opened.\n"
   "--beeper-stereo        Add fake stereo to beeper emulation.\n"
   "--compress-rzx         Write RZX files out compressed.\n"
   "--double-screen        Write screenshots out as double size.\n"
   "--issue2               Emulate an Issue 2 Spectrum.\n"
   "--kempston             Emulate the Kempston joystick on QAOP<space>.\n"
   "--loading-sound        Emulate the sound of tapes loading.\n"
   "--separation           Use ACB stereo for the AY-3-8912 sound chip.\n"
   "--sound                Produce sound.\n"
   "--slt                  Turn SLT traps on.\n"
   "--traps                Turn tape traps on.\n\n"
   "Other options:\n\n"
   "--help                 This information.\n"
   "--machine <type>       Which machine should be emulated?\n"
   "--playback <filename>  Play back RZX file <filename>.\n"
   "--record <filename>    Record to RZX file <filename>.\n"
   "--snapshot <filename>  Load snapshot <filename>.\n"
   "--speed <percentage>   How fast should emulation run?\n"
   "--svga-mode <mode>     Which mode should be used for SVGAlib?\n"
   "--tape <filename>      Open tape file <filename>.\n"
   "--version              Print version number and exit.\n\n" );
}

/* Stop all activities associated with actual Spectrum emulation */
int fuse_emulation_pause(void)
{
  /* If we were already paused, just return. In any case, increment
     the pause count */
  if( fuse_emulation_paused++ ) return 0;

  /* If we had sound enabled (and hence doing the speed regulation),
     turn it off */
  if( sound_enabled ) sound_end();

  return 0;
}

/* Restart emulation activities */
int fuse_emulation_unpause(void)
{
  /* If this doesn't start us running again, just return. In any case,
     decrement the pause count */
  if( --fuse_emulation_paused ) return 0;

  /* If we now want sound, enable it */
  if( settings_current.sound && settings_current.emulation_speed == 100 ) {

    /* If sound has just started providing the timing, remove the old
       SIGALRM timer (the SDL sound routines do not provide timing) */
#ifndef UI_SDL
    if( !fuse_sound_in_use ) timer_end();
#endif				/* #ifndef UI_SDL */

    sound_init( settings_current.sound_device );

    /* If the sound code couldn't re-initialise, fall back to the
       signal based routines */
    if( !sound_enabled ) {
      /* Increment pause_count, report, decrement pause_count
       * (i.e. avoid the effects of fuse_emulation_{,un}pause).
       * Otherwise, we may be recursively reporting this error. */
      fuse_emulation_paused++;
      ui_error( UI_ERROR_ERROR, "Couldn't reinitialise sound" );
      fuse_emulation_paused--;
      settings_current.sound = fuse_sound_in_use = 0;
      /* FIXME: How to deal with error return here? */
#ifndef UI_SDL
      timer_init();
#endif				/* #ifndef UI_SDL */

    }
#ifdef UI_SDL
    timer_init();
#endif				/* #ifdef UI_SDL */
    fuse_sound_in_use = sound_enabled;
  }
#ifndef UI_SDL
  /* If we don't want sound any more, put previously did, start the SIGALRM
     timer */
  else if( fuse_sound_in_use ) {
    timer_init();
    fuse_sound_in_use = 0;
  }
#endif				/* #ifndef UI_SDL */

  return 0;
}

/* Make 'best guesses' as to what to do with non-option arguments */
static int
parse_nonoption_args( int argc, char **argv, int first_arg, int autoload )
{
  unsigned char *buffer; size_t length; libspectrum_id_t type;
  int error = 0;

  while( first_arg < argc ) {

    if( utils_read_file( argv[ first_arg ], &buffer, &length ) ) return 1;

    if( libspectrum_identify_file( &type, argv[ first_arg ], buffer,
				   length ) ) {
      munmap( buffer, length );
      return 1;
    }

    switch( type ) {

    case LIBSPECTRUM_ID_UNKNOWN:
      fprintf( stderr, "%s: couldn't identify `%s'\n", fuse_progname,
	       argv[ first_arg ] );
      munmap( buffer, length );
      return 1;

    case LIBSPECTRUM_ID_RECORDING_RZX:
      error = rzx_start_playback_from_buffer( buffer, length );
      break;

    case LIBSPECTRUM_ID_SNAPSHOT_SNA:
    case LIBSPECTRUM_ID_SNAPSHOT_Z80:
      error = snapshot_read_buffer( buffer, length, type );
      if( !error ) autoload = 0;
      break;

    case LIBSPECTRUM_ID_TAPE_TAP:
    case LIBSPECTRUM_ID_TAPE_TZX:
      error = tape_read_buffer( buffer, length, type, autoload );
      break;

#ifdef HAVE_765_H
    case LIBSPECTRUM_ID_DISK_DSK:
      error = machine_select( LIBSPECTRUM_MACHINE_PLUS3 );
      if( error ) return error;

      error = specplus3_disk_insert( SPECPLUS3_DRIVE_A, argv[ first_arg ] );
      break;
#endif				/* #ifdef HAVE_765_H */

    case LIBSPECTRUM_ID_DISK_SCL:
    case LIBSPECTRUM_ID_DISK_TRD:
      error = machine_select( LIBSPECTRUM_MACHINE_PENT );
      if( error ) return error;

      error = trdos_disk_insert( TRDOS_DRIVE_A, argv[ first_arg ] );
      break;

    default:
      fprintf( stderr, "%s: parse_nonoption_args: unknown type %d!\n",
	       fuse_progname, type );
      fuse_abort();
    }

    if( error ) { munmap( buffer, length ); return 1; }

    if( length && munmap( buffer, length ) ) {
      fprintf( stderr, "%s: parse_nonoption_args: couldn't munmap `%s': %s\n",
	       fuse_progname, argv[ first_arg ], strerror( errno ) );
      return 1;
    }

    first_arg++;
  }

  return 0;
}

/* Tidy-up function called at end of emulation */
static int fuse_end(void)
{
  int error;

  /* Must happen before memory is deallocated as we read the character
     set from memory for the text output */
  printer_end();

  rzx_end();
  debugger_end();

  error = machine_end();
  if( error ) return error;

#ifdef UI_SDL
  timer_end();
#else				/* #ifdef UI_SDL */
  if(!sound_enabled) timer_end();
#endif				/* #ifdef UI_SDL */

  sound_end();
  event_end();
  ui_end();

#ifdef USE_WIDGET
  widget_end();
#endif                          /* #ifdef USE_WIDGET */

  return 0;
}

/* Emergency shutdown */
int fuse_abort( void )
{
  fuse_end();
  abort();
}
