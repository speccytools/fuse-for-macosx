/* ScummVM - Scumm Interpreter
 * Copyright (C) 2002-2003 The ScummVM project, Fredrick Meunier and
 *			   Philip Kendall
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 */

#ifndef SCALER_H
#define SCALER_H

#include <libspectrum.h>

#ifdef __cplusplus
extern "C" {
#endif        /* #ifdef __cplusplus */

typedef enum scaler_type {
  SCALER_HALF = 0,
  SCALER_HALFSKIP,
  SCALER_NORMAL,
  SCALER_DOUBLESIZE,
  SCALER_TRIPLESIZE,
  SCALER_2XSAI,
  SCALER_SUPER2XSAI,
  SCALER_SUPEREAGLE,
  SCALER_ADVMAME2X,
  SCALER_ADVMAME3X,
  SCALER_TV2X,
  SCALER_TIMEXTV,
  SCALER_DOTMATRIX,
  SCALER_TIMEX1_5X,

  SCALER_NUM		/* End marker; do not remove */
} scaler_type;

typedef enum scaler_flags_t {
  SCALER_FLAGS_NONE        = 0,
  SCALER_FLAGS_EXPAND      = 1 << 0,
} scaler_flags_t;

typedef enum scale_factor_t {
  SCALE_FACTOR_HALF        = 1 << 0,
  SCALE_FACTOR_ONE         = 1 << 1,
  SCALE_FACTOR_ONE_HALF    = 1 << 2,
  SCALE_FACTOR_TWO         = 1 << 3,
  SCALE_FACTOR_THREE       = 1 << 4,
} scale_factor_t;

typedef void ScalerProc( const libspectrum_byte *srcPtr,
			 libspectrum_dword srcPitch,
			 libspectrum_byte *dstPtr, libspectrum_dword dstPitch,
			 int width, int height );

/* The type of function used to expand the area dirtied by a scaler */
typedef void scaler_expand_fn( int *x, int *y, int *w, int *h,
			       int image_width, int image_height );

extern scaler_type current_scaler;
extern ScalerProc *scaler_proc16, *scaler_proc32;
extern scaler_flags_t scaler_flags;
extern scaler_expand_fn *scaler_expander;
extern int scalers_registered;

typedef int (*scaler_available_fn)( scaler_type scaler );

int scaler_select_id( const char *scaler_mode );
void scaler_register_clear( void );
int scaler_select_scaler( scaler_type scaler );
void scaler_register( scaler_type scaler );
int scaler_is_supported( scaler_type scaler );
const char *scaler_name( scaler_type scaler );
ScalerProc *scaler_get_proc16( scaler_type scaler );
ScalerProc *scaler_get_proc32( scaler_type scaler );
scaler_flags_t scaler_get_flags( scaler_type scaler );
float scaler_get_scaling_factor( scaler_type scaler );
int scaler_scale_number( scaler_type scaler, int num );
scaler_expand_fn* scaler_get_expander( scaler_type scaler );

int scaler_select_bitformat( libspectrum_dword BitFormat );

int real2Aspect(int y);
int aspect2Real(int y);

void makeRectStretchable( int *x, int *y, int *w, int *h, int image_width,
                          int image_height );

int scaler_rect_to_ntsc( int *y, int *h, int image_width );

int stretch200To240_16( libspectrum_byte *buf, libspectrum_dword pitch,
                    int width, int height, int srcX, int srcY, int origSrcY );

int stretch200To240_32( libspectrum_byte *buf, libspectrum_dword pitch,
                    int width, int height, int srcX, int srcY, int origSrcY );

#ifdef __cplusplus
};
#endif        /* #ifdef __cplusplus */

#endif
