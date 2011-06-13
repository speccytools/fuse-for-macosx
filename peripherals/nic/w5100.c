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

enum w5100_socket_registers {
  W5100_SOCKET_MR = 0x00,
  W5100_SOCKET_CR,
  W5100_SOCKET_IR,
  W5100_SOCKET_SR,
  
  W5100_SOCKET_PORT0,
  W5100_SOCKET_PORT1,

  W5100_SOCKET_DIPR0 = 0x0c,
  W5100_SOCKET_DIPR1,
  W5100_SOCKET_DIPR2,
  W5100_SOCKET_DIPR3,

  W5100_SOCKET_DPORT0,
  W5100_SOCKET_DPORT1,

  W5100_SOCKET_TX_FSR0 = 0x20,
  W5100_SOCKET_TX_FSR1,
  W5100_SOCKET_TX_RR0,
  W5100_SOCKET_TX_RR1,
  W5100_SOCKET_TX_WR0,
  W5100_SOCKET_TX_WR1,
};

typedef enum w5100_socket_mode {
  W5100_SOCKET_MODE_CLOSED = 0x00,
  W5100_SOCKET_MODE_TCP,
  W5100_SOCKET_MODE_UDP,
} w5100_socket_mode;

typedef enum w5100_socket_state {
  W5100_SOCKET_STATE_CLOSED = 0x00,

  W5100_SOCKET_STATE_UDP = 0x22,
} w5100_socket_state;

enum w5100_socket_command {
  W5100_SOCKET_COMMAND_OPEN = 0x01,
  W5100_SOCKET_COMMAND_SEND = 0x20,
};

typedef struct nic_w5100_socket_t {
  w5100_socket_mode mode;
  w5100_socket_state state;

  libspectrum_byte ir;      /* Interrupt register */

  libspectrum_byte port[2]; /* Source port */

  libspectrum_byte dip[4];  /* Destination IP address */
  libspectrum_byte dport[2];/* Destination port */

  libspectrum_word tx_rr;   /* Transmit read pointer */
  libspectrum_word tx_wr;   /* Transmit write pointer */

  int id; /* For debug use only. Remove when no longer needed */
} nic_w5100_socket_t;

struct nic_w5100_t {
  libspectrum_byte gw[4];   /* Gateway IP address */
  libspectrum_byte sub[4];  /* Our subnet mask */
  libspectrum_byte sha[6];  /* MAC address */
  libspectrum_byte sip[4];  /* Our IP address */

  nic_w5100_socket_t socket[4];
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

static libspectrum_word
w5100_socket_fsr( nic_w5100_socket_t *socket )
{
  libspectrum_word fsr = socket->tx_rr - socket->tx_wr;

  if( fsr <= 0 )
    fsr += 0x0800;  /* We support only 2Kb buffer size */

  return fsr;
}

libspectrum_byte
nic_w5100_read( nic_w5100_t *self, libspectrum_word reg )
{
  libspectrum_byte b;

  if( reg < 0x030 ) {
    switch( reg ) {
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
        break;
      default:
        b = 0xff;
        printf("w5100: reading 0x%02x from unsupported register 0x%03x\n", b, reg );
        break;
    }
  }
  else if( reg >= 0x400 && reg < 0x800 ) {
    nic_w5100_socket_t *socket = &self->socket[(reg >> 8) - 4];
    int socket_reg = reg & 0xff;
    int reg_offset;
    switch( socket_reg ) {
      case W5100_SOCKET_IR:
        b = socket->ir;
        printf("w5100: reading 0x%02x from S%d_IR\n", b, socket->id);
        break;
      case W5100_SOCKET_SR:
        b = socket->state;
        printf("w5100: reading 0x%02x from S%d_SR\n", b, socket->id);
        break;
      case W5100_SOCKET_PORT0: case W5100_SOCKET_PORT1:
        b = socket->port[socket_reg - W5100_SOCKET_PORT0];
        printf("w5100: reading 0x%02x from S%d_PORT%d\n", b, socket->id, socket_reg - W5100_SOCKET_PORT0);
        break;
      case W5100_SOCKET_TX_FSR0: case W5100_SOCKET_TX_FSR1:
        reg_offset = socket_reg - W5100_SOCKET_TX_FSR0;
        b = ( w5100_socket_fsr( socket ) >> ( 8 * ( 1 - reg_offset ) ) ) & 0xff;
        printf("w5100: reading 0x%02x from S%d_TX_FSR%d\n", b, socket->id, reg_offset);
        break;
      case W5100_SOCKET_TX_RR0: case W5100_SOCKET_TX_RR1:
        reg_offset = socket_reg - W5100_SOCKET_TX_RR0;
        b = ( socket->tx_rr >> ( 8 * ( 1 - reg_offset ) ) ) & 0xff;
        printf("w5100: reading 0x%02x from S%d_TX_RR%d\n", b, socket->id, reg_offset);
        break;
      case W5100_SOCKET_TX_WR0: case W5100_SOCKET_TX_WR1:
        reg_offset = socket_reg - W5100_SOCKET_TX_WR0;
        b = ( socket->tx_wr >> ( 8 * ( 1 - reg_offset ) ) ) & 0xff;
        printf("w5100: reading 0x%02x from S%d_TX_WR%d\n", b, socket->id, reg_offset);
        break;
      default:
        b = 0xff;
        printf("w5100: reading 0x%02x from unsupported register 0x%03x\n", b, reg );
        break;
    }
  }
  else {
    b = 0xff;
    printf("w5100: reading 0x%02x from unsupported register 0x%03x\n", b, reg );
  }


  return b;
}

static void
w5100_reset( nic_w5100_t *self )
{
  size_t i;

  printf("w5100: reset\n");

  memset( self->gw, 0, sizeof( self->gw ) );
  memset( self->sub, 0, sizeof( self->sub ) );
  memset( self->sha, 0, sizeof( self->sha ) );
  memset( self->sip, 0, sizeof( self->sip ) );

  for( i = 0; i < 4; i++ ) {
    nic_w5100_socket_t *socket = &self->socket[i];
    socket->id = i; /* Debug use only */
    socket->mode = W5100_SOCKET_MODE_CLOSED;
    socket->state = W5100_SOCKET_STATE_CLOSED;
  }
}

static void
w5100_socket_reset( nic_w5100_socket_t *socket )
{
  socket->ir = 0;
  memset( socket->port, 0, sizeof( socket->port ) );
  memset( socket->dip, 0, sizeof( socket->dip ) );
  memset( socket->dport, 0, sizeof( socket->dport ) );
  socket->tx_rr = socket->tx_wr = 0;
}

static void
w5100_write_mr( nic_w5100_t *self, libspectrum_byte b )
{
  printf("w5100: writing 0x%02x to MR\n", b);

  if( b & 0x80 )
    w5100_reset( self );

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

static void
w5100_write_socket_mr( nic_w5100_socket_t *socket, libspectrum_byte b )
{
  printf("w5100: writing 0x%02x to S%d_MR\n", b, socket->id);

  if( b <= 0x02 )
    socket->mode = b;
  else {
    socket->mode = W5100_SOCKET_MODE_CLOSED;
    printf("w5100: unsupported value 0x%02x written to S%d_MR\n", b, socket->id);
  }
}

static void
w5100_socket_open( nic_w5100_socket_t *socket )
{
  if( socket->mode == W5100_SOCKET_MODE_UDP &&
    socket->state == W5100_SOCKET_STATE_CLOSED ) {
    socket->state = W5100_SOCKET_STATE_UDP;
    w5100_socket_reset( socket );
  }
}

static void
w5100_socket_send( nic_w5100_socket_t *socket )
{
  if( socket->mode == W5100_SOCKET_MODE_UDP &&
    socket->state == W5100_SOCKET_STATE_UDP ) {
    libspectrum_word length = socket->tx_wr - socket->tx_rr;
    printf("w5100: sending 0x%03x bytes via UDP on socket %d\n", length, socket->id);
    socket->tx_rr = socket->tx_wr;
  }
}

static void
w5100_write_socket_cr( nic_w5100_socket_t *socket, libspectrum_byte b )
{
  printf("w5100: writing 0x%02x to S%d_CR\n", b, socket->id);

  switch( b ) {
    case W5100_SOCKET_COMMAND_OPEN:
      w5100_socket_open( socket );
      break;
    case W5100_SOCKET_COMMAND_SEND:
      w5100_socket_send( socket );
      break;
  }
}

void
nic_w5100_write( nic_w5100_t *self, libspectrum_word reg, libspectrum_byte b )
{
  if( reg < 0x030 ) {
    switch( reg ) {
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
        printf("w5100: writing 0x%02x to unsupported register 0x%03x\n", b, reg);
        break;
    }
  }
  else if( reg >= 0x400 && reg < 0x800 ) {
    nic_w5100_socket_t *socket = &self->socket[(reg >> 8) - 4];
    int socket_reg = reg & 0xff;
    switch( socket_reg ) {
      case W5100_SOCKET_MR:
        w5100_write_socket_mr( socket, b );
        break;
      case W5100_SOCKET_CR:
        w5100_write_socket_cr( socket, b );
        break;
      case W5100_SOCKET_IR:
        printf("w5100: writing 0x%02x to S%d_IR\n", b, socket->id);
        socket->ir &= ~b;
        break;
      case W5100_SOCKET_PORT0: case W5100_SOCKET_PORT1:
        printf("w5100: writing 0x%02x to S%d_PORT%d\n", b, socket->id, socket_reg - W5100_SOCKET_PORT0);
        socket->port[socket_reg - W5100_SOCKET_PORT0] = b;
        break;
      case W5100_SOCKET_DIPR0: case W5100_SOCKET_DIPR1: case W5100_SOCKET_DIPR2: case W5100_SOCKET_DIPR3:
        printf("w5100: writing 0x%02x to S%d_DIPR%d\n", b, socket->id, socket_reg - W5100_SOCKET_DIPR0);
        socket->dip[socket_reg - W5100_SOCKET_DIPR0] = b;
        break;
      case W5100_SOCKET_DPORT0: case W5100_SOCKET_DPORT1:
        printf("w5100: writing 0x%02x to S%d_DPORT%d\n", b, socket->id, socket_reg - W5100_SOCKET_DPORT0);
        socket->dport[socket_reg - W5100_SOCKET_DPORT0] = b;
        break;
      case W5100_SOCKET_TX_WR0:
        printf("w5100: writing 0x%02x to S%d_WR0\n", b, socket->id);
        socket->tx_wr = (socket->tx_wr & 0xff) | (b << 8);
        break;
      case W5100_SOCKET_TX_WR1:
        printf("w5100: writing 0x%02x to S%d_WR1\n", b, socket->id);
        socket->tx_wr = (socket->tx_wr & 0xff00) | b;
        break;
      default:
        printf("w5100: writing 0x%02x to unsupported register 0x%03x\n", b, reg);
        break;
    }
  }
  else
    printf("w5100: writing 0x%02x to unsupported register 0x%03x\n", b, reg);
}
