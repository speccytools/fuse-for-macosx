/* tape.h: tape handling routines
   Copyright (c) 1999-2001 Philip Kendall

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

#ifndef FUSE_TAPE_H
#define FUSE_TAPE_H

#ifndef FUSE_TYPES_H
#include "types.h"
#endif

int tape_init( void );

int tape_open( const char *filename, int autoload );
int tape_open_tap_buffer( unsigned char *buffer, size_t length, int autoload );
int tape_open_tzx_buffer( unsigned char *buffer, size_t length, int autoload );

int tape_close( void );
int tape_select_block( size_t n );
int tape_get_current_block( void );
int tape_write( const char *filename );

int tape_load_trap( void );
int tape_save_trap( void );

int tape_play( void );
int tape_toggle_play( void );

int tape_next_edge( DWORD last_tstates );

int tape_stop( void );

int tape_get_block_list( char ****list, size_t *n );
int tape_free_block_list( char ***list, size_t n );

extern int tape_playing;
extern int tape_microphone;

#endif
