/* specplus2.c: Spectrum +2 specific routines
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

#include "spec128.h"
#include "spectrum.h"

/* The +2 emulation just uses the 128K routines */

int specplus2_init(void)
{
  FILE *f;

  f=fopen("plus2-0.rom","rb");
  if(!f) return 1;
  fread(ROM[0],0x4000,1,f);

  f=freopen("plus2-1.rom","rb",f);
  if(!f) return 2;
  fread(ROM[1],0x4000,1,f);

  fclose(f);

  readbyte=spec128_readbyte;
  read_screen_memory=spec128_read_screen_memory;
  writebyte=spec128_writebyte;

  tstates=0;

  spectrum_set_timings(24,128,24,52,311,3.54690e6,8865);
  machine.reset=spec128_reset;

  machine.peripherals=spec128_peripherals;

  return 0;

}
