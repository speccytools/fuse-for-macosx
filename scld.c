/* scld.c: Routines for handling the Timex SCLD
   Copyright (c) 2002-2004 Fredrick Meunier, Philip Kendall, Witold Filipczyk

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

   Philip: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

   Fred: fredm@spamcop.net

*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>

#include "compat.h"
#include "display.h"
#include "machine.h"
#include "memory.h"
#include "scld.h"
#include "z80/z80.h"

scld scld_last_dec;                 /* The last byte sent to Timex DEC port */

libspectrum_byte scld_last_hsr = 0; /* The last byte sent to Timex HSR port */

libspectrum_byte timex_fake_bank[0x2000];

memory_page timex_exrom_dock[8];
memory_page timex_exrom[8];
memory_page timex_dock[8];
memory_page timex_home[8];

libspectrum_byte
scld_dec_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  *attached = 1;

  return scld_last_dec.byte;
}

void
scld_dec_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  scld old_dec = scld_last_dec;
  libspectrum_byte ink,paper;

  scld_last_dec.byte = b;

  /* If we changed the active screen, or change the colour in hires
   * mode mark the entire display file as dirty so we redraw it on
   * the next pass */
  if((scld_last_dec.mask.scrnmode != old_dec.mask.scrnmode)
       || (scld_last_dec.name.hires &&
           (scld_last_dec.mask.hirescol != old_dec.mask.hirescol))) {
    display_refresh_all();
  }

  /* If we just reenabled interrupts, check for a retriggered interrupt */
  if( old_dec.name.intdisable && !scld_last_dec.name.intdisable )
    z80_interrupt();

  if( scld_last_dec.name.altmembank != old_dec.name.altmembank ) {
    int i;
    
    if( scld_last_dec.name.altmembank ) {
      for( i = 0; i < 8; i++ ) {
        timex_exrom_dock[i] = timex_exrom[i];
      }
    } else {
      for( i = 0; i < 8; i++ ) {
        timex_exrom_dock[i] = timex_dock[i];
      }
    }
    scld_hsr_write( 0xf4, scld_last_hsr );
  }

  display_parse_attr( hires_get_attr(), &ink, &paper );
  display_set_hires_border( paper );
}

void
scld_reset(void)
{
  scld_last_dec.byte = 0;
}

void
scld_hsr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  int i;
  
  scld_last_hsr = b;

  if( libspectrum_machine_capabilities( machine_current->machine ) &
      LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_DOCK )
    for( i = 0; i < 8; i++ )
      memory_map[i] = ( b & ( 1 << i ) ) ? timex_exrom_dock[i] : timex_home[i];

}

libspectrum_byte
scld_hsr_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  *attached = 1;

  return scld_last_hsr;
}

libspectrum_byte
hires_get_attr( void )
{
  return( hires_convert_dec( scld_last_dec.byte ) );
}

libspectrum_byte
hires_convert_dec( libspectrum_byte attr )
{
  scld colour;

  colour.byte = attr;

  switch ( colour.mask.hirescol )
  {
    case BLACKWHITE:   return 0x47;
    case BLUEYELLOW:   return 0x4e;
    case REDCYAN:      return 0x55;
    case MAGENTAGREEN: return 0x5c;
    case GREENMAGENTA: return 0x63;
    case CYANRED:      return 0x6a;
    case YELLOWBLUE:   return 0x71;
    default:	       return 0x78; /* WHITEBLACK */
  }
}

void
scld_dock_free( void )
{
  size_t i;

  for( i = 0; i < 8; i++ ) {
    if( timex_dock[i].allocated ) {
      free( timex_dock[i].page );
      timex_dock[i].page = 0;
      timex_dock[i].allocated = 0;
    }
  }
}

void
scld_exrom_free( void )
{
  size_t i;

  for( i = 0; i < 8; i++ ) {
    if( timex_exrom[i].allocated ) {
      free( timex_exrom[i].page );
      timex_exrom[i].page = 0;
      timex_exrom[i].allocated = 0;
    }
  }
}

void
scld_home_free( void )
{
  size_t i;

  for( i = 0; i < 8; i++ ) {
    if( timex_home[i].allocated ) {
      free( timex_home[i].page );
      timex_home[i].page = 0;
      timex_home[i].allocated = 0;
    }
  }
}
