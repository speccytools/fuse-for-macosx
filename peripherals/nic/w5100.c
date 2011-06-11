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

enum w5100_registers {
  W5100_MR = 0x000,

  W5100_GWR0,
  W5100_GWR1,
  W5100_GWR2,
  W5100_GWR3,
  
  W5100_SUBR0,
  W5100_SUBR1,
  W5100_SUBR2,
  W5100_SUBR3,

  W5100_SHAR0,
  W5100_SHAR1,
  W5100_SHAR2,
  W5100_SHAR3,
  W5100_SHAR4,
  W5100_SHAR5,

  W5100_SIPR0,
  W5100_SIPR1,
  W5100_SIPR2,
  W5100_SIPR3,

  W5100_IMR = 0x016,

  W5100_RMSR = 0x01a,
  W5100_TMSR,
};

struct nic_w5100_t {
  libspectrum_byte gw[4];   /* Gateway IP address */
  libspectrum_byte sub[4];  /* Our subnet mask */
  libspectrum_byte sha[6];  /* MAC address */
  libspectrum_byte sip[4];  /* Our IP address */
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
  libspectrum_byte b;

  switch( reg )
  {
    case W5100_MR:
      /* We don't support any flags, so we always return zero here */
      b = 0x00;
      printf("w5100: reading 0x%02x from MR\n", b);
      break;
    case W5100_GWR0: case W5100_GWR1: case W5100_GWR2: case W5100_GWR3:
      b = self->gw[reg - W5100_GWR0];
      printf("w5100: reading 0x%02x from GWR%d\n", b, reg - W5100_GWR0);
      break;
    case W5100_SUBR0: case W5100_SUBR1: case W5100_SUBR2: case W5100_SUBR3:
      b = self->sub[reg - W5100_SUBR0];
      printf("w5100: reading 0x%02x from SUBR%d\n", b, reg - W5100_SUBR0);
      break;
    case W5100_SHAR0: case W5100_SHAR1: case W5100_SHAR2:
    case W5100_SHAR3: case W5100_SHAR4: case W5100_SHAR5:
      b = self->sha[reg - W5100_SHAR0];
      printf("w5100: reading 0x%02x from SHAR%d\n", b, reg - W5100_SHAR0);
      break;
    case W5100_SIPR0: case W5100_SIPR1: case W5100_SIPR2: case W5100_SIPR3:
      b = self->sip[reg - W5100_SIPR0];
      printf("w5100: reading 0x%02x from SIPR%d\n", b, reg - W5100_SIPR0);
      break;
    case W5100_IMR:
      /* We support only "allow all" */
      b = 0xef;
      printf("w5100: reading 0x%02x from IMR\n", b);
      break;
    case W5100_RMSR: case W5100_TMSR:
      /* We support only 2K per socket */
      b = 0x55;
      printf("w5100: reading 0x%02x from %s\n", b, reg == W5100_RMSR ? "RMSR" : "TMSR");
    default:
      b = 0xff;
      printf("w5100: reading 0x%02x from unsupported register 0x%03x\n", b, reg );
      break;
  }


  return b;
}

static void
reset_w5100( nic_w5100_t *self )
{
  printf("w5100: reset\n");

  memset( self->gw, 0xff, sizeof( self->gw ) );
  memset( self->sub, 0xff, sizeof( self->sub ) );
  memset( self->sha, 0xff, sizeof( self->sha ) );
  memset( self->sip, 0xff, sizeof( self->sip ) );
}

static void
w5100_write_mr( nic_w5100_t *self, libspectrum_byte b )
{
  printf("w5100: writing 0x%02x to MR\n", b);

  if( b & 0x80 )
    reset_w5100( self );

  if( b & 0x7f )
    printf("w5100: unsupported value 0x%02x written to MR\n", b);
}

static void
w5100_write_imr( nic_w5100_t *self, libspectrum_byte b )
{
  printf("w5100: writing 0x%02x to IMR\n", b);

  if( b != 0xef )
    printf("w5100: unsupported value 0x%02x written to IMR\n", b);
}
  

static void
w5100_write__msr( nic_w5100_t *self, libspectrum_word reg, libspectrum_byte b )
{
  const char *regname = reg == W5100_RMSR ? "RMSR" : "TMSR";

  printf("w5100: writing 0x%02x to %s\n", b, regname);

  if( b != 0x55 )
    printf("w5100: unsupported value 0x%02x written to %s\n", b, regname);
}

void
nic_w5100_write( nic_w5100_t *self, libspectrum_word reg, libspectrum_byte b )
{
  switch( reg )
  {
    case W5100_MR:
      w5100_write_mr( self, b );
      break;
    case W5100_GWR0: case W5100_GWR1: case W5100_GWR2: case W5100_GWR3:
      printf("w5100: writing 0x%02x to GWR%d\n", b, reg - W5100_GWR0);
      self->gw[reg - W5100_GWR0] = b;
      break;
    case W5100_SUBR0: case W5100_SUBR1: case W5100_SUBR2: case W5100_SUBR3:
      printf("w5100: writing 0x%02x to SUBR%d\n", b, reg - W5100_SUBR0);
      self->sub[reg - W5100_SUBR0] = b;
      break;
    case W5100_SHAR0: case W5100_SHAR1: case W5100_SHAR2:
    case W5100_SHAR3: case W5100_SHAR4: case W5100_SHAR5:
      printf("w5100: writing 0x%02x to SHAR%d\n", b, reg - W5100_SHAR0);
      self->sha[reg - W5100_SHAR0] = b;
      break;
    case W5100_SIPR0: case W5100_SIPR1: case W5100_SIPR2: case W5100_SIPR3:
      printf("w5100: writing 0x%02x to SIPR%d\n", b, reg - W5100_SIPR0);
      self->sip[reg - W5100_SIPR0] = b;
      break;
    case W5100_IMR:
      w5100_write_imr( self, b );
      break;
    case W5100_RMSR: case W5100_TMSR:
      w5100_write__msr( self, reg, b );
      break;
    default:
      printf( "w5100: writing 0x%02x to unsupported register 0x%03x\n", b, reg );
      break;
  }
}
