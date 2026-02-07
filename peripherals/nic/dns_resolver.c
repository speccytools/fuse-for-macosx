#include "config.h"

#include "dns_resolver.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <arpa/inet.h>

#ifdef HAVE_LIB_GLIB
#include <glib.h>
#else
#include "libspectrum.h"
#endif

#include "libspectrum.h"

/* Debug logging function for DNS resolver */
static void
dns_resolver_debug( const char *format, ... )
{
  if( DNS_DEBUG ) {
    va_list ap;
    va_start( ap, format );
    vprintf( format, ap );
    va_end( ap );
  }
}

/* Hash table mapping IP address (uint32_t) to hostname (char*) */
static GHashTable *dns_reverse_cache = NULL;

/* Hash function for uint32_t IP addresses */
static guint
dns_ip_hash( gconstpointer v )
{
  uint32_t ip = *(const uint32_t*)v;
  return (guint)ip;
}

/* Equality function for uint32_t IP addresses */
static gint
dns_ip_equal( gconstpointer v1, gconstpointer v2 )
{
  uint32_t ip1 = *(const uint32_t*)v1;
  uint32_t ip2 = *(const uint32_t*)v2;
  return ip1 == ip2 ? 1 : 0;
}

void
dns_resolver_init()
{
  if( !dns_reverse_cache ) {
    dns_reverse_cache = g_hash_table_new_full( dns_ip_hash, dns_ip_equal,
                                                libspectrum_free, libspectrum_free );
  }
}

void
dns_response_hostname_identified( const char *hostname, uint32_t ipv4 )
{
  uint32_t *key;
  char *hostname_copy;
  
  if( !hostname || !*hostname )
    return;
  
  if( !dns_reverse_cache )
    dns_resolver_init();
  
  /* Allocate key (IP address) - will be freed by g_hash_table_insert if entry exists */
  key = libspectrum_malloc( sizeof( uint32_t ) );
  if( !key )
    return;
  *key = ipv4;
  
  /* Allocate hostname copy */
  hostname_copy = libspectrum_malloc( strlen( hostname ) + 1 );
  if( !hostname_copy ) {
    libspectrum_free( key );
    return;
  }
  strcpy( hostname_copy, hostname );
  
  /* Check if entry already exists */
  uint32_t lookup_key = ipv4;
  const char *old_hostname = (const char*)g_hash_table_lookup( dns_reverse_cache, &lookup_key );
  
  /* Insert or update the mapping */
  /* If key already exists, g_hash_table_insert will:
   *   - Free the new key we pass (key) via key_destroy_func
   *   - Free the old value via value_destroy_func
   *   - Update the value to the new hostname_copy
   * If key doesn't exist, it will insert the new key and value
   */
  g_hash_table_insert( dns_reverse_cache, key, hostname_copy );
  
  /* Log cache addition/update */
  struct in_addr addr;
  addr.s_addr = htonl( ipv4 );
  if( old_hostname ) {
    dns_resolver_debug( "dns: cache update: %s -> %s\n", inet_ntoa( addr ), hostname );
  } else {
    dns_resolver_debug( "dns: cache add: %s -> %s\n", inet_ntoa( addr ), hostname );
  }
}

const char*
dns_resolve_hostname( uint32_t ipv4 )
{
  uint32_t key = ipv4;
  const char *hostname;
  struct in_addr addr;
  
  if( !dns_reverse_cache )
    return NULL;
  
  addr.s_addr = htonl( ipv4 );
  hostname = (const char*)g_hash_table_lookup( dns_reverse_cache, &key );
  
  if( hostname ) {
    dns_resolver_debug( "dns: cache hit: %s -> %s\n", inet_ntoa( addr ), hostname );
  } else {
    dns_resolver_debug( "dns: cache miss: %s\n", inet_ntoa( addr ) );
  }
  
  return hostname;
}
