/* memory.c: Routines for accessing memory
   Copyright (c) 1999-2002 Philip Kendall

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

#include "debugger/debugger.h"
#include "display.h"
#include "machine.h"
#include "spectrum.h"
#include "types.h"
#include "ui/ui.h"

#ifdef INTERNAL
#define FUNCTION( name ) name##_internal
#else				/* #ifdef INTERNAL */
#define FUNCTION( name ) name
#endif				/* #ifdef INTERNAL */

BYTE
FUNCTION( spec48_readbyte )( WORD address )
{

#ifndef INTERNAL
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_READ, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;
#endif				/* #ifndef INTERNAL */

  switch( address >> 14 ) {
  case 0: return ROM[0][ address & 0x3fff ];
  case 1: return RAM[5][ address & 0x3fff ];
  case 2: return RAM[2][ address & 0x3fff ];
  case 3: return RAM[0][ address & 0x3fff ];
  }
  return 0; /* Keep gcc happy */
}

void
FUNCTION( spec48_writebyte )( WORD address, BYTE b )
{

#ifndef INTERNAL
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_WRITE, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;
#endif				/* #ifndef INTERNAL */

  switch( address >> 14 ) {
  case 0: break;
  case 1:
    if( ( address & 0x3fff ) < 0x1b00 && RAM[5][ address & 0x3fff ] != b )
      display_dirty( address );
    RAM[5][ address & 0x3fff ] = b; break;
  case 2: RAM[2][ address & 0x3fff ] = b; break;
  case 3: RAM[0][ address & 0x3fff ] = b; break;
  }
}

BYTE
FUNCTION( spec128_readbyte )( WORD address )
{

#ifndef INTERNAL
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_READ, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;
#endif				/* #ifndef INTERNAL */

  switch( address >> 14 ) {
  case 0: return ROM[ machine_current->ram.current_rom][ address & 0x3fff ];
  case 1: return RAM[                                5][ address & 0x3fff ];
  case 2: return RAM[                                2][ address & 0x3fff ];
  case 3: return RAM[machine_current->ram.current_page][ address & 0x3fff ];
  }
  return 0; /* Keep gcc happy */
}

void
FUNCTION( spec128_writebyte )( WORD address, BYTE b )
{
  int bank = address >> 14;

#ifndef INTERNAL
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_WRITE, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;
#endif				/* #ifndef INTERNAL */

  switch( bank ) {
  case 0: return;
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

BYTE
FUNCTION( specplus3_readbyte )( WORD address )
{

#ifndef INTERNAL
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_READ, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;
#endif				/* #ifndef INTERNAL */

  if( machine_current->ram.special ) {
    switch( machine_current->ram.specialcfg ) {
      case 0: return RAM[   address >> 14       ][ address & 0x3fff ];
      case 1: return RAM[ ( address >> 14 ) + 4 ][ address & 0x3fff ];
      case 2: switch( address >> 14 ) {
	case 0: return RAM[4][ address & 0x3fff ];
	case 1: return RAM[5][ address & 0x3fff ];
	case 2: return RAM[6][ address & 0x3fff ];
	case 3: return RAM[3][ address & 0x3fff ];
      }
      case 3: switch( address >> 14 ) {
	case 0: return RAM[4][ address & 0x3fff ];
	case 1: return RAM[7][ address & 0x3fff ];
	case 2: return RAM[6][ address & 0x3fff ];
	case 3: return RAM[3][ address & 0x3fff ];
      }
      default:
	ui_error( UI_ERROR_ERROR, "Unknown +3 special configuration %d",
		  machine_current->ram.specialcfg );
	fuse_abort();
    }
  } else {
    switch( address >> 14 ) {
    case 0: return ROM[ machine_current->ram.current_rom][ address & 0x3fff ];
    case 1: return RAM[				       5][ address & 0x3fff ];
    case 2: return RAM[				       2][ address & 0x3fff ];
    case 3: return RAM[machine_current->ram.current_page][ address & 0x3fff ];
    }
  }

  return 0; /* Keep gcc happy */
}

void
FUNCTION( specplus3_writebyte )( WORD address, BYTE b )
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
      case 0: return;
      case 1: bank=5;				      break;
      case 2: bank=2;				      break;
      case 3: bank=machine_current->ram.current_page; break;
    }
  }

  if( bank == machine_current->ram.current_screen &&
      ( address & 0x3fff ) < 0x1b00 &&
      RAM[ bank ][ address & 0x3fff ] != b )
    display_dirty( ( address & 0x3fff ) | 0x4000 );
    
  RAM[ bank ][ address & 0x3fff ] = b;
}


BYTE
FUNCTION( tc2048_readbyte )( WORD address )
{
  WORD offset = address & 0x3fff;

#ifndef INTERNAL
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_READ, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;
#endif				/* #ifndef INTERNAL */

  switch( address >> 14 ) {
  case 0: return ROM[0][offset]; break;
  case 1: return RAM[5][offset]; break;
  case 2: return RAM[2][offset]; break;
  case 3: return RAM[0][offset]; break;
  }
  return 0; /* Keep gcc happy */
}

void
FUNCTION( tc2048_writebyte )( WORD address, BYTE b )
{
  WORD offset = address & 0x3fff;

#ifndef INTERNAL
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_WRITE, address ) )
    debugger_mode = DEBUGGER_MODE_HALTED;
#endif				/* #ifndef INTERNAL */

  switch( address >> 14 ) {
  case 0: break;
  case 1: 
    if( RAM[5][offset] != b ) { display_dirty( address ); RAM[5][offset] = b; }
    break;
  case 2: RAM[2][offset]=b; break;
  case 3: RAM[0][offset]=b; break;
  }
}
