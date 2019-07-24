/* ttx2000s.c: Routines for handling the TTX2000S teletext adapter
   Copyright (c) 2018 Alistair Cree

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

#include <config.h>

#include <libspectrum.h>

#include "compat.h"
#include "debugger/debugger.h"
#include "infrastructure/startup_manager.h"
#include "machine.h"
#include "memory_pages.h"
#include "module.h"
#include "periph.h"
#include "settings.h"
#include "unittests/unittests.h"
#include "ttx2000s.h"
#include "ui/ui.h"
#include "z80/z80.h"

static memory_page ttx2000s_memory_map_romcs_rom[ MEMORY_PAGES_IN_8K ];
static memory_page ttx2000s_memory_map_romcs_ram[ MEMORY_PAGES_IN_8K ];
static libspectrum_byte ttx2000s_ram[2048];	/* this should really be 1k */

static int ttx2000s_rom_memory_source;
static int ttx2000s_ram_memory_source;

int ttx2000s_paged = 0;

compat_socket_t teletext_socket = INVALID_SOCKET;
int ttx2000s_connected = 0;

/* default addresses and ports for the four channels */
char teletext_socket_ips[4][16] = { "127.0.0.1", "127.0.0.1", "127.0.0.1", "127.0.0.1" };
libspectrum_word teletext_socket_ports[4] = { 19761, 19762, 19763, 19764 };

int ttx2000s_channel;

static void ttx2000s_write( libspectrum_word port, libspectrum_byte val );
static void ttx2000s_change_channel( int channel );
static void ttx2000s_reset( int hard_reset );
static void ttx2000s_memory_map( void );

static int field_event;
static void ttx2000s_field_event( libspectrum_dword last_tstates, int event, void *user_data );

/* Debugger events */
static const char * const event_type_string = "ttx2000s";
static int page_event, unpage_event;

void
ttx2000s_page( void )
{
  if( ttx2000s_paged )
    return;

  ttx2000s_paged = 1;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();
  debugger_event( page_event );
}

void
ttx2000s_unpage( void )
{
  if( !ttx2000s_paged )
    return;

  ttx2000s_paged = 0;
  machine_current->ram.romcs = 0;
  machine_current->memory_map();
  debugger_event( unpage_event );
}

static module_info_t ttx2000s_module_info = {
  /* .reset = */ ttx2000s_reset,
  /* .romcs = */ ttx2000s_memory_map,
  /* .snapshot_enabled = */ NULL,
  /* .snapshot_from = */ NULL,
  /* .snapshot_to = */ NULL,
};

static const periph_port_t ttx2000s_ports[] = {
  { 0x0080, 0x0000, NULL, ttx2000s_write },
  { 0, 0, NULL, NULL }
};

static const periph_t ttx2000s_periph = {
  /* .option = */ &settings_current.ttx2000s,
  /* .ports = */ ttx2000s_ports,
  /* .hard_reset = */ 1,
  /* .activate = */ NULL,
};

static int
ttx2000s_init( void *context )
{
  int i;

  module_register( &ttx2000s_module_info );

  ttx2000s_rom_memory_source = memory_source_register( "TTX2000S ROM" );
  ttx2000s_ram_memory_source = memory_source_register( "TTX2000S RAM" );
  for( i = 0; i < MEMORY_PAGES_IN_8K; i++ )
    ttx2000s_memory_map_romcs_rom[i].source = ttx2000s_rom_memory_source;
  for( i = 0; i < MEMORY_PAGES_IN_2K; i++ )
    ttx2000s_memory_map_romcs_ram[i].source = ttx2000s_ram_memory_source;

  periph_register( PERIPH_TYPE_TTX2000S, &ttx2000s_periph );
  periph_register_paging_events( event_type_string, &page_event,
				 &unpage_event );

  compat_socket_networking_init(); // enable networking
  
  field_event = event_register( ttx2000s_field_event, "TTX2000S field event" );
  
  return 0;
}

static void
ttx2000s_end( void )
{
  compat_socket_networking_end();
}

void
ttx2000s_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DEBUGGER,
    STARTUP_MANAGER_MODULE_MEMORY,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_TTX2000S, dependencies,
                            ARRAY_SIZE( dependencies ), ttx2000s_init, NULL,
                            ttx2000s_end );
}

static void
ttx2000s_reset( int hard_reset GCC_UNUSED )
{
  int i;

  ttx2000s_paged = 0;

  event_remove_type( field_event );
  if ( !(settings_current.ttx2000s) ) {
    if ( teletext_socket != INVALID_SOCKET ) { /* close the socket */
      if( compat_socket_close( teletext_socket ) ) {
        /* what should we do if closing the socket fails? */
        ui_error( UI_ERROR_ERROR, "ttx2000s: close returned unexpected errno %d: %s\n", compat_socket_get_error(), compat_socket_get_strerror() );
      }
      ttx2000s_connected = 0; /* disconnected */
    }
    
    return;
  }
  
  /* enable video field interrupt event */
  event_add_with_data( tstates + 2 *        /* 20ms delay */
                machine_current->timings.processor_speed / 100,
                field_event, 0 );
  
  ttx2000s_channel = -1; /* force the connection to be reset */
  ttx2000s_change_channel( 0 );
  
  if( machine_load_rom_bank( ttx2000s_memory_map_romcs_rom, 0,
			     settings_current.rom_ttx2000s,
			     settings_default.rom_ttx2000s, 0x2000 ) ) {
    settings_current.ttx2000s = 0;
    periph_activate_type( PERIPH_TYPE_TTX2000S, 0 );
    return;
  }

  periph_activate_type( PERIPH_TYPE_TTX2000S, 1 );

  for( i = 0; i < MEMORY_PAGES_IN_2K; i++ ) {
    struct memory_page *page = &ttx2000s_memory_map_romcs_ram[ i ];
    page->page = &ttx2000s_ram[ i * MEMORY_PAGE_SIZE ];
    page->offset = i * MEMORY_PAGE_SIZE;
    page->writable = 1;
  }

  ttx2000s_paged = 1;
  machine_current->memory_map();
  machine_current->ram.romcs = 1;
}

static void
ttx2000s_memory_map( void )
{
  if( !ttx2000s_paged ) return;

  memory_map_romcs_8k( 0x0000, ttx2000s_memory_map_romcs_rom );
  memory_map_romcs_2k( 0x2000, ttx2000s_memory_map_romcs_ram );
  memory_map_romcs_2k( 0x2800, ttx2000s_memory_map_romcs_ram );
  memory_map_romcs_2k( 0x3000, ttx2000s_memory_map_romcs_ram );
  memory_map_romcs_2k( 0x3800, ttx2000s_memory_map_romcs_ram );
}

static void
ttx2000s_write( libspectrum_word port GCC_UNUSED, libspectrum_byte val )
{
  /* bits 0 and 1 select channel preset */
  ttx2000s_change_channel( val & 0x03 );
  /* bit 2 enables automatic frequency control */
  if (val & 0x08) /* bit 3 pages out */
    ttx2000s_unpage();
  else
    ttx2000s_page();
}

static void
ttx2000s_change_channel( int channel )
{
  if ( channel != ttx2000s_channel ) {
    /* only reconnect if channel preset changed */
    if ( teletext_socket != INVALID_SOCKET ) {
      if( compat_socket_close( teletext_socket ) ) {
        /* what should we do if closing the socket fails? */
        ui_error( UI_ERROR_ERROR, "ttx2000s: close returned unexpected errno %d: %s\n", compat_socket_get_error(), compat_socket_get_strerror() );
      }
    }
      
    teletext_socket = socket( AF_INET, SOCK_STREAM, 0 ); /* create a new socket */
    
    if ( compat_socket_blocking_mode( teletext_socket, 1 ) ) { /* make it non blocking */
      /* what should we do if it fails? */
      ui_error( UI_ERROR_ERROR, "ttx2000s: failed to set socket non-blocking" );
    }
    
    struct sockaddr_in teletext_serv_addr;
    const char *addr = teletext_socket_ips[channel & 3];
    teletext_serv_addr.sin_family = AF_INET; /* address family Internet */
    teletext_serv_addr.sin_port = htons (teletext_socket_ports[channel & 3]); /* Target port */
    teletext_serv_addr.sin_addr.s_addr = inet_addr (addr); /* Target IP */
    
    if (connect( teletext_socket, (SOCKADDR *)&teletext_serv_addr, sizeof(teletext_serv_addr) ) ) {
      /* TODO: ERROR HANDLING! */
      errno = compat_socket_get_error();
      #ifdef WIN32
      if (errno == WSAEWOULDBLOCK)
      #else
      if (errno == EINPROGRESS)
      #endif
      {
        /* we expect this as socket is non-blocking */
        ttx2000s_connected = 1; /* assume we are connected */
      } else {
        /* TODO: what should we do when there's an unexpected error? */
        ui_error( UI_ERROR_ERROR, "ttx2000s: connect returned unexpected errno %d: %s\n", errno, compat_socket_get_strerror() );
      }
    }
    
    ttx2000s_channel = channel;
  }
}

static void
ttx2000s_field_event ( libspectrum_dword last_tstates GCC_UNUSED, int event, void *user_data )
{
  int bytes_read;
  int i;
  libspectrum_byte ttx2000s_socket_buffer[672];
  
  /* do stuff */
  if ( teletext_socket != INVALID_SOCKET && ttx2000s_connected )
  {
    bytes_read = recv(teletext_socket, (char*) ttx2000s_socket_buffer, 672, 0);
    /* packet server sends 16 lines of 42 bytes, unusued lines are padded with 0x00 */
    if (bytes_read == 672) {
      for ( i = 0; i < 12; i++ ) {
        /* TTX2000S logic only reads the first 12 lines */
        if (ttx2000s_socket_buffer[i*42] != 0) {
          /* I think they are stored at 0x2100 onwards */
          ttx2000s_ram[i*64+256] = 0x27;
          memcpy(ttx2000s_ram + (i * 64) + 257, ttx2000s_socket_buffer + (i * 42), 42);
        }
      }
      /* only generate NMI when ROM is paged in and there is signal */
      if (ttx2000s_paged)
        event_add( 0, z80_nmi_event );    /* pull /NMI */
    } else if (bytes_read == -1) {
      errno = compat_socket_get_error();
      #ifdef WIN32
      if (errno == WSAENOTCONN || errno == WSAEWOULDBLOCK)
      #else
      if (errno == ENOTCONN || errno == EWOULDBLOCK)
      #endif
      {
        /* just ignore if the socket is not connected or recv would block */
      } else {
        /* TODO: what should we do when there's an unexpected error */
        ui_error( UI_ERROR_ERROR, "ttx2000s: recv returned unexpected errno %d: %s\n", errno, compat_socket_get_strerror() );
        ttx2000s_connected = 0; /* the connection has failed */
      }
    }
  }
  
  event_remove_type( field_event );
  event_add_with_data( tstates + 2 *        /* 20ms delay */
                machine_current->timings.processor_speed / 100,
                field_event, 0 );
}

int
ttx2000s_unittest( void )
{
  int r = 0;

  ttx2000s_paged = 1;
  ttx2000s_memory_map();
  machine_current->ram.romcs = 1;

  r += unittests_assert_8k_page( 0x0000, ttx2000s_rom_memory_source, 0 );
  r += unittests_assert_2k_page( 0x2000, ttx2000s_ram_memory_source, 0 );
  r += unittests_assert_2k_page( 0x2800, ttx2000s_ram_memory_source, 0 );
  r += unittests_assert_2k_page( 0x3000, ttx2000s_ram_memory_source, 0 );
  r += unittests_assert_2k_page( 0x3800, ttx2000s_ram_memory_source, 0 );
  r += unittests_assert_16k_ram_page( 0x4000, 5 );
  r += unittests_assert_16k_ram_page( 0x8000, 2 );
  r += unittests_assert_16k_ram_page( 0xc000, 0 );

  ttx2000s_paged = 0;
  machine_current->memory_map();
  machine_current->ram.romcs = 0;

  r += unittests_paging_test_48( 2 );

  return r;
}
