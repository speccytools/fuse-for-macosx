/* scld.c: Routines for handling the Timex SCLD
   Copyright (c) 2002-2003 Fredrick Meunier, Philip Kendall, Witold Filipczyk

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

   E-mail: fredm@spamcop.net

*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>

#include "compat.h"
#include "display.h"
#include "scld.h"

scld scld_last_dec;                 /* The last byte sent to Timex DEC port */

BYTE scld_last_hsr   = 0;           /* The last byte sent to Timex HSR port */

BYTE timex_fake_bank[0x2000];

timex_mem timex_exrom_dock[8];
timex_mem timex_exrom[8];
timex_mem timex_dock[8];
timex_mem timex_home[8];
timex_mem timex_memory[8];

BYTE
scld_dec_read( WORD port GCC_UNUSED )
{
  return scld_last_dec.byte;
}

void
scld_dec_write( WORD port GCC_UNUSED, BYTE b )
{
  scld old_dec = scld_last_dec;
  BYTE ink,paper;

  scld_last_dec.byte = b;

  /* If we changed the active screen, or change the colour in hires
   * mode mark the entire display file as dirty so we redraw it on
   * the next pass */
  if((scld_last_dec.mask.scrnmode != old_dec.mask.scrnmode)
       || (scld_last_dec.name.hires &&
           (scld_last_dec.mask.hirescol != old_dec.mask.hirescol))) {
    display_refresh_all();
  }

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
scld_hsr_write( WORD port GCC_UNUSED, BYTE b )
{
  int i;
  
  scld_last_hsr = b;

  for( i = 0; i < 8; i++ ) {
    if( b & (1<<i) ) {
      timex_memory[i] = timex_exrom_dock[i];
    } else {
      timex_memory[i] = timex_home[i];
    }
  }
}

BYTE
scld_hsr_read( WORD port GCC_UNUSED )
{
  return scld_last_hsr;
}

BYTE
hires_get_attr( void )
{
  return( hires_convert_dec( scld_last_dec.byte ) );
}

BYTE
hires_convert_dec( BYTE attr )
{
  scld colour;

  colour.byte = attr;

  switch ( colour.mask.hirescol )
  {
    case YELLOWBLUE:
      return (BYTE) 0x71;
      break;
    case CYANRED:
      return (BYTE) 0x6A;
      break;
    case GREENMAGENTA:
      return (BYTE) 0x63;
      break;
    case MAGENTAGREEN:
      return (BYTE) 0x5C;
      break;
    case REDCYAN:
      return (BYTE) 0x55;
      break;
    case BLUEYELLOW:
      return (BYTE) 0x4E;
      break;
    case BLACKWHITE:
      return (BYTE) 0x47;
      break;
    default:  /* WHITEBLACK */
      return (BYTE) 0x78;
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
