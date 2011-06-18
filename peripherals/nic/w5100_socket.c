/* w5100_socket.c: Wiznet W5100 emulation - sockets code
   
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

#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "fuse.h"
#include "ui/ui.h"
#include "w5100.h"
#include "w5100_internals.h"

enum w5100_socket_command {
  W5100_SOCKET_COMMAND_OPEN = 0x01,
  W5100_SOCKET_COMMAND_CLOSE = 0x10,
  W5100_SOCKET_COMMAND_SEND = 0x20,
  W5100_SOCKET_COMMAND_RECV = 0x40,
};

void
nic_w5100_socket_init( nic_w5100_socket_t *socket, int which )
{
  socket->id = which;
  socket->mode = W5100_SOCKET_MODE_CLOSED;
  socket->state = W5100_SOCKET_STATE_CLOSED;
  socket->fd = -1;
  pthread_mutex_init( &socket->lock, NULL );
}

static void
w5100_socket_acquire_lock( nic_w5100_socket_t *socket )
{
  int error = pthread_mutex_lock( &socket->lock );
  if( error ) {
    ui_error( UI_ERROR_ERROR, "%s:%d: error %d locking mutex for socket %d\n",
      __FILE__, __LINE__, error, socket->id );
    fuse_abort();
  }
}

static void
w5100_socket_release_lock( nic_w5100_socket_t *socket )
{
  int error = pthread_mutex_unlock( &socket->lock );
  if( error ) {
    ui_error( UI_ERROR_ERROR, "%s:%d: error %d unlocking mutex for socket %d\n",
      __FILE__, __LINE__, error, socket->id );
    fuse_abort();
  }
}

void
nic_w5100_socket_reset( nic_w5100_socket_t *socket )
{
  socket->ir = 0;
  memset( socket->port, 0, sizeof( socket->port ) );
  memset( socket->dip, 0, sizeof( socket->dip ) );
  memset( socket->dport, 0, sizeof( socket->dport ) );
  socket->tx_rr = socket->tx_wr = 0;
  socket->rx_rsr = 0;
  socket->old_rx_rd = socket->rx_rd = 0;

  nic_w5100_socket_free( socket );
}

void
nic_w5100_socket_free( nic_w5100_socket_t *socket )
{
  if( socket->fd != -1 ) {
    w5100_socket_acquire_lock( socket );
    close( socket->fd );
    socket->fd = -1;
    socket->ok_for_io = 0;
    socket->write_pending = 0;
    w5100_socket_release_lock( socket );
  }
}

static void
w5100_write_socket_mr( nic_w5100_socket_t *socket, libspectrum_byte b )
{
  printf("w5100: writing 0x%02x to S%d_MR\n", b, socket->id);

  w5100_socket_mode mode = b & 0x0f;
  libspectrum_byte flags = b & 0xf0;

  switch( mode ) {
    case W5100_SOCKET_MODE_CLOSED:
      socket->mode = mode;
      break;
    case W5100_SOCKET_MODE_TCP:
      /* We support only "disable no delayed ACK" */
      if( flags != 0x20 )
        printf("w5100: unsupported flags 0x%02x set for TCP mode on socket %d\n", b & 0xf0, socket->id);
      socket->mode = mode;
      break;
    case W5100_SOCKET_MODE_UDP:
      /* We don't support multicast */
      if( flags != 0x00 )
        printf("w5100: unsupported flags 0x%02x set for UDP mode on socket %d\n", b & 0xf0, socket->id);
      socket->mode = mode;
      break;
    case W5100_SOCKET_MODE_IPRAW:
    case W5100_SOCKET_MODE_MACRAW:
    case W5100_SOCKET_MODE_PPPOE:
    default:
      printf("w5100: unsupported mode 0x%02x set on socket %d\n", b, socket->id);
      socket->mode = W5100_SOCKET_MODE_CLOSED;
      break;
  }
}

static void
w5100_socket_open( nic_w5100_socket_t *socket_obj )
{
  if( socket_obj->mode == W5100_SOCKET_MODE_UDP &&
    socket_obj->state == W5100_SOCKET_STATE_CLOSED ) {
    nic_w5100_socket_reset( socket_obj );

    w5100_socket_acquire_lock( socket_obj );
    socket_obj->fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( socket_obj->fd == -1 ) {
      printf("w5100: failed to open UDP socket for socket %d; errno = %d\n", socket_obj->id, errno);
      w5100_socket_release_lock( socket_obj );
      return;
    }
    w5100_socket_release_lock( socket_obj );

    socket_obj->state = W5100_SOCKET_STATE_UDP;
    printf("w5100: opened UDP fd %d for socket %d\n", socket_obj->fd, socket_obj->id);
  }
  else if( socket_obj->mode == W5100_SOCKET_MODE_TCP &&
    socket_obj->state == W5100_SOCKET_STATE_CLOSED ) {
    nic_w5100_socket_reset( socket_obj );

    w5100_socket_acquire_lock( socket_obj );
    socket_obj->fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if( socket_obj->fd == -1 ) {
      printf("w5100: failed to open TCP socket for socket %d; errno = %d\n", socket_obj->id, errno);
      w5100_socket_release_lock( socket_obj );
      return;
    }
    w5100_socket_release_lock( socket_obj );

    socket_obj->state = W5100_SOCKET_STATE_INIT;
    printf("w5100: opened TCP fd %d for socket %d\n", socket_obj->fd, socket_obj->id);
  }
}

static void
w5100_socket_bind_port( nic_w5100_t *self, nic_w5100_socket_t *socket )
{
  struct sockaddr_in sa;

  sa.sin_family = AF_INET;
  memcpy( &sa.sin_port, socket->port, 2 );
  memcpy( &sa.sin_addr.s_addr, self->sip, 4 );

  w5100_socket_acquire_lock( socket );
  if( bind( socket->fd, (struct sockaddr*)&sa, sizeof(sa) ) == -1 ) {
    printf("w5100: failed to bind socket %d; errno = %d\n", socket->id, errno);
    w5100_socket_release_lock( socket );
    return;
  }
  w5100_socket_release_lock( socket );

  nic_w5100_wake_io_thread( self );

  printf("w5100: successfully bound socket %d\n", socket->id);
}

static void
w5100_socket_close( nic_w5100_t *self, nic_w5100_socket_t *socket )
{
  if( socket->mode == W5100_SOCKET_MODE_UDP &&
    socket->state == W5100_SOCKET_STATE_UDP ) {
    w5100_socket_acquire_lock( socket );
    close( socket->fd );
    socket->fd = -1;
    socket->ok_for_io = 0;
    socket->state = W5100_SOCKET_STATE_CLOSED;
    w5100_socket_release_lock( socket );
    nic_w5100_wake_io_thread( self );
  }
}

static void
w5100_socket_send( nic_w5100_t *self, nic_w5100_socket_t *socket )
{
  if( socket->mode == W5100_SOCKET_MODE_UDP &&
    socket->state == W5100_SOCKET_STATE_UDP ) {
    w5100_socket_acquire_lock( socket );
    socket->write_pending = 1;
    w5100_socket_release_lock( socket );
    nic_w5100_wake_io_thread( self );
  }
}

static void
w5100_socket_recv( nic_w5100_t *self, nic_w5100_socket_t *socket )
{
  if( socket->mode == W5100_SOCKET_MODE_UDP &&
    socket->state == W5100_SOCKET_STATE_UDP ) {
    w5100_socket_acquire_lock( socket );
    socket->rx_rsr -= socket->rx_rd - socket->old_rx_rd;
    socket->old_rx_rd = socket->rx_rd;
    if( socket->rx_rsr != 0 )
      socket->ir |= 1 << 2;
    w5100_socket_release_lock( socket );
    nic_w5100_wake_io_thread( self );
  }
}

static void
w5100_write_socket_cr( nic_w5100_t *self, nic_w5100_socket_t *socket, libspectrum_byte b )
{
  printf("w5100: writing 0x%02x to S%d_CR\n", b, socket->id);

  switch( b ) {
    case W5100_SOCKET_COMMAND_OPEN:
      w5100_socket_open( socket );
      break;
    case W5100_SOCKET_COMMAND_CLOSE:
      w5100_socket_close( self, socket );
      break;
    case W5100_SOCKET_COMMAND_SEND:
      w5100_socket_send( self, socket );
      break;
    case W5100_SOCKET_COMMAND_RECV:
      w5100_socket_recv( self, socket );
      break;
    default:
      printf("w5100: unknown command 0x%02x sent to socket %d\n", b, socket->id);
      break;
  }
}

libspectrum_byte
nic_w5100_socket_read( nic_w5100_t *self, libspectrum_word reg )
{
  nic_w5100_socket_t *socket = &self->socket[(reg >> 8) - 4];
  int socket_reg = reg & 0xff;
  int reg_offset;
  libspectrum_word fsr;
  libspectrum_byte b;
  switch( socket_reg ) {
    case W5100_SOCKET_MR:
      b = socket->mode;
      printf("w5100: reading 0x%02x from S%d_MR\n", b, socket->id);
      break;
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
      fsr = 0x0800 - (socket->tx_wr - socket->tx_rr);
      b = ( fsr >> ( 8 * ( 1 - reg_offset ) ) ) & 0xff;
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
    case W5100_SOCKET_RX_RSR0: case W5100_SOCKET_RX_RSR1:
      reg_offset = socket_reg - W5100_SOCKET_RX_RSR0;
      b = ( socket->rx_rsr >> ( 8 * ( 1 - reg_offset ) ) ) & 0xff;
      printf("w5100: reading 0x%02x from S%d_RX_RSR%d\n", b, socket->id, reg_offset);
      break;
    case W5100_SOCKET_RX_RD0: case W5100_SOCKET_RX_RD1:
      reg_offset = socket_reg - W5100_SOCKET_RX_RD0;
      b = ( socket->rx_rd >> ( 8 * ( 1 - reg_offset ) ) ) & 0xff;
      printf("w5100: reading 0x%02x from S%d_RX_RD%d\n", b, socket->id, reg_offset);
      break;
    default:
      b = 0xff;
      printf("w5100: reading 0x%02x from unsupported register 0x%03x\n", b, reg );
      break;
  }
  return b;
}

void
nic_w5100_socket_write( nic_w5100_t *self, libspectrum_word reg, libspectrum_byte b )
{
  nic_w5100_socket_t *socket = &self->socket[(reg >> 8) - 4];
  int socket_reg = reg & 0xff;
  switch( socket_reg ) {
    case W5100_SOCKET_MR:
      w5100_write_socket_mr( socket, b );
      break;
    case W5100_SOCKET_CR:
      w5100_write_socket_cr( self, socket, b );
      break;
    case W5100_SOCKET_IR:
      printf("w5100: writing 0x%02x to S%d_IR\n", b, socket->id);
      socket->ir &= ~b;
      break;
    case W5100_SOCKET_PORT0: case W5100_SOCKET_PORT1:
      printf("w5100: writing 0x%02x to S%d_PORT%d\n", b, socket->id, socket_reg - W5100_SOCKET_PORT0);
      socket->port[socket_reg - W5100_SOCKET_PORT0] = b;
      if( socket_reg == W5100_SOCKET_PORT0 )
        w5100_socket_bind_port( self, socket );
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
    case W5100_SOCKET_RX_RD0:
      printf("w5100: writing 0x%02x to S%d_RX_RD0\n", b, socket->id);
      socket->rx_rd = (socket->rx_rd & 0xff) | (b << 8);
      break;
    case W5100_SOCKET_RX_RD1:
      printf("w5100: writing 0x%02x to S%d_RX_RD1\n", b, socket->id);
      socket->rx_rd = (socket->rx_rd & 0xff00) | b;
      break;
    default:
      printf("w5100: writing 0x%02x to unsupported register 0x%03x\n", b, reg);
      break;
  }
}

libspectrum_byte
nic_w5100_socket_read_rx_buffer( nic_w5100_t *self, libspectrum_word reg )
{
  nic_w5100_socket_t *socket = &self->socket[(reg - 0x6000) / 0x0800];
  int offset = reg & 0x7ff;
  libspectrum_byte b = socket->rx_buffer[offset];
  printf("w5100: reading 0x%02x from socket %d rx buffer offset 0x%03x\n", b, socket->id, offset);
  return b;
}

void
nic_w5100_socket_write_tx_buffer( nic_w5100_t *self, libspectrum_word reg, libspectrum_byte b )
{
  nic_w5100_socket_t *socket = &self->socket[(reg - 0x4000) / 0x0800];
  int offset = reg & 0x7ff;
  printf("w5100: writing 0x%02x to socket %d tx buffer offset 0x%03x\n", b, socket->id, offset);
  socket->tx_buffer[offset] = b;
}

void
nic_w5100_socket_add_to_sets( nic_w5100_socket_t *socket, fd_set *readfds,
  fd_set *writefds, int *max_fd )
{
  w5100_socket_acquire_lock( socket );

  if( socket->fd != -1 ) {
    socket->ok_for_io = 1;

    /* We can process a UDP read if we're in a UDP state and there are at least
       9 bytes free in our buffer (8 byte UDP header and 1 byte of actual
       data). */
    int udp_read = socket->state == W5100_SOCKET_STATE_UDP &&
      0x800 - socket->rx_rsr >= 9;

    if( udp_read ) {
      FD_SET( socket->fd, readfds );
      if( socket->fd > *max_fd )
        *max_fd = socket->fd;
      printf("w5100: checking for read on socket %d with fd %d; max fd %d\n", socket->id, socket->fd, *max_fd);
    }

    if( socket->write_pending ) {
      FD_SET( socket->fd, writefds );
      if( socket->fd > *max_fd )
        *max_fd = socket->fd;
      printf("w5100: write pending on socket %d with fd %d; max fd %d\n", socket->id, socket->fd, *max_fd);
    }
  }

  w5100_socket_release_lock( socket );
}

static void
w5100_socket_process_read( nic_w5100_socket_t *socket )
{
  libspectrum_byte buffer[0x800];
  int bytes_free = 0x800 - socket->rx_rsr;
  ssize_t bytes_read;
  struct sockaddr_in sa;
  socklen_t sa_length = sizeof(sa);

  printf("w5100: reading from socket %d\n", socket->id);

  sa.sin_family = AF_INET;
  memcpy( &sa.sin_port, socket->port, 2 );
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  bytes_read = recvfrom( socket->fd, buffer + 8, bytes_free - 8, 0, (struct sockaddr*)&sa, &sa_length );
  printf("w5100: read 0x%03x bytes from socket %d\n", (int)bytes_read, socket->id);

  if( bytes_read != -1 ) {
    int offset = (socket->old_rx_rd + socket->rx_rsr) & 0x7ff;
    libspectrum_byte *dest = &socket->rx_buffer[offset];

    /* Add the W5100's UDP header */
    memcpy( buffer, &sa.sin_addr.s_addr, 4 );
    memcpy( buffer + 4, &sa.sin_port, 2 );
    buffer[6] = (bytes_read >> 8) & 0xff;
    buffer[7] = bytes_read & 0xff;

    bytes_read += 8;

    socket->rx_rsr += bytes_read;
    socket->ir |= 1 << 2;

    if( offset + bytes_read <= 0x800 ) {
      memcpy( dest, buffer, bytes_read );
    }
    else {
      int first_chunk = 0x800 - offset;
      memcpy( dest, buffer, first_chunk );
      memcpy( socket->rx_buffer, buffer + first_chunk, bytes_read - first_chunk );
    }
  }
  else {
    printf("w5100: error %d reading from socket %d\n", errno, socket->id);
  }
}

static void
w5100_socket_process_write( nic_w5100_socket_t *socket )
{
  struct sockaddr_in sa;
  ssize_t bytes_sent;
  int offset = socket->tx_rr & 0x7ff;
  libspectrum_word length = socket->tx_wr - socket->tx_rr;
  libspectrum_byte *data = &socket->tx_buffer[ offset ];

  /* If the data wraps round the write buffer, write it in two chunks */
  if( offset + length > 0x800 )
    length = 0x800 - offset;

  printf("w5100: writing to socket %d\n", socket->id);

  sa.sin_family = AF_INET;
  memcpy( &sa.sin_port, socket->dport, 2 );
  memcpy( &sa.sin_addr.s_addr, socket->dip, 4 );

  bytes_sent = sendto( socket->fd, data, length, 0, (struct sockaddr*)&sa, sizeof(sa) );
  printf("w5100: sent 0x%03x bytes of %d to socket %d\n", (int)bytes_sent, length, socket->id);

  if( bytes_sent != -1 ) {
    socket->tx_rr += bytes_sent;
    if( socket->tx_rr == socket->tx_wr ) {
      socket->write_pending = 0;
      socket->ir |= 1 << 4;
    }
  }
  else
    printf("w5100: error %d writing to socket %d\n", errno, socket->id);
}

void
nic_w5100_socket_process_io( nic_w5100_socket_t *socket, fd_set readfds,
  fd_set writefds )
{
  w5100_socket_acquire_lock( socket );

  /* Process only if we're an open socket, and we haven't been closed and
     re-opened since the select() started */
  if( socket->fd != -1 && socket->ok_for_io ) {
    if( FD_ISSET( socket->fd, &readfds ) )
      w5100_socket_process_read( socket );

    if( FD_ISSET( socket->fd, &writefds ) )
      w5100_socket_process_write( socket );
  }

  w5100_socket_release_lock( socket );
}
