/* memory.c: Routines for accessing memory
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

/* Note that this file is compiled twice: once to produce the 'normal'
   versions of these functions, and once to produce the '_internal'
   versions, which do not trigger breakpoints. This behaviour is
   controlled by the 'INTERNAL' macro
*/

#include <config.h>

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "display.h"
#include "fuse.h"
#include "machine.h"
#include "pentagon.h"
#include "scld.h"
#include "settings.h"
#include "spec16.h"
#include "spec48.h"
#include "spec128.h"
#include "specplus3.h"
#include "spectrum.h"
#include "tc2048.h"
#include "trdos.h"
#include "ui/ui.h"

#ifdef INTERNAL
#define FUNCTION( name ) name##_internal
#else				/* #ifdef INTERNAL */
#define FUNCTION( name ) name
#endif				/* #ifdef INTERNAL */

libspectrum_byte
FUNCTION( spec16_readbyte )( libspectrum_word address )
{
  libspectrum_byte b;

  switch( address >> 14 ) {
  case 0: b = ROM[0][ address & 0x3fff ]; break;
  case 1:
#ifndef INTERNAL
    tstates += contend_memory( address );
#endif				/* #ifndef INTERNAL */
    b = RAM[5][ address & 0x3fff ]; break;
  case 2: case 3: b = 0xff; break;

  default: b = 0; break;	/* Keep gcc happy */
  }

#ifndef INTERNAL
  tstates += 3;

  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_READ, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;
#endif				/* #ifndef INTERNAL */

  return b;
}

void
FUNCTION( spec16_writebyte )( libspectrum_word address, libspectrum_byte b )
{
  switch( address >> 14 ) {
  case 0:
    if( settings_current.writable_roms ) ROM[0][ address & 0x3fff ] = b;
    break;
  case 1:
#ifndef INTERNAL
    tstates += contend_memory( address );
#endif				/* #ifndef INTERNAL */
    if( ( address & 0x3fff ) < 0x1b00 && RAM[5][ address & 0x3fff ] != b )
      display_dirty( address );
    RAM[5][ address & 0x3fff ] = b; break;
  case 2: case 3: break;
  }

#ifndef INTERNAL
  tstates += 3;

  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_WRITE, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;
#endif				/* #ifndef INTERNAL */

}

libspectrum_byte
FUNCTION( spec48_readbyte )( libspectrum_word address )
{
  libspectrum_byte b;

  switch( address >> 14 ) {
  case 0: b = ROM[0][ address & 0x3fff ]; break;
  case 1:
#ifndef INTERNAL
    tstates += contend_memory( address );
#endif				/* #ifndef INTERNAL */
    b = RAM[5][ address & 0x3fff ]; break;
  case 2: b = RAM[2][ address & 0x3fff ]; break;
  case 3: b = RAM[0][ address & 0x3fff ]; break;
  default: b = 0; break;	/* Keep gcc happy */
  }

#ifndef INTERNAL
  tstates += 3;

  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_READ, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;
#endif				/* #ifndef INTERNAL */

  return b;
}

void
FUNCTION( spec48_writebyte )( libspectrum_word address, libspectrum_byte b )
{
  switch( address >> 14 ) {
  case 0:
    if( settings_current.writable_roms ) ROM[0][ address & 0x3fff ] = b;
    break;
  case 1:
#ifndef INTERNAL
    tstates += contend_memory( address );
#endif				/* #ifndef INTERNAL */
    if( ( address & 0x3fff ) < 0x1b00 && RAM[5][ address & 0x3fff ] != b )
      display_dirty( address );
    RAM[5][ address & 0x3fff ] = b; break;
  case 2: RAM[2][ address & 0x3fff ] = b; break;
  case 3: RAM[0][ address & 0x3fff ] = b; break;
  }

#ifndef INTERNAL
  tstates += 3;

  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_WRITE, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;
#endif				/* #ifndef INTERNAL */

}

libspectrum_byte
FUNCTION( spec128_readbyte )( libspectrum_word address )
{
  int bank;

#ifndef INTERNAL
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_READ, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;
#endif				/* #ifndef INTERNAL */

  switch( address >> 14 ) {

  case 0:
#ifndef INTERNAL
    tstates += 3;
#endif				/* #ifndef INTERNAL */
    return ROM[machine_current->ram.current_rom][ address & 0x3fff ];

  case 1: bank = 5; break;
  case 2: bank = 2; break;
  case 3: bank = machine_current->ram.current_page; break;
  default: bank = 0; break;	/* Keep gcc happy */
  }

#ifndef INTERNAL
  if( bank & 0x01 ) tstates += contend_memory( address );
  tstates += 3;
#endif				/* #ifndef INTERNAL */

  return RAM[bank][ address & 0x3fff ];
}

void
FUNCTION( spec128_writebyte )( libspectrum_word address, libspectrum_byte b )
{
  int bank = address >> 14;

#ifndef INTERNAL
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_WRITE, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;
#endif				/* #ifndef INTERNAL */

  switch( bank ) {

  case 0:
    if( settings_current.writable_roms )
      ROM[ machine_current->ram.current_rom ][ address & 0x3fff ] = b;
#ifndef INTERNAL
    tstates += 3;
#endif				/* #ifndef INTERNAL */
    return;

  case 1: bank = 5;				    break;
  case 2: bank = 2;				    break;
  case 3: bank = machine_current->ram.current_page; break;
  }

  if( bank == machine_current->ram.current_screen &&
      ( address & 0x3fff ) < 0x1b00 &&
      RAM[ bank ][ address & 0x3fff ] != b )
    display_dirty( ( address & 0x3fff ) | 0x4000 );
    
#ifndef INTERNAL
  if( bank & 0x01 ) tstates += contend_memory( address );
  tstates += 3;
#endif				/* #ifndef INTERNAL */

  RAM[ bank ][ address & 0x3fff ] = b;
}

libspectrum_byte
FUNCTION( specplus3_readbyte )( libspectrum_word address )
{
  int bank = address >> 14;

#ifndef INTERNAL
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_READ, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;
#endif				/* #ifndef INTERNAL */

  if( machine_current->ram.special ) {
    switch( machine_current->ram.specialcfg ) {

    case 0: break;
    case 1: bank += 4; break;

    case 2:
      switch( bank ) {
      case 0: bank = 4; break;
      case 1: bank = 5; break;
      case 2: bank = 6; break; 
      case 3: bank = 3; break;
      }
      break;

    case 3:
      switch( bank ) {
      case 0: bank = 4; break;
      case 1: bank = 7; break;
      case 2: bank = 6; break; 
      case 3: bank = 3; break;
      }

    default:
      ui_error( UI_ERROR_ERROR, "Unknown +3 special configuration %d",
		machine_current->ram.specialcfg );
      fuse_abort();
    }

  } else {
    switch( bank ) {
    case 0:
#ifndef INTERNAL
    tstates += 3;
#endif				/* #ifndef INTERNAL */
    return ROM[machine_current->ram.current_rom][ address & 0x3fff ];

    case 1: bank = 5; break;
    case 2: bank = 2; break;
    case 3: bank = machine_current->ram.current_page;

    }
  }

#ifndef INTERNAL
  if( bank & 0x04 ) tstates += contend_memory( address );
  tstates += 3;
#endif				/* #ifndef INTERNAL */

  return RAM[bank][ address & 0x3fff ];
}

void
FUNCTION( specplus3_writebyte )( libspectrum_word address, libspectrum_byte b )
{
  int bank = address >> 14;

#ifndef INTERNAL
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_WRITE, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;
#endif				/* #ifndef INTERNAL */

  if( machine_current->ram.special ) {
    switch( machine_current->ram.specialcfg ) {
      case 0: break;
      case 1: bank+=4; break;
      case 2:
	switch( bank ) {
	  case 0: bank=4; break;
	  case 1: bank=5; break;
	  case 2: bank=6; break;
	  case 3: bank=3; break;
	}
	break;
      case 3: switch( bank ) {
	case 0: bank=4; break;
	case 1: bank=7; break;
	case 2: bank=6; break;
	case 3: bank=3; break;
      }
      break;
    }
  } else {
    switch( bank ) {

      case 0:
	if( settings_current.writable_roms )
	  ROM[ machine_current->ram.current_rom ][ address & 0x3fff ] = b;
#ifndef INTERNAL
	tstates += 3;
#endif				/* #ifndef INTERNAL */
	return;

      case 1: bank=5;				      break;
      case 2: bank=2;				      break;
      case 3: bank=machine_current->ram.current_page; break;
    }
  }

  if( bank == machine_current->ram.current_screen &&
      ( address & 0x3fff ) < 0x1b00 &&
      RAM[ bank ][ address & 0x3fff ] != b )
    display_dirty( ( address & 0x3fff ) | 0x4000 );
    
#ifndef INTERNAL
  if( bank & 0x04 ) tstates += contend_memory( address );
  tstates += 3;
#endif				/* #ifndef INTERNAL */

  RAM[ bank ][ address & 0x3fff ] = b;
}


libspectrum_byte
FUNCTION( tc2048_readbyte )( libspectrum_word address )
{
  libspectrum_byte b;

  switch( address >> 14 ) {
  case 0: b = ROM[0][ address & 0x3fff ]; break;
  case 1:
#ifndef INTERNAL
    tstates += contend_memory( address );
#endif				/* #ifndef INTERNAL */
    b = RAM[5][ address & 0x3fff ]; break;
  case 2: b = RAM[2][ address & 0x3fff ]; break;
  case 3: b = RAM[0][ address & 0x3fff ]; break;
  default: b = 0; break;	/* Keep gcc happy */
  }

#ifndef INTERNAL
  tstates += 3;

  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_READ, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;
#endif				/* #ifndef INTERNAL */

  return b;
}

void
FUNCTION( tc2048_writebyte )( libspectrum_word address, libspectrum_byte b )
{
  switch( address >> 14 ) {
  case 0:
    if( settings_current.writable_roms ) ROM[0][ address & 0x3fff ] = b;
    break;
  case 1:
#ifndef INTERNAL
    tstates += contend_memory( address );
#endif				/* #ifndef INTERNAL */
    if( RAM[5][ address & 0x3fff ] != b ) display_dirty( address );
    RAM[5][ address & 0x3fff ] = b; break;
  case 2: RAM[2][ address & 0x3fff ] = b; break;
  case 3: RAM[0][ address & 0x3fff ] = b; break;
  }

#ifndef INTERNAL
  tstates += 3;

  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_WRITE, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;
#endif				/* #ifndef INTERNAL */
}

libspectrum_byte
FUNCTION( pentagon_readbyte )( libspectrum_word address )
{

#ifndef INTERNAL
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_READ, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;

  tstates += 3;
#endif				/* #ifndef INTERNAL */

  switch( address >> 14 ) {
  case 0: if ( trdos_active )
            return ROM[                                2][ address & 0x3fff ];
          else
            return ROM[ machine_current->ram.current_rom][ address & 0x3fff ];
  case 1:   return RAM[                                5][ address & 0x3fff ];
  case 2:   return RAM[                                2][ address & 0x3fff ];
  case 3:   return RAM[machine_current->ram.current_page][ address & 0x3fff ];
  }
  return 0; /* Keep gcc happy */
}

void
FUNCTION( pentagon_writebyte )( libspectrum_word address, libspectrum_byte b )
{
  int bank = address >> 14;

#ifndef INTERNAL
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_WRITE, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;

  tstates += 3;
#endif				/* #ifndef INTERNAL */

  switch( bank ) {
  case 0:
    if( settings_current.writable_roms )
      ROM[ machine_current->ram.current_rom ][ address & 0x3fff ] = b;
    return;

  case 1: bank = 5;				    break;
  case 2: bank = 2;				    break;
  case 3: bank = machine_current->ram.current_page; break;
  }

  if( bank == machine_current->ram.current_screen &&
      ( address & 0x3fff ) < 0x1b00 &&
      RAM[ bank ][ address & 0x3fff ] != b )
    display_dirty( ( address & 0x3fff ) | 0x4000 );
    
  RAM[ bank ][ address & 0x3fff ] = b;
}

libspectrum_byte
FUNCTION( tc2068_readbyte )( libspectrum_word address )
{
  libspectrum_word offset = address & 0x1fff;
  int chunk = address >> 13;

#ifndef INTERNAL
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_READ, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;

  if( chunk == 2 || chunk == 3 ) tstates += contend_memory( address );
  tstates += 3;
#endif				/* #ifndef INTERNAL */

  return timex_memory[chunk].page[offset];
}

void
FUNCTION( tc2068_writebyte )( libspectrum_word address, libspectrum_byte b )
{
  libspectrum_word offset = address & 0x1fff;
  int chunk = address >> 13;

#ifndef INTERNAL
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_WRITE, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;

  if( chunk == 2 || chunk == 3 ) tstates += contend_memory( address );
  tstates += 3;
#endif				/* #ifndef INTERNAL */

/* Need to take check if write is to home bank screen area for display
   dirty checking */
  switch( chunk ) {
  case 2:
  case 3:
    if( timex_memory[chunk].page == timex_home[chunk].page &&
        timex_memory[chunk].page[offset] != b ) display_dirty( address );
    /* Fall through */
  default:
    if( timex_memory[chunk].writeable == 1 ) timex_memory[chunk].page[offset] = b;
  }
}
