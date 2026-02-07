#include "config.h"

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <stdlib.h> /* For arc4random_buf */
#else
#include <sys/random.h>
#endif

#ifndef WIN32
#include <sys/select.h>
#endif

#include "mbedtls_config.h"

#include <mbedtls/ssl.h>
#include <mbedtls/error.h>
#include <mbedtls/debug.h>

#include "libspectrum.h"
#include "fuse.h"
#include "ui/ui.h"
#include "compat.h"
#include "tls.h"

/* Custom BIO read callback */
static int
tls_bio_read( void *ctx, unsigned char *buf, size_t len )
{
  compat_socket_t fd = *(compat_socket_t*)ctx;
  ssize_t ret;

  if( len > INT_MAX )
    len = INT_MAX;

  ret = recv( fd, (char*)buf, len, 0 );
  
  if( ret < 0 ) {
    if( compat_socket_get_error() == COMPAT_EWOULDBLOCK ||
        compat_socket_get_error() == COMPAT_EINPROGRESS )
      return MBEDTLS_ERR_SSL_WANT_READ;
    return -1; /* Network receive error */
  }

  if( ret == 0 )
    return MBEDTLS_ERR_SSL_CONN_EOF;

  return (int)ret;
}

/* Custom BIO write callback */
static int
tls_bio_write( void *ctx, const unsigned char *buf, size_t len )
{
  compat_socket_t fd = *(compat_socket_t*)ctx;
  ssize_t ret;

  if( len > INT_MAX )
    len = INT_MAX;

  ret = send( fd, (const char*)buf, len, 0 );
  
  if( ret < 0 ) {
    if( compat_socket_get_error() == COMPAT_EWOULDBLOCK ||
        compat_socket_get_error() == COMPAT_EINPROGRESS )
      return MBEDTLS_ERR_SSL_WANT_WRITE;
    return -1; /* Network send error */
  }

  return (int)ret;
}

/* Platform RNG function using system random number generator */
static int
tls_rng( void *ctx, unsigned char *buf, size_t len )
{
  (void)ctx; /* Unused */
  
#ifdef __APPLE__
  /* Use macOS arc4random_buf - no framework linking required */
  arc4random_buf( buf, len );
#else
  /* Use Linux getrandom() */
  ssize_t ret = getrandom( buf, len, 0 );
  if( ret < 0 || (size_t)ret != len )
    return -1; /* RNG failure */
#endif
  
  return 0;
}

tls_socket_t*
tls_socket_alloc( compat_socket_t fd, const char *hostname )
{
  int ret;
  tls_socket_t *tls;

  tls = libspectrum_new( tls_socket_t, 1 );
  if( !tls )
    return NULL;

  tls->fd = fd;
  tls->handshake_complete = 0;
  tls->has_pending_data = 0;

  mbedtls_ssl_init( &tls->ssl );
  mbedtls_ssl_config_init( &tls->conf );

  /* Configure SSL */
  ret = mbedtls_ssl_config_defaults( &tls->conf,
                                     MBEDTLS_SSL_IS_CLIENT,
                                     MBEDTLS_SSL_TRANSPORT_STREAM,
                                     MBEDTLS_SSL_PRESET_DEFAULT );
  if( ret != 0 ) {
    ui_error( UI_ERROR_ERROR, "tls: failed to configure SSL defaults: %d\n", ret );
    tls_socket_free( tls );
    return NULL;
  }

  /* Disable certificate verification for now */
  mbedtls_ssl_conf_authmode( &tls->conf, MBEDTLS_SSL_VERIFY_NONE );

  /* Set RNG callback using platform RNG */
  mbedtls_ssl_conf_rng( &tls->conf, tls_rng, NULL );

  /* Set up SSL context */
  ret = mbedtls_ssl_setup( &tls->ssl, &tls->conf );
  if( ret != 0 ) {
    ui_error( UI_ERROR_ERROR, "tls: failed to setup SSL context: %d\n", ret );
    tls_socket_free( tls );
    return NULL;
  }

  /* Set hostname for SNI (Server Name Indication) */
  if( hostname && *hostname ) {
    ret = mbedtls_ssl_set_hostname( &tls->ssl, hostname );
    if( ret != 0 ) {
      ui_error( UI_ERROR_ERROR, "tls: failed to set hostname for SNI: %d\n", ret );
      /* Continue anyway - SNI is optional */
    }
  }

  /* Set custom BIO callbacks */
  mbedtls_ssl_set_bio( &tls->ssl, &tls->fd, tls_bio_write, tls_bio_read, NULL );

  return tls;
}

void
tls_socket_free( tls_socket_t *tls )
{
  if( !tls )
    return;

  mbedtls_ssl_free( &tls->ssl );
  mbedtls_ssl_config_free( &tls->conf );

  libspectrum_free( tls );
}

int
tls_connect( tls_socket_t *tls )
{
  int ret;
  int blocking_result;

  if( !tls )
    return -1;

  if( tls->handshake_complete )
    return 0;

  /* Make socket blocking for handshake */
  /* Note: compat_socket_blocking_mode( fd, 1 ) makes it blocking */
  blocking_result = compat_socket_blocking_mode( tls->fd, 1 );
  if( blocking_result != 0 ) {
    ui_error( UI_ERROR_ERROR, "tls: failed to set socket blocking mode\n" );
    return -1;
  }

  /* Perform blocking handshake - loop until complete or error */
  do {
    ret = mbedtls_ssl_handshake( &tls->ssl );
    
    if( ret == 0 ) {
      tls->handshake_complete = 1;
      break;
    }
    else if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE ) {
      /* With blocking socket, this shouldn't happen, but handle it anyway */
      /* Wait for socket to become ready */
      fd_set readfds, writefds;
      struct timeval timeout;
      int select_ret;
      
      FD_ZERO( &readfds );
      FD_ZERO( &writefds );
      
      if( ret == MBEDTLS_ERR_SSL_WANT_READ )
        FD_SET( tls->fd, &readfds );
      else
        FD_SET( tls->fd, &writefds );
      
      timeout.tv_sec = 30; /* 30 second timeout */
      timeout.tv_usec = 0;
      
      select_ret = select( tls->fd + 1, &readfds, &writefds, NULL, &timeout );
      if( select_ret <= 0 ) {
        /* Timeout or error */
        ret = -1;
        break;
      }
      /* Continue loop to retry handshake */
    }
    else {
      /* Handshake failed */
      char error_buf[256];
      mbedtls_strerror( ret, error_buf, sizeof(error_buf) );
      ui_error( UI_ERROR_ERROR, "tls: handshake failed: %d (%s)\n", ret, error_buf );
      break;
    }
  } while( ret != 0 && ( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE ) );

  /* Restore socket to non-blocking mode */
  compat_socket_blocking_mode( tls->fd, 0 );

  if( ret == 0 ) {
    return 0;
  }
  else {
    return ret;
  }
}

ssize_t
tls_read( tls_socket_t *tls, void *buf, size_t len )
{
  int ret;

  if( !tls || !tls->handshake_complete )
    return -1;

  if( len > INT_MAX )
    len = INT_MAX;

  ret = mbedtls_ssl_read( &tls->ssl, (unsigned char*)buf, len );
  
  if( ret > 0 ) {
    /* Check if there's more data available */
    if( mbedtls_ssl_get_bytes_avail( &tls->ssl ) > 0 )
      tls->has_pending_data = 1;
    else
      tls->has_pending_data = 0;
    return ret;
  }
  else if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE ) {
    tls->has_pending_data = 0;
    return 0; /* Would block */
  }
  else if( ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY ) {
    /* Peer closed connection gracefully */
    tls->has_pending_data = 0;
    return 0;
  }
  else {
    tls->has_pending_data = 0;
    return -1;
  }
}

ssize_t
tls_write( tls_socket_t *tls, const void *buf, size_t len )
{
  int ret;

  if( !tls || !tls->handshake_complete )
    return -1;

  if( len > INT_MAX )
    len = INT_MAX;

  ret = mbedtls_ssl_write( &tls->ssl, (const unsigned char*)buf, len );
  
  if( ret >= 0 )
    return ret;
  else if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE )
    return 0; /* Would block */
  else
    return -1;
}

void
tls_close( tls_socket_t *tls )
{
  if( !tls )
    return;

  if( tls->handshake_complete ) {
    mbedtls_ssl_close_notify( &tls->ssl );
  }
}

