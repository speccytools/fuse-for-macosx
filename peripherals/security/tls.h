/* tls.h: TLS/SSL socket support using mbedTLS
   
   Provides TLS/SSL support for W5100 socket emulation.

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

*/

#ifndef FUSE_TLS_H
#define FUSE_TLS_H

#include "compat.h"

#include <mbedtls/ssl.h>
#include <mbedtls/error.h>

typedef struct tls_socket_t {
  mbedtls_ssl_context ssl;           /* SSL context */
  mbedtls_ssl_config conf;            /* SSL configuration */
  compat_socket_t fd;                 /* Underlying POSIX socket */
  int handshake_complete;             /* Flag for handshake status */
  int has_pending_data;               /* Flag indicating more data available after read */
} tls_socket_t;

/* Allocate and initialize a TLS socket from a POSIX socket */
/* hostname can be NULL, IP address string, or hostname for SNI */
tls_socket_t* tls_socket_alloc( compat_socket_t fd, const char *hostname );

/* Free TLS socket and cleanup mbedTLS contexts */
void tls_socket_free( tls_socket_t *tls );

/* Perform blocking TLS handshake (called after connect) */
int tls_connect( tls_socket_t *tls );

/* Read from TLS socket, sets has_pending_data if more data available */
ssize_t tls_read( tls_socket_t *tls, void *buf, size_t len );

/* Write to TLS socket */
ssize_t tls_write( tls_socket_t *tls, const void *buf, size_t len );

/* Close TLS connection gracefully */
void tls_close( tls_socket_t *tls );

#endif                          /* #ifndef FUSE_TLS_H */

