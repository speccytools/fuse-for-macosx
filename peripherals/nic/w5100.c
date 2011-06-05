/* w5100.c: Wiznet W5100 emulation
   
   Emulates a minimal subset of the Wiznet W5100 TCP/IP controller.

   Copyright (c) 2011 Philip Kendall
   
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

#include "config.h"

#include "fuse.h"
#include "ui/ui.h"
#include "w5100.h"

struct nic_w5100_t {
};

nic_w5100_t*
nic_w5100_alloc( void )
{
  nic_w5100_t *self = malloc( sizeof( *self ) );
  if( !self ) {
    ui_error( UI_ERROR_ERROR, "%s:%d out of memory", __FILE__, __LINE__ );
    fuse_abort();
  }
  return self;
}

void
nic_w5100_free( nic_w5100_t *self )
{
  free( self );
}

libspectrum_byte
nic_w5100_read( nic_w5100_t *self, libspectrum_word reg )
{
  printf( "w5100: reading 0xff from register 0x%03x\n", reg );
  return 0xff;
}

void
nic_w5100_write( nic_w5100_t *self, libspectrum_word reg, libspectrum_byte b )
{
  printf( "w5100: writing 0x%02x to register 0x%03x\n", b, reg );
}
