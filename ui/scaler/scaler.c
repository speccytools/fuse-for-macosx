/* scaler.c: code for selecting (etc) scalers
 * Copyright (C) 2003 Fredrick Meunier, Philip Kendall
 * 
 * $Id$
 *
 * Originally taken from ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001/2002 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <config.h>
#include <string.h>
#include "types.h"

#include "scaler.h"
#include "scaler_internals.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"

static int scaler_supported[ SCALER_NUM ] = {0};

int scalers_registered = 0;

struct scaler_info {

  const char *name;
  const char *id;
  enum scaler_flags_t flags;
  ScalerProc *scaler16, *scaler32;

};

/* Information on each of the available scalers. Make sure this array stays
   in the same order as scaler.h:scaler_type */
static struct scaler_info available_scalers[] = {

  { "Timex Half size", "half",       SCALER_FLAGS_NONE,	       
    scaler_Half_16,       scaler_Half_32                       },
  { "Normal",	       "normal",     SCALER_FLAGS_NONE,
    scaler_Normal1x_16,   scaler_Normal1x_32                   },
  { "Double size",     "2x",	     SCALER_FLAGS_NONE,
    scaler_Normal2x_16,   scaler_Normal2x_32                   },
  { "Triple size",     "3x",	     SCALER_FLAGS_NONE,
    scaler_Normal3x_16,   scaler_Normal3x_32                   },
  { "2xSaI",	       "2xsai",	     SCALER_EXPAND_1_PIXEL,
    scaler_2xSaI_16,      scaler_2xSaI_32                      },
  { "Super 2xSaI",     "super2xsai", SCALER_EXPAND_1_PIXEL, 
    scaler_Super2xSaI_16, scaler_2xSaI_32                      },
  { "SuperEagle",      "supereagle", SCALER_EXPAND_1_PIXEL,
    scaler_SuperEagle_16, scaler_SuperEagle_32                 },
  { "AdvMAME 2x",      "advmame2x",  SCALER_EXPAND_1_PIXEL, 
    scaler_AdvMame2x_16,  scaler_AdvMame2x_32		       },
  { "TV 2x",	       "tv2x",	     SCALER_EXPAND_1_PIXEL, 
    scaler_TV2x_16,       scaler_TV2x_32                       },
  { "Timex TV",	       "timextv",    SCALER_EXPAND_2_Y_PIXELS, 
    scaler_TimexTV_16,    scaler_TimexTV_32		       },
};

scaler_type current_scaler = SCALER_NUM;
ScalerProc *scaler_proc16, *scaler_proc32;
scaler_flags_t scaler_flags;

int
scaler_select_scaler( scaler_type scaler )
{
  if( !scaler_is_supported( scaler ) ) return 1;

  if( current_scaler == scaler ) return 0;

  current_scaler = scaler;

  if( settings_current.start_scaler_mode ) free( settings_current.start_scaler_mode );
  settings_current.start_scaler_mode =
    strdup( available_scalers[current_scaler].id );
  if( !settings_current.start_scaler_mode ) {
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return 1;
  }

  scaler_proc16 = scaler_get_proc16( current_scaler );
  scaler_proc32 = scaler_get_proc32( current_scaler );
  scaler_flags = scaler_get_flags( current_scaler );

  uidisplay_hotswap_gfx_mode();

  return 0;
}

int
scaler_select_id( const char *id )
{
  scaler_type i;

  for( i=0; i < SCALER_NUM; i++ ) {
    if( ! strcmp( available_scalers[i].id, id ) ) {
      scaler_select_scaler( i );
      return 0;
    }
  }

  ui_error( UI_ERROR_ERROR, "Scaler id '%s' unknown", id );
  return 1;
}

void
scaler_register_clear( void )
{
  scalers_registered = 0;
  memset( scaler_supported, 0, sizeof(int) * SCALER_NUM );
}

void
scaler_register( scaler_type scaler )
{
  if( scaler_supported[scaler] == 1 ) return;
  scalers_registered++;
  scaler_supported[scaler] = 1;
}

int
scaler_is_supported( scaler_type scaler )
{
  return ( scaler >= SCALER_NUM ? 0 : scaler_supported[scaler] );
}

const char *
scaler_name( scaler_type scaler )
{
  return available_scalers[scaler].name;
}

ScalerProc*
scaler_get_proc16( scaler_type scaler )
{
  return available_scalers[scaler].scaler16;
}

ScalerProc*
scaler_get_proc32( scaler_type scaler )
{
  return available_scalers[scaler].scaler32;
}

scaler_flags_t
scaler_get_flags( scaler_type scaler )
{
  return available_scalers[scaler].flags;
}
