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
  sound_init();		/* sound-init failure non-fatal? */
  if(!sound_enabled) if(timer_init()) return 1;
  fuse_keyboard_init();

  z80_init();

  error = machine_init_machines();
  if( error ) return error;
  error = machine_select_first();
  if( error ) return error;

  if( argc >= 2 ) snapshot_read( argv[1] );

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
