/* screenshot.c: Routines for saving .png screenshots
   Copyright (c) 2002 Philip Kendall

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

#include "settings.h"
#include "types.h"
#include "ui/ui.h"

#ifdef USE_LIBPNG

#include <png.h>

BYTE screenshot_screen[240][640];

static BYTE saved_screen[240][640];

int
screenshot_save( void )
{
  size_t y;

  for( y=0; y<240; y++ )
    memcpy( saved_screen[y], screenshot_screen[y], 640 * sizeof( BYTE ) );

  return 0;
}

int
screenshot_write( const char *filename )
{
  FILE *f;

  png_structp png_ptr;
  png_infop info_ptr;

  BYTE png_data[240][320], *row_pointers[480];
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
		settings_current.double_screen ? 640 : 320, /* height */
		settings_current.double_screen ? 480 : 240, /* width */
		4,			/* 2^4 colours */
		PNG_COLOR_TYPE_PALETTE,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT );

  png_set_PLTE( png_ptr, info_ptr, palette, 16 );
 
  for( y=0; y<240; y++ ) {

    if( settings_current.double_screen ) {
      row_pointers[2*y] = row_pointers[2*y+1] = png_data[y];
      for( x=0; x<320; x++ )
	png_data[y][x] = (   saved_screen[y][2*x  ] & 0x1f )        |
	                 ( ( saved_screen[y][2*x+1] & 0x1f ) << 4 );
    } else {
      row_pointers[y] = png_data[y];
      for( x=0; x<160; x++ )
	png_data[y][x] = (   saved_screen[y][4*x+2] & 0x1f )        |
	                 ( ( saved_screen[y][4*x  ] & 0x1f ) << 4 );
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
