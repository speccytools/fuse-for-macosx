/* fuse.c: The Free Unix Spectrum Emulator
   Copyright (c) 1999 Philip Kendall

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

   E-mail: pak21@cam.ac.uk
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <stdio.h>

#include "alleg.h"
#include "display.h"
#include "keyboard.h"
#include "snapshot.h"
#include "spectrum.h"
#include "tape.h"
#include "timer.h"
#include "z80.h"

static int fuse_init(void);
static void fuse_show_copyright(void);
static int fuse_keys(void);

int main(void)
{

  if(fuse_init()) {
    fprintf(stderr,"Error initalising -- giving up!\n");
    return 1;
  }

  while(1) {
    if(fuse_keys()) break;
    display_line();
    spectrum_interrupt();
#ifdef TIMER_HOGCPU
    timer_delay();
#endif			/* #ifdef TIMER_HOGCPU */
    z80_do_opcode();
  }
  
  return 0;

}
#ifndef Xwin_ALLEGRO
END_OF_MAIN();
#endif		/* #ifndef Xwin_ALLEGRO */

static int fuse_init(void)
{
  int i;

  fuse_show_copyright();

  i=allegro_init(); if(i) return i;
 
#ifndef Xwin_ALLEGRO
  /* Let the console version throw away its root priviliges */
  setuid(getuid());
#endif

  timer_init();
  machine.machine=SPECTRUM_MACHINE_48; spectrum_init();
  display_init();
  keyboard_init();
  z80_init();

  return 0;

}

static void fuse_show_copyright(void)
{
  printf(
   "The Free Unix Spectrum Emulator (Fuse) version " VERSION ".\n"
   "Copyright (c) 1999 Philip Kendall <pak21@cam.ac.uk> and others;\n"
   "For other contributors, see the file `ChangeLog'.\n"
   "\n"
   "This program is distributed in the hope that it will be useful,\n"
   "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
   "GNU General Public License for more details.\n\n");

}

static int fuse_keys(void)
{
  if(key[KEY_F2 ]) {
    snapshot_write();
    while(key[KEY_F2 ]) ;
  }
  if(key[KEY_F3 ]) {
    snapshot_read(); display_entire_screen();
    while(key[KEY_F3 ]) ;
  }
  if(key[KEY_F5 ]) {
    machine.reset();
    while(key[KEY_F5 ]) ;
  }
  if(key[KEY_F7 ]) {
    tape_open();
    while(key[KEY_F7 ]) ;
  }
  if(key[KEY_F9 ]) {
    switch(machine.machine) {
      case SPECTRUM_MACHINE_48:
	machine.machine=SPECTRUM_MACHINE_128;
	break;
      case SPECTRUM_MACHINE_128:
	machine.machine=SPECTRUM_MACHINE_PLUS2;
	break;
      case SPECTRUM_MACHINE_PLUS2:
	machine.machine=SPECTRUM_MACHINE_PLUS3;
	break;
      case SPECTRUM_MACHINE_PLUS3:
	machine.machine=SPECTRUM_MACHINE_48;
	break;
    }
    spectrum_init(); machine.reset();
    while(key[KEY_F9 ]) ;
  }
#ifndef Xwin_ALLEGRO
  if(key[KEY_F11]) {
    display_start_res(320,200);
    while(key[KEY_F11]) ;
  }
  if(key[KEY_F12]) {
    display_start_res(320,240);
    while(key[KEY_F12]) ;
  }
#endif			/* #ifndef Xwin_ALLEGRO */
  return (key[KEY_F10]);
}
