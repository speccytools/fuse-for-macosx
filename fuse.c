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

   E-mail: pak@ast.cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <stdio.h>

#include "display.h"
#include "event.h"
#include "keyboard.h"
#include "snapshot.h"
#include "sound.h"
#include "spectrum.h"
#include "tape.h"
#include "timer.h"
#include "ui.h"
#include "z80.h"

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
    fprintf(stderr,"Error initalising -- giving up!\n");
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
  fuse_show_copyright();

  fuse_progname=argv[0];

  if(display_init(&argc,&argv)) return 1;
  if(event_init()) return 1;
  sound_init();		/* sound-init failure non-fatal? */
  if(!sound_enabled) if(timer_init()) return 1;

  machine.machine=SPECTRUM_MACHINE_48; spectrum_init(); machine.reset();
  fuse_keyboard_init();
  z80_init();

  return 0;

}

static void fuse_show_copyright(void)
{
  printf(
   "The Free Unix Spectrum Emulator (Fuse) version " VERSION ".\n"
   "Copyright (c) 1999-2001 Philip Kendall <pak@ast.cam.ac.uk> and others.\n"
   "\n"
   "This program is distributed in the hope that it will be useful,\n"
   "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
   "GNU General Public License for more details.\n\n");

}

/* Tidy-up function called at end of emulation */
static int fuse_end(void)
{
  if(!sound_enabled) timer_end();
  sound_end();
  event_end();
  ui_end();

  return 0;
}
