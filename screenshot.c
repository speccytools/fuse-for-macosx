/* screenshot.c: Routines for handling .png and .scr screenshots
   Copyright (c) 2002-2003 Philip Kendall, Fredrick Meunier

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

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "display.h"
#include "machine.h"
#include "scld.h"
#include "settings.h"
#include "types.h"
#include "ui/ui.h"
#include "utils.h"

#define STANDARD_SCR_SIZE 6912
#define MONO_BITMAP_SIZE 6144
#define HICOLOUR_SCR_SIZE (2 * MONO_BITMAP_SIZE)
#define HIRES_ATTR HICOLOUR_SCR_SIZE
#define HIRES_SCR_SIZE (HICOLOUR_SCR_SIZE + 1)

#ifdef USE_LIBPNG

#include <png.h>

/* A copy of display.c:display_image, taken so we can draw widgets on
   display_image */
static WORD saved_screen[2*DISPLAY_SCREEN_HEIGHT][DISPLAY_SCREEN_WIDTH];

int
screenshot_save( void )
{
  memcpy( saved_screen, display_image, sizeof( display_image ) );
  return 0;
}

int
screenshot_write( const char *filename )
{
  FILE *f;

  png_structp png_ptr;
  png_infop info_ptr;

  BYTE png_data[240][320*3], *row_pointers[240];
  size_t x, y;

  png_color palette[] = { {   0,   0,   0 },
			  {   0,   0, 192 },
			  { 192,   0,   0 },
			  { 192,   0, 192 },
			  {   0, 192,   0 },
			  {   0, 192, 192 },
			  { 192, 192,   0 },
			  { 192, 192, 192 },
			  {   0,   0,   0 },
			  {   0,   0, 255 },
			  { 255,   0,   0 },
			  { 255,   0, 255 },
			  {   0, 255,   0 },
			  {   0, 255, 255 },
			  { 255, 255,   0 },
			  { 255, 255, 255 } };

  f = fopen( filename, "wb" );
  if( !f ) {
    ui_error( UI_ERROR_ERROR, "Couldn't open `%s': %s", filename,
	      strerror( errno ) );
    return 1;
  }

  png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING,
				     NULL, NULL, NULL );
  if( !png_ptr ) {
    ui_error( UI_ERROR_ERROR, "Couldn't allocate png_ptr" );
    fclose( f );
    return 1;
  }

  info_ptr = png_create_info_struct( png_ptr );
  if( !info_ptr ) {
    ui_error( UI_ERROR_ERROR, "Couldn't allocate info_ptr" );
    png_destroy_write_struct( &png_ptr, NULL );
    fclose( f );
    return 1;
  }

  /* Set up the error handling; libpng will return to here if it
     encounters an error */
  if( setjmp( png_jmpbuf( png_ptr ) ) ) {
    ui_error( UI_ERROR_ERROR, "Error from libpng" );
    png_destroy_write_struct( &png_ptr, &info_ptr );
    fclose( f );
    return 1;
  }

  png_init_io( png_ptr, f );

  /* Make files as small as possible */
  png_set_compression_level( png_ptr, Z_BEST_COMPRESSION );

  png_set_IHDR( png_ptr, info_ptr,
		320, 240, 8, 
		PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT );

  for( y=0; y<240; y++ ) {
    row_pointers[y] = png_data[y];
    for( x=0; x<320; x++ ) {
      int colour = saved_screen[y][x];

      png_data[y][3*x  ] = palette[colour].red;
      png_data[y][3*x+1] = palette[colour].green;
      png_data[y][3*x+2] = palette[colour].blue;
    }
  }
  
  png_set_rows( png_ptr, info_ptr, row_pointers );

  png_write_png( png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL );

  png_destroy_write_struct( &png_ptr, &info_ptr );

  if( fclose( f ) ) {
    ui_error( UI_ERROR_ERROR, "Couldn't close `%s': %s", filename,
	      strerror( errno ) );
    return 1;
  }

  return 0;
}

#endif				/* #ifdef USE_LIBPNG */

int
screenshot_scr_write( const char *filename )
{
  BYTE scr_data[HIRES_SCR_SIZE];
  int scr_length;

  memset( scr_data, 0, HIRES_SCR_SIZE );

  if( scld_last_dec.name.hires ) {
    memcpy( scr_data,
            &RAM[machine_current->ram.current_screen][display_get_addr(0,0)],
            MONO_BITMAP_SIZE );
    memcpy( scr_data + MONO_BITMAP_SIZE,
            &RAM[machine_current->ram.current_screen][display_get_addr(0,0) +
            ALTDFILE_OFFSET], MONO_BITMAP_SIZE );
    scr_data[HIRES_ATTR] = scld_last_dec.mask.hirescol |
                           scld_last_dec.mask.scrnmode;
    scr_length = HIRES_SCR_SIZE;
  }
  else if( scld_last_dec.name.b1 ) {
    memcpy( scr_data,
            &RAM[machine_current->ram.current_screen][display_get_addr(0,0)],
            MONO_BITMAP_SIZE );
    memcpy( scr_data + MONO_BITMAP_SIZE,
            &RAM[machine_current->ram.current_screen][display_get_addr(0,0) +
            ALTDFILE_OFFSET], MONO_BITMAP_SIZE );
    scr_length = HICOLOUR_SCR_SIZE;
  }
  else { /* ALTDFILE and default */
    memcpy( scr_data, 
            &RAM[machine_current->ram.current_screen][display_get_addr(0,0)],
            STANDARD_SCR_SIZE );
    scr_length = STANDARD_SCR_SIZE;
  }

  return utils_write_file( filename, scr_data, scr_length );
}

#ifdef WORDS_BIGENDIAN

typedef struct {
  unsigned b7 : 1;
  unsigned b6 : 1;
  unsigned b5 : 1;
  unsigned b4 : 1;
  unsigned b3 : 1;
  unsigned b2 : 1;
  unsigned b1 : 1;
  unsigned b0 : 1;
} byte_field_type;

#else			/* #ifdef WORDS_BIGENDIAN */

typedef struct {
  unsigned b0 : 1;
  unsigned b1 : 1;
  unsigned b2 : 1;
  unsigned b3 : 1;
  unsigned b4 : 1;
  unsigned b5 : 1;
  unsigned b6 : 1;
  unsigned b7 : 1;
} byte_field_type;

#endif			/* #ifdef WORDS_BIGENDIAN */

typedef union {
  BYTE byte;
  byte_field_type bit;
} bft_union;

BYTE
convert_hires_to_lores( BYTE high, BYTE low )
{
  bft_union ret, h, l;

  h.byte = high;
  l.byte = low;

  ret.bit.b0 = l.bit.b0;
  ret.bit.b1 = l.bit.b2;
  ret.bit.b2 = l.bit.b4;
  ret.bit.b3 = l.bit.b6;
  ret.bit.b4 = h.bit.b0;
  ret.bit.b5 = h.bit.b2;
  ret.bit.b6 = h.bit.b4;
  ret.bit.b7 = h.bit.b6;

  return ret.byte;
}

int
screenshot_scr_read( const char *filename )
{
  int error = 0;
  int i;
  size_t length;
  BYTE *screen;

  error =  utils_read_file( filename, &screen, &length );
  if( error ) return error;

  switch( length ) {
  case STANDARD_SCR_SIZE:
    memcpy( &RAM[machine_current->ram.current_screen][display_get_addr(0,0)],
            screen, length );

    /* If it is a Timex and it is in hi colour or hires mode, switch out of
       hires or hicolour mode */
    if( scld_last_dec.name.b1 || scld_last_dec.name.hires )
      scld_dec_write( 0xff, scld_last_dec.byte & ~HIRES );
    break;

  case HICOLOUR_SCR_SIZE:
    /* If it is a Timex and it is not in hi colour mode, copy screen and switch
        mode if neccesary */
    /* If it is not a Timex copy the mono bitmap and raise an error */
    if( machine_current->timex ) {
      if( !scld_last_dec.name.b1 )
        scld_dec_write( 0xff, ( scld_last_dec.byte & ~HIRESATTR ) | EXTCOLOUR );
      memcpy( &RAM[machine_current->ram.current_screen][display_get_addr(0,0) +
              ALTDFILE_OFFSET], screen + MONO_BITMAP_SIZE, MONO_BITMAP_SIZE );
    } else
      ui_error( UI_ERROR_INFO,
            "The file contained a TC2048 high-colour screen, loaded as mono");

    memcpy( &RAM[machine_current->ram.current_screen][display_get_addr(0,0)],
              screen, MONO_BITMAP_SIZE );
    break;

  case HIRES_SCR_SIZE:
    /* If it is a Timex and it is not in hi res mode, copy screen and switch
        mode if neccesary */
    /* If it is not a Timex scale the bitmap and raise an error */
    if( machine_current->timex ) {
      memcpy( &RAM[machine_current->ram.current_screen][display_get_addr(0,0)],
                screen, MONO_BITMAP_SIZE );

      memcpy( &RAM[machine_current->ram.current_screen][display_get_addr(0,0)] +
              ALTDFILE_OFFSET, screen + MONO_BITMAP_SIZE, MONO_BITMAP_SIZE );
      if( !scld_last_dec.name.hires )
        scld_dec_write( 0xff,
            ( scld_last_dec.byte & ~( HIRESCOLMASK | HIRES ) ) |
            ( *(screen + HIRES_ATTR) & ( HIRESCOLMASK | HIRES ) ) );
    } else {
      for( i = 0; i < MONO_BITMAP_SIZE; i++ )
        RAM[machine_current->ram.current_screen][display_get_addr(0,0) + i] =
          convert_hires_to_lores( *(screen + MONO_BITMAP_SIZE + i),
                                  *(screen + i) );

      /* set attributes based on hires attribute byte */
      for( i = 0; i < 768; i++ )
        RAM[machine_current->ram.current_screen][display_get_addr(0,0) +
            MONO_BITMAP_SIZE + i] = hires_convert_dec( *(screen + HIRES_ATTR) );

      ui_error( UI_ERROR_INFO,
            "The file contained a TC2048 high-res screen, converted to lores");
    }
    break;

  default:
    ui_error( UI_ERROR_ERROR, "'%s' is not a valid scr file", filename );
    error = 1;
  }

  display_refresh_all();

  return error;
}
