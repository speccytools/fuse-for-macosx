/* fuse.c: The Free Unix Spectrum Emulator
   Copyright (c) 1999-2001 Philip Kendall

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

#include <stdio.h>

#include "display.h"
#include "event.h"
#include "keyboard.h"
#include "machine.h"
#include "snapshot.h"
#include "sound.h"
#include "spectrum.h"
#include "tape.h"
#include "timer.h"
#include "ui.h"
#include "z80/z80.h"

/* What name were we called under? */
char* fuse_progname;

/* A flag to say when we want to exit the emulator */
int fuse_exiting;

/* Is Spectrum emulation currently running? */
int fuse_emulation_running;

/* Are we going to try and use the sound card; this differs from
   sound.c:sound_enabled in that this gives a desire, whereas sound_enabled
   is an actual state; when the Spectrum emulation is not running, this
   stores whether we try to reenable the sound card afterwards */
int fuse_sound_in_use;

static int fuse_init(int argc, char **argv);
static void fuse_show_copyright(void);
static int fuse_end(void);

int main(int argc,char **argv)
{
  if(fuse_init(argc,argv)) {
    fprintf(stderr,"%s: error initalising -- giving up!\n", fuse_progname);
    return 1;
  }

  while( !fuse_exiting ) {
    z80_do_opcodes();
    event_do_events();
  }

  fuse_end();
  
  return 0;

}

static int fuse_init(int argc, char **argv)
{
  int error;

  fuse_show_copyright();

  fuse_progname=argv[0];

  if(display_init(&argc,&argv)) return 1;
  if(event_init()) return 1;

  fuse_sound_in_use = 0;
  sound_init();
  if(sound_enabled) {
    fuse_sound_in_use = 1;
  } else {
    if(timer_init()) return 1;
  }

  fuse_keyboard_init();

  z80_init();

  error = machine_init_machines();
  if( error ) return error;
  error = machine_select_first();
  if( error ) return error;

  if( argc >= 2 ) snapshot_read( argv[1] );

  fuse_emulation_running = 1;

  return 0;

}

static void fuse_show_copyright(void)
{
  printf(
   "\nThe Free Unix Spectrum Emulator (Fuse) version " VERSION ".\n"
   "Copyright (c) 1999-2001 Philip Kendall <pak21-fuse@srcf.ucam.org> and others;\n"
   "See the file `AUTHORS' for more details.\n"
   "\n"
   "This program is distributed in the hope that it will be useful,\n"
   "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
   "GNU General Public License for more details.\n\n");

}

/* Stop all activities associated with actual Spectrum emulation */
int fuse_emulation_pause(void)
{
  fuse_emulation_running = 0;

  /* If we had sound enabled (and hence doing the speed regulation),
     turn it off */
  if( sound_enabled ) sound_end();

  return 0;
}

/* Restart emulation activities */
int fuse_emulation_unpause(void)
{
  /* If we were previously using sound, re-enable it */
  if( fuse_sound_in_use ) {
    sound_init();

    /* If the sound code couldn't re-initialise, fall back to the
       signal based routines */
    if( !sound_enabled ) {
      fprintf( stderr, "%s: Couldn't reinitialise sound", fuse_progname );
      fuse_sound_in_use = 0;
      /* FIXME: How to deal with error return here? */
      timer_init();
    }
  }

  fuse_emulation_running = 1;

  return 0;
}

/* Tidy-up function called at end of emulation */
static int fuse_end(void)
{
  int error;

  error = machine_end();
  if( error ) return error;

  if(!sound_enabled) timer_end();
  sound_end();
  event_end();
  ui_end();

  return 0;
}
