/* periph.c: code for handling peripherals
   Copyright (c) 2004 Philip Kendall

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#include <config.h>

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "joystick.h"
#include "periph.h"
#include "rzx.h"
#include "settings.h"
#include "simpleide.h"
#include "ui/ui.h"
#include "zxatasp.h"
#include "zxcf.h"

/*
 * General peripheral list handling routines
 */

/* Full information about a peripheral */
typedef struct periph_private_t {

  int id;
  int active;

  periph_t peripheral;

} periph_private_t;

static GSList *peripherals = NULL;
static int last_id = 0;

static gint find_by_id( gconstpointer data, gconstpointer id );
static void free_peripheral( gpointer data, gpointer user_data );

/* Register a peripheral. Returns -1 on error or a peripheral ID if
   successful */
int
periph_register( const periph_t *peripheral )
{
  periph_private_t *private;

  private = malloc( sizeof( periph_private_t ) );
  if( !private ) {
    ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__, __LINE__ );
    return -1;
  }

  private->id = last_id++;
  private->active = 1;
  private->peripheral = *peripheral;

  peripherals = g_slist_append( peripherals, private );

  return private->id;
}

/* Register many peripherals */
int
periph_register_n( const periph_t *peripherals_list, size_t n )
{
  const periph_t *ptr;

  for( ptr = peripherals_list; n--; ptr++ ) {
    int id;
    id = periph_register( ptr ); if( id == -1 ) return -1;
  }

  return 0;
}

/* (De)activate a specific peripheral */
int
periph_set_active( int id, int active )
{
  GSList *ptr;
  periph_private_t *private;

  ptr = g_slist_find_custom( peripherals, &id, find_by_id );
  if( !ptr ) {
    ui_error( UI_ERROR_ERROR, "couldn't find peripheral ID %d", id );
    return 1;
  }

  private = ptr->data;

  private->active = active;

  return 0;
}

static gint
find_by_id( gconstpointer data, gconstpointer user_data )
{
  const periph_private_t *private = data;
  int id = *(const int*)user_data;

  return private->id - id;
}

/* Clear all peripherals */
void
periph_clear( void )
{
  g_slist_foreach( peripherals, free_peripheral, NULL );
  g_slist_free( peripherals );
  peripherals = NULL;

  last_id = 0;
}

static void
free_peripheral( gpointer data, gpointer user_data )
{
  periph_t *private = data;

  free( private );
}

/*
 * The actual routines to read and write a port
 */

static void read_peripheral( gpointer data, gpointer user_data );
static void write_peripheral( gpointer data, gpointer user_data );

/* Internal type used for passing to read_peripheral and write_peripheral */
struct peripheral_data_t {

  libspectrum_word port;

  int attached;
  libspectrum_byte value;
};

libspectrum_byte
readport( libspectrum_word port )
{
  struct peripheral_data_t callback_info;

  /* Trigger the debugger if wanted */
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_PORT_READ, port ) )
    debugger_mode = DEBUGGER_MODE_HALTED;

  /* If we're doing RZX playback, get a byte from the RZX file */
  if( rzx_playback ) {

    libspectrum_error error;
    libspectrum_byte value;

    error = libspectrum_rzx_playback( rzx, &value );
    if( error ) { rzx_stop_playback( 1 ); return readport( port ); }

    return value;
  }

  /* If we're not doing RZX playback, get the byte normally */
  callback_info.port = port;
  callback_info.attached = 0;
  callback_info.value = 0xff;

  g_slist_foreach( peripherals, read_peripheral, &callback_info );

  if( !callback_info.attached )
    callback_info.value = machine_current->unattached_port();

  /* If we're RZX recording, store this byte */
  if( rzx_recording ) rzx_store_byte( callback_info.value );

  return callback_info.value;
}

static void
read_peripheral( gpointer data, gpointer user_data )
{
  periph_private_t *private = data;
  struct peripheral_data_t *callback_info = user_data;

  periph_t *peripheral;

  peripheral = &( private->peripheral );

  if( private->active && peripheral->read &&
      ( ( callback_info->port & peripheral->mask ) == peripheral->value ) ) {
    callback_info->value &= peripheral->read( callback_info->port,
					      &( callback_info->attached ) );
  }
}

void
writeport( libspectrum_word port, libspectrum_byte b )
{
  struct peripheral_data_t callback_info;

  /* Trigger the debugger if wanted */
  if( debugger_mode != DEBUGGER_MODE_INACTIVE &&
      debugger_check( DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE, port ) )
    debugger_mode = DEBUGGER_MODE_HALTED;

  callback_info.port = port;
  callback_info.value = b;
  
  g_slist_foreach( peripherals, write_peripheral, &callback_info );
}

static void
write_peripheral( gpointer data, gpointer user_data )
{
  periph_private_t *private = data;
  struct peripheral_data_t *callback_info = user_data;

  periph_t *peripheral;

  peripheral = &( private->peripheral );
  
  if( private->active && peripheral->write &&
      ( ( callback_info->port & peripheral->mask ) == peripheral->value ) )
    peripheral->write( callback_info->port, callback_info->value );
}

/*
 * The more Fuse-specific peripheral handling routines
 */

/* What sort of Kempston interface does the current machine have */
static periph_present kempston_present;

/* Is the Kempston interface currently active */
int periph_kempston_active;

/* What sort of Interface II does the current machine have */
static periph_present interface2_present;

/* Is the Interface II currently active */
int periph_interface2_active;

int
periph_setup( const periph_t *peripherals_list, size_t n,
	      periph_present kempston,
	      periph_present interface2 )
{
  int error;

  periph_clear();

  error =
    periph_register_n( simpleide_peripherals, simpleide_peripherals_count );
  if( error ) return error;

  periph_register_n( zxatasp_peripherals, zxatasp_peripherals_count );
  periph_register_n( zxcf_peripherals, zxcf_peripherals_count );

  error = periph_register_n( peripherals_list, n ); if( error ) return error;

  kempston_present = kempston;
  interface2_present = interface2;

  periph_update();

  return 0;
}

void
periph_update( void )
{
  switch( kempston_present ) {
  case PERIPH_PRESENT_NEVER: periph_kempston_active = 0; break;
  case PERIPH_PRESENT_OPTIONAL:
    periph_kempston_active = settings_current.joy_kempston; break;
  case PERIPH_PRESENT_ALWAYS: periph_kempston_active = 1; break;
  }
  switch( interface2_present ) {
  case PERIPH_PRESENT_NEVER: periph_interface2_active = 0; break;
  case PERIPH_PRESENT_OPTIONAL:
    periph_interface2_active = settings_current.interface2; break;
  case PERIPH_PRESENT_ALWAYS: periph_interface2_active = 1; break;
  }
  
  machine_current->memory_map();
}
