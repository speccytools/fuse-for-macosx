/* unittests.c: unit testing framework for Fuse
   Copyright (c) 2008 Philip Kendall

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <libspectrum.h>

#include "fuse.h"
#include "machine.h"
#include "ula.h"

static int
contention_test( void )
{
  libspectrum_dword i, checksum = 0, target;
  int error = 0;

  for( i = 0; i < ULA_CONTENTION_SIZE; i++ ) {
    /* Naive, but it will do for now */
    checksum += ula_contention[ i ] * (libspectrum_dword)( i + 1 );
  }

  switch( machine_current->machine ) {
  case LIBSPECTRUM_MACHINE_16:
  case LIBSPECTRUM_MACHINE_48:
  case LIBSPECTRUM_MACHINE_SE:
    target = 2308862976UL;
    break;
  case LIBSPECTRUM_MACHINE_128:
  case LIBSPECTRUM_MACHINE_PLUS2:
    target = 2335183872UL;
    break;
  case LIBSPECTRUM_MACHINE_PLUS2A:
  case LIBSPECTRUM_MACHINE_PLUS3:
  case LIBSPECTRUM_MACHINE_PLUS3E:
    target = 3113754624UL;
    break;
  case LIBSPECTRUM_MACHINE_TC2048:
  case LIBSPECTRUM_MACHINE_TC2068:
    target = 2307895296UL;
    break;
  case LIBSPECTRUM_MACHINE_TS2068:
    target = 1975529472UL;
    break;
  case LIBSPECTRUM_MACHINE_PENT:
  case LIBSPECTRUM_MACHINE_PENT512:
  case LIBSPECTRUM_MACHINE_PENT1024:
  case LIBSPECTRUM_MACHINE_SCORP:
    target = 0;
    break;
  default:
    target = -1;
    break;
  }

  if( checksum != target ) {
    fprintf( stderr, "%s: contention test: checksum = %u, expected = %u\n", fuse_progname, checksum, target );
    error = 1;
  }

  return error;
}

int
unittests_run( void )
{
  int r;

  r += contention_test();

  return r;
}
