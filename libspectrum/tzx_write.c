/* tzx_write.c: Routines for writing .tzx files
   Copyright (c) 2001,2002 Philip Kendall

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

#include <stdio.h>
#include <string.h>

#include "tape.h"

/* The .tzx file signature (first 8 bytes) */
static const libspectrum_byte *signature = "ZXTape!\x1a";

/*** Local function prototypes ***/

static libspectrum_error
tzx_write_rom( libspectrum_tape_rom_block *rom_block,
	       libspectrum_byte **buffer, libspectrum_byte **ptr,
	       size_t *length );
static libspectrum_error
tzx_write_turbo( libspectrum_tape_turbo_block *rom_block,
		 libspectrum_byte **buffer, libspectrum_byte **ptr,
		 size_t *length );
static libspectrum_error
tzx_write_pure_tone( libspectrum_tape_pure_tone_block *tone_block,
		     libspectrum_byte **buffer, libspectrum_byte **ptr,
		     size_t *length );
static libspectrum_error
tzx_write_data( libspectrum_tape_pure_data_block *data_block,
		libspectrum_byte **buffer, libspectrum_byte **ptr,
		size_t *length );
static libspectrum_error
tzx_write_pulses( libspectrum_tape_pulses_block *pulses_block,
		  libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length );
static libspectrum_error
tzx_write_pause( libspectrum_tape_pause_block *pause_block,
		 libspectrum_byte **buffer, libspectrum_byte **ptr,
		 size_t *length );
static libspectrum_error
tzx_write_group_start( libspectrum_tape_group_start_block *start_block,
		       libspectrum_byte **buffer, libspectrum_byte **ptr,
		       size_t *length );
static libspectrum_error
tzx_write_jump( libspectrum_tape_jump_block *block,
		libspectrum_byte **buffer, libspectrum_byte **ptr,
		size_t *length );
static libspectrum_error
tzx_write_loop_start( libspectrum_tape_loop_start_block *block,
		      libspectrum_byte **buffer, libspectrum_byte **ptr,
		      size_t *length );
static libspectrum_error
tzx_write_select( libspectrum_tape_select_block *block,
		  libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length );
static libspectrum_error
tzx_write_stop( libspectrum_byte **buffer, libspectrum_byte **ptr,
		size_t *length );
static libspectrum_error
tzx_write_comment( libspectrum_tape_comment_block *comment_block,
		   libspectrum_byte **buffer, libspectrum_byte **ptr,
		   size_t *length );
static libspectrum_error
tzx_write_message( libspectrum_tape_message_block *message_block,
		   libspectrum_byte **buffer, libspectrum_byte **ptr,
		   size_t *length );
static libspectrum_error
tzx_write_archive_info( libspectrum_tape_archive_info_block *info_block,
			libspectrum_byte **buffer, libspectrum_byte **ptr,
			size_t *length );
static libspectrum_error
tzx_write_hardware( libspectrum_tape_hardware_block *hardware_block,
		    libspectrum_byte **buffer, libspectrum_byte **ptr,
		    size_t *length);
static libspectrum_error
tzx_write_custom( libspectrum_tape_custom_block *block,
		  libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length );

static libspectrum_error
tzx_write_empty_block( libspectrum_byte **buffer, libspectrum_byte **ptr,
		       size_t *length, libspectrum_tape_type id );

static libspectrum_error
tzx_write_bytes( libspectrum_byte **ptr, size_t length,
		 size_t length_bytes, libspectrum_byte *data );
static libspectrum_error
tzx_write_string( libspectrum_byte **ptr, libspectrum_byte *string );

/*** Function definitions ***/

/* The main write function */

libspectrum_error
libspectrum_tzx_write( libspectrum_tape *tape,
		       libspectrum_byte **buffer, size_t *length )
{
  libspectrum_error error;

  GSList *block_ptr;
  libspectrum_byte *ptr = *buffer;

  /* First, write the .tzx signature and the version numbers */
  error = libspectrum_make_room( buffer, strlen(signature) + 2, &ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  memcpy( ptr, signature, strlen( signature ) ); ptr += strlen( signature );
  *ptr++ = 1;		/* Major version number */
  *ptr++ = 13;		/* Minor version number */

  for( block_ptr = tape->blocks; block_ptr; block_ptr = block_ptr->next ) {
    libspectrum_tape_block *block = (libspectrum_tape_block*)block_ptr->data;

    switch( block->type ) {

    case LIBSPECTRUM_TAPE_BLOCK_ROM:
      error = tzx_write_rom( &(block->types.rom), buffer, &ptr, length );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_TURBO:
      error = tzx_write_turbo( &(block->types.turbo), buffer, &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PURE_TONE:
      error = tzx_write_pure_tone( &(block->types.pure_tone), buffer, &ptr,
				   length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PULSES:
      error = tzx_write_pulses( &(block->types.pulses), buffer, &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_PURE_DATA:
      error = tzx_write_data( &(block->types.pure_data), buffer, &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_PAUSE:
      error = tzx_write_pause( &(block->types.pause), buffer, &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_GROUP_START:
      error = tzx_write_group_start( &(block->types.group_start), buffer,
				     &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_GROUP_END:
      error = tzx_write_empty_block( buffer, &ptr, length, block->type );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_JUMP:
      error = tzx_write_jump( &(block->types.jump), buffer, &ptr, length );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_LOOP_START:
      error = tzx_write_loop_start( &(block->types.loop_start), buffer, &ptr,
				    length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_LOOP_END:
      error = tzx_write_empty_block( buffer, &ptr, length, block->type );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_SELECT:
      error = tzx_write_select( &(block->types.select), buffer, &ptr, length );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_STOP48:
      error = tzx_write_stop( buffer, &ptr, length );
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_COMMENT:
      error = tzx_write_comment( &(block->types.comment), buffer, &ptr,
				 length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_MESSAGE:
      error = tzx_write_message( &(block->types.message), buffer, &ptr,
				 length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO:
      error = tzx_write_archive_info( &(block->types.archive_info), buffer,
				      &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;
    case LIBSPECTRUM_TAPE_BLOCK_HARDWARE:
      error = tzx_write_hardware( &(block->types.hardware), buffer, &ptr,
				  length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    case LIBSPECTRUM_TAPE_BLOCK_CUSTOM:
      error = tzx_write_custom( &(block->types.custom), buffer, &ptr, length);
      if( error != LIBSPECTRUM_ERROR_NONE ) { free( *buffer ); return error; }
      break;

    default:
      free( *buffer );
      libspectrum_print_error(
        "libspectrum_tzx_write: unknown block type 0x%02x\n", block->type
      );
      return LIBSPECTRUM_ERROR_LOGIC;
    }
  }

  (*length) = ptr - *buffer;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_rom( libspectrum_tape_rom_block *rom_block,
	       libspectrum_byte **buffer, libspectrum_byte **ptr,
	       size_t *length )
{
  libspectrum_error error;

  /* Make room for the ID byte, the pause, the length and the actual data */
  error = libspectrum_make_room( buffer, 5 + rom_block->length, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Write the ID byte and the pause */
  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_ROM;
  libspectrum_write_word( ptr, rom_block->pause );

  /* Copy the data across */
  error = tzx_write_bytes( ptr, rom_block->length, 2, rom_block->data );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_turbo( libspectrum_tape_turbo_block *turbo_block,
		 libspectrum_byte **buffer, libspectrum_byte **ptr,
		 size_t *length )
{
  libspectrum_error error;

  /* Make room for the ID byte, the metadata and the actual data */
  error = libspectrum_make_room( buffer, 19 + turbo_block->length, ptr,
				 length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Write the ID byte and the metadata */
  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_TURBO;
  libspectrum_write_word( ptr, turbo_block->pilot_length );
  libspectrum_write_word( ptr, turbo_block->sync1_length );
  libspectrum_write_word( ptr, turbo_block->sync2_length );
  libspectrum_write_word( ptr, turbo_block->bit0_length  );
  libspectrum_write_word( ptr, turbo_block->bit1_length  );
  libspectrum_write_word( ptr, turbo_block->pilot_pulses );
  *(*ptr)++ = turbo_block->bits_in_last_byte;
  libspectrum_write_word( ptr, turbo_block->pause        );

  /* Copy the data across */
  error = tzx_write_bytes( ptr, turbo_block->length, 3, turbo_block->data );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_pure_tone( libspectrum_tape_pure_tone_block *tone_block,
		     libspectrum_byte **buffer, libspectrum_byte **ptr,
		     size_t *length )
{
  libspectrum_error error;

  /* Make room for the ID byte and the data */
  error = libspectrum_make_room( buffer, 5, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_PURE_TONE;
  libspectrum_write_word( ptr, tone_block->length );
  libspectrum_write_word( ptr, tone_block->pulses );
  
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_pulses( libspectrum_tape_pulses_block *pulses_block,
		  libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length )
{
  libspectrum_error error;

  size_t i;

  /* ID byte, count and 2 bytes for length of each pulse */
  size_t block_length = 2 + 2 * pulses_block->count;

  /* Make room for the ID byte, the count and the data */
  error = libspectrum_make_room( buffer, block_length, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_PULSES;
  *(*ptr)++ = pulses_block->count;
  for( i=0; i<pulses_block->count; i++ ) {
    libspectrum_write_word( ptr, pulses_block->lengths[i] );
  }
  
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_data( libspectrum_tape_pure_data_block *data_block,
		libspectrum_byte **buffer, libspectrum_byte **ptr,
		size_t *length )
{
  libspectrum_error error;

  /* Make room for the ID byte, the metadata and the actual data */
  error = libspectrum_make_room( buffer, 11 + data_block->length, ptr,
				 length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Write the ID byte and the metadata */
  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_PURE_DATA;
  libspectrum_write_word( ptr, data_block->bit0_length  );
  libspectrum_write_word( ptr, data_block->bit1_length  );
  *(*ptr)++ = data_block->bits_in_last_byte;
  libspectrum_write_word( ptr, data_block->pause        );

  /* Copy the data across */
  error = tzx_write_bytes( ptr, data_block->length, 3, data_block->data );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_pause( libspectrum_tape_pause_block *pause_block,
		 libspectrum_byte **buffer, libspectrum_byte **ptr,
		 size_t *length )
{
  libspectrum_error error;

  /* Make room for the ID byte and the data */
  error = libspectrum_make_room( buffer, 3, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_PAUSE;
  libspectrum_write_word( ptr, pause_block->length );
  
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_group_start( libspectrum_tape_group_start_block *start_block,
		       libspectrum_byte **buffer, libspectrum_byte **ptr,
		       size_t *length )
{
  libspectrum_error error;

  size_t name_length = strlen( start_block->name );

  /* Make room for the ID byte, the length byte and the name */
  error = libspectrum_make_room( buffer, 2 + name_length, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_GROUP_START;
  
  error = tzx_write_string( ptr, start_block->name );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_jump( libspectrum_tape_jump_block *block,
		libspectrum_byte **buffer, libspectrum_byte **ptr,
		size_t *length )
{
  libspectrum_error error;

  int u_offset;

  /* Make room for the ID byte and the offset */
  error = libspectrum_make_room( buffer, 3, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_JUMP;

  u_offset = block->offset; if( u_offset < 0 ) u_offset += 65536;
  libspectrum_write_word( ptr, u_offset );
  
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_loop_start( libspectrum_tape_loop_start_block *block,
		      libspectrum_byte **buffer, libspectrum_byte **ptr,
		      size_t *length )
{
  libspectrum_error error;

  /* Make room for the ID byte and the count */
  error = libspectrum_make_room( buffer, 3, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_LOOP_START;

  libspectrum_write_word( ptr, block->count );
  
  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_select( libspectrum_tape_select_block *block,
		  libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length )
{
  libspectrum_error error;

  size_t total_length, i;

  /* The count byte, and ( 2 offset bytes and 1 length byte ) per selection */
  total_length = 1 + 3 * block->count;

  for( i=0; i<block->count; i++ )
    total_length += strlen( block->descriptions[i] );

  /* On top of that, we need an id byte and 2 length bytes */
  error = libspectrum_make_room( buffer, total_length + 3, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_SELECT;
  libspectrum_write_word( ptr, total_length );
  *(*ptr)++ = block->count;

  for( i=0; i<block->count; i++ ) {
    libspectrum_write_word( ptr, block->offsets[i] );
    error = tzx_write_string( ptr, block->descriptions[i] );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_stop( libspectrum_byte **buffer, libspectrum_byte **ptr,
		size_t *length )
{
  libspectrum_error error;

  /* Make room for the ID byte and four length bytes */
  error = libspectrum_make_room( buffer, 5, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_STOP48;
  *(*ptr)++ = '\0'; *(*ptr)++ = '\0'; *(*ptr)++ = '\0'; *(*ptr)++ = '\0';

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_comment( libspectrum_tape_comment_block *comment_block,
		   libspectrum_byte **buffer, libspectrum_byte **ptr,
		   size_t *length )
{
  libspectrum_error error;

  size_t comment_length = strlen( comment_block->text );

  /* Make room for the ID byte, the length byte and the text */
  error = libspectrum_make_room( buffer, 2 + comment_length, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_COMMENT;

  error = tzx_write_string( ptr, comment_block->text );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_message( libspectrum_tape_message_block *message_block,
		   libspectrum_byte **buffer, libspectrum_byte **ptr,
		   size_t *length )
{
  libspectrum_error error;

  size_t text_length = strlen( message_block->text );

  /* Make room for the ID byte, the time byte, length byte and the text */
  error = libspectrum_make_room( buffer, 3 + text_length, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_MESSAGE;
  *(*ptr)++ = message_block->time;

  error = tzx_write_string( ptr, message_block->text );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_archive_info( libspectrum_tape_archive_info_block *info_block,
			libspectrum_byte **buffer, libspectrum_byte **ptr,
			size_t *length )
{
  libspectrum_error error;

  size_t i, total_length;

  /* ID byte, 1 count byte, 2 bytes (ID and length) for
     every string */
  total_length = 2 + 2 * info_block->count;
  /* And then the length of all the strings */
  for( i=0; i<info_block->count; i++ )
    total_length += strlen( info_block->strings[i] );

  /* Make room for all that, and two bytes storing the length */
  error = libspectrum_make_room( buffer, total_length + 2, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Write out the metadata */
  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_ARCHIVE_INFO;
  libspectrum_write_word( ptr, total_length );
  *(*ptr)++ = info_block->count;

  /* And the strings */
  for( i=0; i<info_block->count; i++ ) {
    *(*ptr)++ = info_block->ids[i];
    error = tzx_write_string( ptr, info_block->strings[i] );
    if( error != LIBSPECTRUM_ERROR_NONE ) return error;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_hardware( libspectrum_tape_hardware_block *block,
		    libspectrum_byte **buffer, libspectrum_byte **ptr,
		    size_t *length )
{
  libspectrum_error error;

  size_t i;

  /* We need one ID byte, one count byte and then three bytes for every
     entry */
  error = libspectrum_make_room( buffer, 3 * block->count + 2, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Write out the metadata */
  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_HARDWARE;
  *(*ptr)++ = block->count;

  /* And the info */
  for( i=0; i<block->count; i++ ) {
    *(*ptr)++ = block->types[i];
    *(*ptr)++ = block->ids[i];
    *(*ptr)++ = block->values[i];
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_custom( libspectrum_tape_custom_block *block,
		  libspectrum_byte **buffer, libspectrum_byte **ptr,
		  size_t *length )
{
  libspectrum_error error;

  /* An ID byte, 16 description bytes, 4 length bytes and the data
     itself */
  size_t total_length = 1 + 16 + 4 + block->length;

  /* Make room for the block */
  error = libspectrum_make_room( buffer, total_length, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = LIBSPECTRUM_TAPE_BLOCK_CUSTOM;
  memcpy( *ptr, block->description, 16 ); *ptr += 16;

  error = tzx_write_bytes( ptr, block->length, 4, block->data );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_empty_block( libspectrum_byte **buffer, libspectrum_byte **ptr,
		       size_t *length, libspectrum_tape_type id )
{
  libspectrum_error error;

  /* Make room for the ID byte */
  error = libspectrum_make_room( buffer, 1, ptr, length );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  *(*ptr)++ = id;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_bytes( libspectrum_byte **ptr, size_t length,
		 size_t length_bytes, libspectrum_byte *data )
{
  size_t i, shifter;

  /* Write out the appropriate number of length bytes */
  for( i=0, shifter = length; i<length_bytes; i++, shifter >>= 8 )
    *(*ptr)++ = shifter & 0xff;

  /* And then the actual data */
  memcpy( *ptr, data, length ); (*ptr) += length;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
tzx_write_string( libspectrum_byte **ptr, libspectrum_byte *string )
{
  libspectrum_error error;

  size_t i, length = strlen( string );

  error = tzx_write_bytes( ptr, length, 1, string );
  if( error != LIBSPECTRUM_ERROR_NONE ) return error;

  /* Fix up line endings */
  (*ptr) -= length;
  for( i=0; i<length; i++, (*ptr)++ ) if( **ptr == '\x0a' ) **ptr = '\x0d';

  return LIBSPECTRUM_ERROR_NONE;
}
