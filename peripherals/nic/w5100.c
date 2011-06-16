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

#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

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

  W5100_SOCKET_RX_RSR0,
  W5100_SOCKET_RX_RSR1,
  W5100_SOCKET_RX_RD0,
  W5100_SOCKET_RX_RD1,
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
  W5100_SOCKET_COMMAND_CLOSE = 0x10,
  W5100_SOCKET_COMMAND_SEND = 0x20,
  W5100_SOCKET_COMMAND_RECV = 0x40,
};

typedef struct nic_w5100_socket_t {

  /* W5100 properties */

  w5100_socket_mode mode;
  w5100_socket_state state;

  libspectrum_byte ir;      /* Interrupt register */

  libspectrum_byte port[2]; /* Source port */

  libspectrum_byte dip[4];  /* Destination IP address */
  libspectrum_byte dport[2];/* Destination port */

  libspectrum_word tx_rr;   /* Transmit read pointer */
  libspectrum_word tx_wr;   /* Transmit write pointer */

  libspectrum_word rx_rsr;  /* Received size */
  libspectrum_word rx_rd;   /* Received read pointer */

  libspectrum_word old_rx_rd; /* Used in RECV command processing */

  libspectrum_byte tx_buffer[0x800];  /* Transmit buffer */
  libspectrum_byte rx_buffer[0x800];  /* Received buffer */

  /* Host properties */

  int fd;                   /* Socket file descriptor */
  int io_fd;                /* Temporary copy of fd used during IO */
  int write_pending;        /* True if we're waiting to write data on this socket */

  pthread_mutex_t lock;     /* Mutex for this socket */

  int id; /* For debug use only. Remove when no longer needed */
} nic_w5100_socket_t;

struct nic_w5100_t {
  libspectrum_byte gw[4];   /* Gateway IP address */
  libspectrum_byte sub[4];  /* Our subnet mask */
  libspectrum_byte sha[6];  /* MAC address */
  libspectrum_byte sip[4];  /* Our IP address */

  nic_w5100_socket_t socket[4];

  pthread_t thread;         /* Thread for doing I/O */
  sig_atomic_t stop_io_thread; /* Flag to stop I/O thread */
  int pipe_read;            /* Pipe for waking I/O thread */
  int pipe_write;
};

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

    w5100_socket_acquire_lock( socket );
    socket->fd = -1;
    socket->write_pending = 0;
    w5100_socket_release_lock( socket );
  }
}

static void*
w5100_io_thread( void *arg )
{
  nic_w5100_t *self = arg;
  int i;

  while( !self->stop_io_thread ) {
    fd_set readfds, writefds;
    int max_fd = self->pipe_read;

    FD_ZERO( &readfds );
    FD_SET( self->pipe_read, &readfds );

    FD_ZERO( &writefds );

    for( i = 0; i < 4; i++ ) {
      nic_w5100_socket_t *socket = &self->socket[i];

      w5100_socket_acquire_lock( socket );

      socket->io_fd = socket->fd;

      if( socket->fd != -1 ) {
        FD_SET( socket->fd, &readfds );
        if( socket->fd > max_fd )
          max_fd = socket->fd;
        printf("w5100: checking for read on socket %d with fd %d; max fd %d\n", socket->id, socket->fd, max_fd);

        if( socket->write_pending ) {
          FD_SET( socket->fd, &writefds );
          printf("w5100: write pending on socket %d with fd %d; max fd %d\n", socket->id, socket->fd, max_fd);
        }
      }

      w5100_socket_release_lock( socket );
    }

    printf("w5100: io thread select\n");

    select( max_fd + 1, &readfds, &writefds, NULL, NULL );

    printf("w5100: io thread wake\n");

    if( FD_ISSET( self->pipe_read, &readfds ) ) {
      char bitbucket;
      printf("w5100: discarding pipe data\n");
      read( self->pipe_read, &bitbucket, 1 );
    }

    for( i = 0; i < 4; i++ ) {
      nic_w5100_socket_t *socket = &self->socket[i];

      w5100_socket_acquire_lock( socket );

      /* If our current fd has changed since we started the select(),
         skip this */
      if( socket->fd != socket->io_fd ) {
        w5100_socket_release_lock( socket );
        continue;
      }

      if( FD_ISSET( socket->fd, &readfds ) ) {
        libspectrum_byte buffer[0x800];
        ssize_t bytes_read;
        struct sockaddr_in sa;
        socklen_t sa_length = sizeof(sa);

        printf("w5100: reading from socket %d\n", socket->id);

        sa.sin_family = AF_INET;
        memcpy( &sa.sin_port, socket->port, 2 );
        sa.sin_addr.s_addr = htonl(INADDR_ANY);
        bytes_read = recvfrom( socket->fd, buffer + 8, 0x800 - 8, 0, (struct sockaddr*)&sa, &sa_length );
        printf("w5100: read 0x%03x bytes from socket %d\n", (int)bytes_read, socket->id);

        if( bytes_read != -1 ) {
          int offset = socket->rx_rd & 0x7ff;
          libspectrum_byte *dest = &socket->rx_buffer[offset];

          /* Add the W5100's UDP header */
          memcpy( buffer, &sa.sin_addr.s_addr, 4 );
          memcpy( buffer + 4, &sa.sin_port, 2 );
          buffer[6] = (bytes_read >> 8) & 0xff;
          buffer[7] = bytes_read & 0xff;

          bytes_read += 8;

          socket->rx_rsr = bytes_read;
          socket->old_rx_rd = socket->rx_rd;
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

      if( FD_ISSET( socket->fd, &writefds ) ) {
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

      w5100_socket_release_lock( socket );
    }
  }
  return NULL;
}

static void
w5100_wake_io_thread( nic_w5100_t *self )
{
  const char dummy = 0;
  write( self->pipe_write, &dummy, 1 );
}

nic_w5100_t*
nic_w5100_alloc( void )
{
  int error;
  int pipefd[2];
  int i;

  nic_w5100_t *self = malloc( sizeof( *self ) );
  if( !self ) {
    ui_error( UI_ERROR_ERROR, "%s:%d out of memory", __FILE__, __LINE__ );
    fuse_abort();
  }

  for( i = 0; i < 4; i++ ) {
    nic_w5100_socket_t *socket = &self->socket[i];
    pthread_mutex_init( &socket->lock, NULL );
  }

  w5100_reset( self );

  error = pipe( pipefd );
  if( error ) {
    ui_error( UI_ERROR_ERROR, "w5100: error %d creating pipe", error );
    fuse_abort();
  }

  self->pipe_read = pipefd[0];
  self->pipe_write = pipefd[1];

  self->stop_io_thread = 0;

  error = pthread_create( &self->thread, NULL, w5100_io_thread, self );
  if( error ) {
    ui_error( UI_ERROR_ERROR, "w5100: error %d creating thread", error );
    fuse_abort();
  }

  return self;
}

static void
w5100_socket_free( nic_w5100_socket_t *socket )
{
  if( socket->fd != -1 ) {
    w5100_socket_acquire_lock( socket );
    close( socket->fd );
    socket->fd = -1;
    socket->write_pending = 0;
    w5100_socket_release_lock( socket );
  }
}

void
nic_w5100_free( nic_w5100_t *self )
{
  int i;

  self->stop_io_thread = 1;
  w5100_wake_io_thread( self );

  pthread_join( self->thread, NULL );

  for( i = 0; i < 4; i++ )
    w5100_socket_free( &self->socket[i] );
    
  free( self );
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
    libspectrum_word fsr;
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
  }
  else if( reg >= 0x6000 && reg < 0x8000 ) {
    nic_w5100_socket_t *socket = &self->socket[(reg - 0x6000) / 0x0800];
    int offset = reg & 0x7ff;
    b = socket->rx_buffer[offset];
    printf("w5100: reading 0x%02x from socket %d rx buffer offset 0x%03x\n", b, socket->id, offset);
  }
  else {
    b = 0xff;
    printf("w5100: reading 0x%02x from unsupported register 0x%03x\n", b, reg );
  }


  return b;
}

static void
w5100_socket_reset( nic_w5100_socket_t *socket )
{
  socket->ir = 0;
  memset( socket->port, 0, sizeof( socket->port ) );
  memset( socket->dip, 0, sizeof( socket->dip ) );
  memset( socket->dport, 0, sizeof( socket->dport ) );
  socket->tx_rr = socket->tx_wr = 0;
  socket->rx_rsr = 0;
  socket->rx_rd = 0;

  if( socket->fd != -1 ) {
    w5100_socket_acquire_lock( socket );
    close( socket->fd );
    socket->fd = -1;
    socket->write_pending = 0;
    w5100_socket_release_lock( socket );
  }
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
w5100_socket_open( nic_w5100_socket_t *socket_obj )
{
  if( socket_obj->mode == W5100_SOCKET_MODE_UDP &&
    socket_obj->state == W5100_SOCKET_STATE_CLOSED ) {

    socket_obj->state = W5100_SOCKET_STATE_UDP;
    w5100_socket_reset( socket_obj );

    w5100_socket_acquire_lock( socket_obj );
    socket_obj->fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( socket_obj->fd == -1 ) {
      printf("w5100: failed to open UDP socket for socket %d; errno = %d\n", socket_obj->id, errno);
      w5100_socket_release_lock( socket_obj );
      return;
    }
    w5100_socket_release_lock( socket_obj );

    printf("w5100: opened fd %d for socket %d\n", socket_obj->fd, socket_obj->id);
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

  w5100_wake_io_thread( self );

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
    socket->state = W5100_SOCKET_STATE_CLOSED;
    w5100_socket_release_lock( socket );
    w5100_wake_io_thread( self );
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
    w5100_wake_io_thread( self );
  }
}

static void
w5100_socket_recv( nic_w5100_socket_t *socket )
{
  if( socket->mode == W5100_SOCKET_MODE_UDP &&
    socket->state == W5100_SOCKET_STATE_UDP ) {
    socket->rx_rsr -= socket->rx_rd - socket->old_rx_rd;
    if( socket->rx_rsr != 0 )
      socket->ir |= 1 << 2;
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
      w5100_socket_recv( socket );
      break;
    default:
      printf("w5100: unknown command 0x%02x sent to socket %d\n", b, socket->id);
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
  else if( reg >= 0x4000 && reg < 0x6000 ) {
    nic_w5100_socket_t *socket = &self->socket[(reg - 0x4000) / 0x0800];
    int offset = reg & 0x7ff;
    printf("w5100: writing 0x%02x to socket %d tx buffer offset 0x%03x\n", b, socket->id, offset);
    socket->tx_buffer[offset] = b;
  }
  else
    printf("w5100: writing 0x%02x to unsupported register 0x%03x\n", b, reg);
}
