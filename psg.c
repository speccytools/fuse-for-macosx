/* psg.c: recording AY chip output to .psg files
   Copyright (c) 2003 Matthew Westcott

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

#include <stdio.h>

#include "psg.h"
#include "ui/ui.h"

/* Are we currently recording a .psg file? */
int psg_recording;

static BYTE psg_register_values[16];

/* booleans to indicate the registers written to in this frame */
static int psg_registers_written[16];

/* number of prior frames with no AY events */
static int psg_empty_frame_count;

static FILE *psg_file;

static int write_frame_separator( void );

int
psg_init( void )
{
  psg_recording = 0;
  return 0;
}

int
psg_start_recording( const char *filename )
{
  int i;

  if( psg_recording ) return 1;
  
  psg_file = fopen( filename, "wb" );
  if( psg_file == NULL ) {
    ui_error( UI_ERROR_ERROR, "unable to open PSG file for writing" );
    return 1;
  }

  /* write PSG file header */
  if( fprintf( psg_file, "PSG\x1a" ) < 0 ) {
    ui_error( UI_ERROR_ERROR, "unable to write PSG file header" );
    return 1;
  }
  for( i = 0; i < 12; i++ ) putc( 0, psg_file );

  /* begin with no registers written */
  for( i = 0; i < 16; i++ ) psg_registers_written[i] = 0;

  psg_empty_frame_count = 1;

  psg_recording = 1;
  return 0;
}

int
psg_stop_recording( void )
{
  if( !psg_recording ) return 1;

  /* flush final frame */
  psg_frame();

  /* end file with the appropriate end-frame marker */
  write_frame_separator();

  fclose( psg_file );

  psg_recording = 0;
  return 0;
}

static int
write_frame_separator( void )
{
  while( psg_empty_frame_count > 0xff ) {
    putc( 0xfe, psg_file );
    putc( 0xff, psg_file );
    psg_empty_frame_count -= 0xff;
  }

  if( psg_empty_frame_count > 1 ) {
    putc( 0xfe, psg_file );
    putc( psg_empty_frame_count, psg_file );
  } else if( psg_empty_frame_count == 1 ) {
    putc( 0xff, psg_file );
  }

  return 0;
}

int
psg_frame( void )
{
  int i;
  int ay_updated;

  if( !psg_recording ) return 0;

  /* check if any AY sound events have happened this frame */
  ay_updated = 0;
  for( i = 0; i < 14 && !ay_updated; i++ )
    ay_updated = psg_registers_written[i];

  if( ay_updated ) {

    write_frame_separator();
    for( i = 0; i < 14; i++ ) {
      if( psg_registers_written[i] ) {
	putc( i, psg_file );
	putc( psg_register_values[i], psg_file );
      }
    }
    psg_empty_frame_count = 1;

  } else {

    /* AY not updated */
    psg_empty_frame_count++;

  }

  for( i = 0; i < 16; i++ ) psg_registers_written[i] = 0;
  return 0;
}

int
psg_write_register( BYTE reg, BYTE value )
{
  if( psg_registers_written[reg] ) return 0;
  psg_register_values[reg] = value;
  psg_registers_written[reg] = 1;
  return 0;
}

int
psg_end( void )
{
  if( psg_recording ) return psg_stop_recording();
  return 0;
}
