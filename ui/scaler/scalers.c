/* scalers.c: the actual graphics scalers
 * Copyright (C) 2003 Fredrick Meunier, Philip Kendall
 *
 * $Id$
 * 
 * Originally taken from ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001-2003 The ScummVM project
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

#include <libspectrum.h>

#include "scaler.h"
#include "scaler_internals.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"

/* The actual code for the scalers starts here */

#if SCALER_DATA_SIZE == 2

typedef libspectrum_word scaler_data_type;
#define FUNCTION( name ) name##_16

static libspectrum_dword colorMask;
static libspectrum_dword lowPixelMask;
static libspectrum_dword qcolorMask;
static libspectrum_dword qlowpixelMask;
static libspectrum_dword redblueMask;
static libspectrum_dword greenMask;

static const libspectrum_word dotmatrix_565[16] = {
  0x01E0, 0x0007, 0x3800, 0x0000,
  0x39E7, 0x0000, 0x39E7, 0x0000,
  0x3800, 0x0000, 0x01E0, 0x0007,
  0x39E7, 0x0000, 0x39E7, 0x0000
};
static const libspectrum_word dotmatrix_555[16] = {
  0x00E0, 0x0007, 0x1C00, 0x0000,
  0x1CE7, 0x0000, 0x1CE7, 0x0000,
  0x1C00, 0x0000, 0x00E0, 0x0007,
  0x1CE7, 0x0000, 0x1CE7, 0x0000
};
static const libspectrum_word *dotmatrix;

int 
scaler_select_bitformat( libspectrum_dword BitFormat )
{
  switch( BitFormat ) {

    /* FIXME(?): there is an assumption here that our colour fields
       are (*) xxxx|xyyy|yyyz|zzzz for the 565 mode
       and (*) xxxx|xyyy|yyzz|zzz0 for the 555 mode, where (*) is the
       least significant bit on LSB machines and the most significant
       bit on MSB machines. This is currently (April 2003) OK as the
       other user interface to use this code is SDL, which hides all
       variation in SDL_MapRGB(3), but be very careful (especially
       about endianness) if we ever use the "interpolating" scalers
       from another user interface */

  case 565:
    colorMask = 0x0000F7DE;
    lowPixelMask = 0x00000821;
    qcolorMask = 0x0000E79C;
    qlowpixelMask = 0x00001863;
    redblueMask = 0x0000F81F;
    greenMask = 0x000007E0;
    dotmatrix = dotmatrix_565;
    break;

  case 555:
    colorMask = 0x00007BDE;
    lowPixelMask = 0x00000421;
    qcolorMask = 0x0000739C;
    qlowpixelMask = 0x00000C63;
    redblueMask = 0x00007C1F;
    greenMask = 0x000003E0;
    dotmatrix = dotmatrix_555;
    break;

  default:
    ui_error( UI_ERROR_ERROR, "unknown bitformat %d", BitFormat );
    return 1;

  }

  return 0;
}

#elif SCALER_DATA_SIZE == 4	/* #if SCALER_DATA_SIZE == 2 */

typedef libspectrum_dword scaler_data_type;
#define FUNCTION( name ) name##_32

/* The assumption here is that the colour fields are laid out in
   memory as (LSB) red|green|blue|padding (MSB). We wish to access
   these as 32-bit entities, so make sure we get our masks the right
   way round. */

#ifdef WORDS_BIGENDIAN

const static libspectrum_dword colorMask = 0xFEFEFE00;
const static libspectrum_dword lowPixelMask = 0x01010100;
const static libspectrum_dword qcolorMask = 0xFCFCFC00;
const static libspectrum_dword qlowpixelMask = 0x03030300;
const static libspectrum_qword redblueMask = 0xFF00FF00;
const static libspectrum_qword greenMask = 0x00FF0000;

static const libspectrum_dword dotmatrix[16] = {
  0x003F0000, 0x00003F00, 0x3F000000, 0x00000000,
  0x3F3F3F00, 0x00000000, 0x3F3F3F00, 0x00000000,
  0x3F000000, 0x00000000, 0x003F0000, 0x00003F00,
  0x3F3F3F00, 0x00000000, 0x3F3F3F00, 0x00000000
};

#else				/* #ifdef WORDS_BIGENDIAN */

static const libspectrum_dword colorMask = 0x00FEFEFE;
static const libspectrum_dword lowPixelMask = 0x00010101;
static const libspectrum_dword qcolorMask = 0x00FCFCFC;
static const libspectrum_dword qlowpixelMask = 0x00030303;
static const libspectrum_qword redblueMask = 0x00FF00FF;
static const libspectrum_qword greenMask = 0x0000FF00;

static const libspectrum_dword dotmatrix[16] = {
  0x00003F00, 0x003F0000, 0x0000003F, 0x00000000,
  0x003F3F3F, 0x00000000, 0x003F3F3F, 0x00000000,
  0x0000003F, 0x00000000, 0x00003F00, 0x003F0000,
  0x003F3F3F, 0x00000000, 0x003F3F3F, 0x00000000
};

#endif				/* #ifdef WORDS_BIGENDIAN */

#else				/* #if SCALER_DATA_SIZE == 2 or 4 */
#error Unknown SCALER_DATA_SIZE
#endif				/* #if SCALER_DATA_SIZE == 2 or 4 */

static inline int 
GetResult( libspectrum_dword A, libspectrum_dword B, libspectrum_dword C,
	   libspectrum_dword D )
{
  const int ac = (A==C);
  const int bc = (B==C);
  const int x1 = ac;
  const int y1 = (bc & !ac);
  const int ad = (A==D);
  const int bd = (B==D);
  const int x2 = ad;
  const int y2 = (bd & !ad);
  const int x = x1+x2;
  const int y = y1+y2;
  static const int rmap[3][3] = {
      {0, 0, -1},
      {0, 0, -1},
      {1, 1,  0}
    };
  return rmap[y][x];
}

static inline libspectrum_dword 
INTERPOLATE( libspectrum_dword A, libspectrum_dword B )
{
  if (A != B) {
    return (((A & colorMask) >> 1) + ((B & colorMask) >> 1) + (A & B & lowPixelMask));
  } else
    return A;
}

static inline libspectrum_dword 
Q_INTERPOLATE( libspectrum_dword A, libspectrum_dword B, libspectrum_dword C,
	       libspectrum_dword D )
{
  register libspectrum_dword x = ((A & qcolorMask) >> 2) +
  ((B & qcolorMask) >> 2) + ((C & qcolorMask) >> 2) + ((D & qcolorMask) >> 2);
  register libspectrum_dword y = (A & qlowpixelMask) +
  (B & qlowpixelMask) + (C & qlowpixelMask) + (D & qlowpixelMask);

  y = (y >> 2) & qlowpixelMask;
  return x + y;
}

void 
FUNCTION( scaler_Super2xSaI )( const libspectrum_byte *srcPtr,
			       libspectrum_dword srcPitch,
			       libspectrum_byte *dstPtr,
			       libspectrum_dword dstPitch,
			       int width, int height )
{
  const scaler_data_type *bP;
  scaler_data_type *dP;

  {
    const libspectrum_dword Nextline = srcPitch / sizeof( scaler_data_type );
    libspectrum_dword nextDstLine = dstPitch / sizeof( scaler_data_type );
    libspectrum_dword finish;

    while (height--) {
      bP = (const scaler_data_type*)srcPtr;
      dP = (scaler_data_type*)dstPtr;

      for( finish = width; finish; finish-- ) {
	libspectrum_dword color4, color5, color6;
	libspectrum_dword color1, color2, color3;
	libspectrum_dword colorA0, colorA1, colorA2, colorA3, colorB0, colorB1,
	  colorB2, colorB3, colorS1, colorS2;
	libspectrum_dword product1a, product1b, product2a, product2b;

/*---------------------------------------    B1 B2
                                           4  5  6 S2
                                           1  2  3 S1
	                                     A1 A2
*/

        colorB0 = *(bP - Nextline - 1);
	colorB1 = *(bP - Nextline);
	colorB2 = *(bP - Nextline + 1);
	colorB3 = *(bP - Nextline + 2);

	color4 = *(bP - 1);
	color5 = *(bP);
	color6 = *(bP + 1);
	colorS2 = *(bP + 2);

	color1 = *(bP + Nextline - 1);
	color2 = *(bP + Nextline);
	color3 = *(bP + Nextline + 1);
	colorS1 = *(bP + Nextline + 2);

	colorA0 = *(bP + Nextline + Nextline - 1);
	colorA1 = *(bP + Nextline + Nextline);
	colorA2 = *(bP + Nextline + Nextline + 1);
	colorA3 = *(bP + Nextline + Nextline + 2);

/*--------------------------------------*/
	if (color2 == color6 && color5 != color3) {
	  product2b = product1b = color2;
	} else if (color5 == color3 && color2 != color6) {
	  product2b = product1b = color5;
	} else if (color5 == color3 && color2 == color6) {
	  register int r = 0;

	  r += GetResult(color6, color5, color1, colorA1);
	  r += GetResult(color6, color5, color4, colorB1);
	  r += GetResult(color6, color5, colorA2, colorS1);
	  r += GetResult(color6, color5, colorB2, colorS2);

	  if (r > 0)
	    product2b = product1b = color6;
	  else if (r < 0)
	    product2b = product1b = color5;
	  else {
	    product2b = product1b = INTERPOLATE(color5, color6);
	  }
	} else {
	  if (color6 == color3 && color3 == colorA1 && color2 != colorA2 && color3 != colorA0)
	    product2b = Q_INTERPOLATE(color3, color3, color3, color2);
	  else if (color5 == color2 && color2 == colorA2 && colorA1 != color3 && color2 != colorA3)
	    product2b = Q_INTERPOLATE(color2, color2, color2, color3);
	  else
	    product2b = INTERPOLATE(color2, color3);

	  if (color6 == color3 && color6 == colorB1 && color5 != colorB2 && color6 != colorB0)
	    product1b = Q_INTERPOLATE(color6, color6, color6, color5);
	  else if (color5 == color2 && color5 == colorB2 && colorB1 != color6 && color5 != colorB3)
	    product1b = Q_INTERPOLATE(color6, color5, color5, color5);
	  else
	    product1b = INTERPOLATE(color5, color6);
	}

	if (color5 == color3 && color2 != color6 && color4 == color5 && color5 != colorA2)
	  product2a = INTERPOLATE(color2, color5);
	else if (color5 == color1 && color6 == color5 && color4 != color2 && color5 != colorA0)
	  product2a = INTERPOLATE(color2, color5);
	else
	  product2a = color2;

	if (color2 == color6 && color5 != color3 && color1 == color2 && color2 != colorB2)
	  product1a = INTERPOLATE(color2, color5);
	else if (color4 == color2 && color3 == color2 && color1 != color5 && color2 != colorB0)
	  product1a = INTERPOLATE(color2, color5);
	else
	  product1a = color5;

	*dP = product1a; *(dP+nextDstLine) = product2a; dP++;
	*dP = product1b; *(dP+nextDstLine) = product2b; dP++;

	bP++;
      }				/* end of for ( finish= width etc..) */

      srcPtr += srcPitch;
      dstPtr += dstPitch * 2;
    }				/* while (height--) */
  }
}

void 
FUNCTION( scaler_SuperEagle )( const libspectrum_byte *srcPtr,
			       libspectrum_dword srcPitch,
			       libspectrum_byte *dstPtr,
			       libspectrum_dword dstPitch,
			       int width, int height )
{
  const scaler_data_type *bP;
  scaler_data_type *dP;

  {
    libspectrum_dword finish;
    const libspectrum_dword Nextline = srcPitch / sizeof( scaler_data_type );
    libspectrum_dword nextDstLine = dstPitch / sizeof( scaler_data_type );

    while (height--) {
      bP = (const scaler_data_type*)srcPtr;
      dP = (scaler_data_type*)dstPtr;
      for( finish = width; finish; finish-- ) {
	libspectrum_dword color4, color5, color6;
	libspectrum_dword color1, color2, color3;
	libspectrum_dword colorA1, colorA2, colorB1, colorB2, colorS1, colorS2;
	libspectrum_dword product1a, product1b, product2a, product2b;

	colorB1 = *(bP - Nextline);
	colorB2 = *(bP - Nextline + 1);

	color4 = *(bP - 1);
	color5 = *(bP);
	color6 = *(bP + 1);
	colorS2 = *(bP + 2);

	color1 = *(bP + Nextline - 1);
	color2 = *(bP + Nextline);
	color3 = *(bP + Nextline + 1);
	colorS1 = *(bP + Nextline + 2);

	colorA1 = *(bP + Nextline + Nextline);
	colorA2 = *(bP + Nextline + Nextline + 1);

	/* -------------------------------------- */
	if (color2 == color6 && color5 != color3) {
	  product1b = product2a = color2;
	  if ((color1 == color2) || (color6 == colorB2)) {
	    product1a = INTERPOLATE(color2, color5);
	    product1a = INTERPOLATE(color2, product1a);
	  } else {
	    product1a = INTERPOLATE(color5, color6);
	  }

	  if ((color6 == colorS2) || (color2 == colorA1)) {
	    product2b = INTERPOLATE(color2, color3);
	    product2b = INTERPOLATE(color2, product2b);
	  } else {
	    product2b = INTERPOLATE(color2, color3);
	  }
	} else if (color5 == color3 && color2 != color6) {
	  product2b = product1a = color5;

	  if ((colorB1 == color5) || (color3 == colorS1)) {
	    product1b = INTERPOLATE(color5, color6);
	    product1b = INTERPOLATE(color5, product1b);
	  } else {
	    product1b = INTERPOLATE(color5, color6);
	  }

	  if ((color3 == colorA2) || (color4 == color5)) {
	    product2a = INTERPOLATE(color5, color2);
	    product2a = INTERPOLATE(color5, product2a);
	  } else {
	    product2a = INTERPOLATE(color2, color3);
	  }

	} else if (color5 == color3 && color2 == color6) {
	  register int r = 0;

	  r += GetResult(color6, color5, color1, colorA1);
	  r += GetResult(color6, color5, color4, colorB1);
	  r += GetResult(color6, color5, colorA2, colorS1);
	  r += GetResult(color6, color5, colorB2, colorS2);

	  if (r > 0) {
	    product1b = product2a = color2;
	    product1a = product2b = INTERPOLATE(color5, color6);
	  } else if (r < 0) {
	    product2b = product1a = color5;
	    product1b = product2a = INTERPOLATE(color5, color6);
	  } else {
	    product2b = product1a = color5;
	    product1b = product2a = color2;
	  }
	} else {
	  product2b = product1a = INTERPOLATE(color2, color6);
	  product2b = Q_INTERPOLATE(color3, color3, color3, product2b);
	  product1a = Q_INTERPOLATE(color5, color5, color5, product1a);

	  product2a = product1b = INTERPOLATE(color5, color3);
	  product2a = Q_INTERPOLATE(color2, color2, color2, product2a);
	  product1b = Q_INTERPOLATE(color6, color6, color6, product1b);
	}

	*dP = product1a; *(dP+nextDstLine) = product2a; dP++;
	*dP = product1b; *(dP+nextDstLine) = product2b; dP++;

	bP++;
      }				/* end of for ( finish= width etc..) */

      srcPtr += srcPitch;
      dstPtr += dstPitch * 2;
    }				/* endof: while (height--) */
  }
}

void 
FUNCTION( scaler_2xSaI )( const libspectrum_byte *srcPtr,
			  libspectrum_dword srcPitch, libspectrum_byte *dstPtr,
			  libspectrum_dword dstPitch,
			  int width, int height )
{
  const scaler_data_type *bP;
  scaler_data_type *dP;

  {
    libspectrum_dword Nextline = srcPitch / sizeof( scaler_data_type );
    libspectrum_dword nextDstLine = dstPitch / sizeof( scaler_data_type );

    while (height--) {
      libspectrum_dword finish;
      bP = (const scaler_data_type*)srcPtr;
      dP = (scaler_data_type*)dstPtr;

      for( finish = width; finish; finish-- ) {

	register libspectrum_dword colorA, colorB;
	libspectrum_dword colorC, colorD, colorE, colorF, colorG, colorH,
	  colorI, colorJ, colorK, colorL, colorM, colorN, colorO, colorP;
	libspectrum_dword product, product1, product2;

/*---------------------------------------
   Map of the pixels:                    I|E F|J
                                         G|A B|K
                                         H|C D|L
                                         M|N O|P
*/
	colorI = *(bP - Nextline - 1);
	colorE = *(bP - Nextline);
	colorF = *(bP - Nextline + 1);
	colorJ = *(bP - Nextline + 2);

	colorG = *(bP - 1);
	colorA = *(bP);
	colorB = *(bP + 1);
	colorK = *(bP + 2);

	colorH = *(bP + Nextline - 1);
	colorC = *(bP + Nextline);
	colorD = *(bP + Nextline + 1);
	colorL = *(bP + Nextline + 2);

	colorM = *(bP + Nextline + Nextline - 1);
	colorN = *(bP + Nextline + Nextline);
	colorO = *(bP + Nextline + Nextline + 1);
	colorP = *(bP + Nextline + Nextline + 2);

	if ((colorA == colorD) && (colorB != colorC)) {
	  if (((colorA == colorE) && (colorB == colorL)) || ((colorA == colorC) && (colorA == colorF)
						       && (colorB != colorE)
						   && (colorB == colorJ))) {
	    product = colorA;
	  } else {
	    product = INTERPOLATE(colorA, colorB);
	  }

	  if (((colorA == colorG) && (colorC == colorO)) || ((colorA == colorB) && (colorA == colorH)
						       && (colorG != colorC)
						   && (colorC == colorM))) {
	    product1 = colorA;
	  } else {
	    product1 = INTERPOLATE(colorA, colorC);
	  }
	  product2 = colorA;
	} else if ((colorB == colorC) && (colorA != colorD)) {
	  if (((colorB == colorF) && (colorA == colorH)) || ((colorB == colorE) && (colorB == colorD)
						       && (colorA != colorF)
						   && (colorA == colorI))) {
	    product = colorB;
	  } else {
	    product = INTERPOLATE(colorA, colorB);
	  }

	  if (((colorC == colorH) && (colorA == colorF)) || ((colorC == colorG) && (colorC == colorD)
						       && (colorA != colorH)
						   && (colorA == colorI))) {
	    product1 = colorC;
	  } else {
	    product1 = INTERPOLATE(colorA, colorC);
	  }
	  product2 = colorB;
	} else if ((colorA == colorD) && (colorB == colorC)) {
	  if (colorA == colorB) {
	    product = colorA;
	    product1 = colorA;
	    product2 = colorA;
	  } else {
	    register int r = 0;

	    product1 = INTERPOLATE(colorA, colorC);
	    product = INTERPOLATE(colorA, colorB);

	    r += GetResult(colorA, colorB, colorG, colorE);
	    r -= GetResult(colorB, colorA, colorK, colorF);
	    r -= GetResult(colorB, colorA, colorH, colorN);
	    r += GetResult(colorA, colorB, colorL, colorO);

	    if (r > 0)
	      product2 = colorA;
	    else if (r < 0)
	      product2 = colorB;
	    else {
	      product2 = Q_INTERPOLATE(colorA, colorB, colorC, colorD);
	    }
	  }
	} else {
	  product2 = Q_INTERPOLATE(colorA, colorB, colorC, colorD);

	  if ((colorA == colorC) && (colorA == colorF)
	      && (colorB != colorE) && (colorB == colorJ)) {
	    product = colorA;
	  } else if ((colorB == colorE) && (colorB == colorD)
		     && (colorA != colorF) && (colorA == colorI)) {
	    product = colorB;
	  } else {
	    product = INTERPOLATE(colorA, colorB);
	  }

	  if ((colorA == colorB) && (colorA == colorH)
	      && (colorG != colorC) && (colorC == colorM)) {
	    product1 = colorA;
	  } else if ((colorC == colorG) && (colorC == colorD)
		     && (colorA != colorH) && (colorA == colorI)) {
	    product1 = colorC;
	  } else {
	    product1 = INTERPOLATE(colorA, colorC);
	  }
	}

	*dP = colorA; *(dP+nextDstLine) = product1; dP++;
	*dP = product; *(dP+nextDstLine) = product2; dP++;

	bP++;
      }				/* end of for ( finish= width etc..) */

      srcPtr += srcPitch;
      dstPtr += dstPitch * 2;
    }				/* endof: while (height--) */
  }
}

void 
FUNCTION( scaler_AdvMame2x )( const libspectrum_byte *srcPtr,
			      libspectrum_dword srcPitch,
			      libspectrum_byte *dstPtr,
			      libspectrum_dword dstPitch,
			      int width, int height )
{
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  const scaler_data_type *p = (const scaler_data_type*) srcPtr;

  unsigned int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q = (scaler_data_type*) dstPtr;

  scaler_data_type /* A, */ B, C;
  scaler_data_type D, E, F;
  scaler_data_type /* G, */ H, I;

  while (height--) {
    int i;

    B = *(p - 1 - nextlineSrc);
    E = *(p - 1);
    H = *(p - 1 + nextlineSrc);
    C = *(p - nextlineSrc);
    F = *(p);
    I = *(p + nextlineSrc);

    for (i = 0; i < width; ++i) {
      p++;
      /* A = B; */ B = C; C = *(p - nextlineSrc);
      D = E; E = F; F = *(p);
      /* G = H; */ H = I; I = *(p + nextlineSrc);

      *(q) = D == B && B != F && D != H ? D : E;
      *(q + 1) = B == F && B != D && F != H ? F : E;
      *(q + nextlineDst) = D == H && D != B && H != F ? D : E;
      *(q + nextlineDst + 1) = H == F && D != H && B != F ? F : E;
      q += 2;
    }
    p += nextlineSrc - width;
    q += (nextlineDst - width) << 1;
  }
}

void 
FUNCTION( scaler_AdvMame3x )( const libspectrum_byte *srcPtr,
			      libspectrum_dword srcPitch,
			      libspectrum_byte *dstPtr,
			      libspectrum_dword dstPitch,
			      int width, int height )
{
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  const scaler_data_type *p = (const scaler_data_type*) srcPtr;

  unsigned int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q = (scaler_data_type*) dstPtr;

  scaler_data_type /* A, */ B, C;
  scaler_data_type D, E, F;
  scaler_data_type /* G, */ H, I;

  while (height--) {
    int i;

    B = *(p - 1 - nextlineSrc);
    E = *(p - 1);
    H = *(p - 1 + nextlineSrc);
    C = *(p - nextlineSrc);
    F = *(p);
    I = *(p + nextlineSrc);

    for (i = 0; i < width; ++i) {
      p++;
      /* A = B; */ B = C; C = *(p - nextlineSrc);
      D = E; E = F; F = *(p);
      /* G = H; */ H = I; I = *(p + nextlineSrc);

      *(q) = D == B && B != F && D != H ? D : E;
      *(q + 1) = E;
      *(q + 2) = B == F && B != D && F != H ? F : E;
      *(q + nextlineDst) = E;
      *(q + nextlineDst + 1) = E;
      *(q + nextlineDst + 2) = E;
      *(q + 2 * nextlineDst) = D == H && D != B && H != F ? D : E;
      *(q + 2 * nextlineDst + 1) = E;
      *(q + 2 * nextlineDst + 2) = H == F && D != H && B != F ? F : E;
      q += 3;
    }
    p += nextlineSrc - width;
    q += (nextlineDst - width) * 3;
  }
}

void 
FUNCTION( scaler_Half )( const libspectrum_byte *srcPtr,
			 libspectrum_dword srcPitch, libspectrum_byte *dstPtr,
			 libspectrum_dword dstPitch,
			 int width, int height )
{
  scaler_data_type *r;

  while (height--) {
    int i;
    r = (scaler_data_type*) dstPtr;

    if( ( height & 1 ) == 0 ) {
      for (i = 0; i < width; i+=2, ++r) {
        scaler_data_type color1 = *(((scaler_data_type*) srcPtr) + i);
        scaler_data_type color2 = *(((scaler_data_type*) srcPtr) + i + 1);

        *r = INTERPOLATE(color1, color2);
      }
      dstPtr += dstPitch;
    }

    srcPtr += srcPitch;
  }
}

void 
FUNCTION( scaler_HalfSkip )( const libspectrum_byte *srcPtr,
			     libspectrum_dword srcPitch,
			     libspectrum_byte *dstPtr,
			     libspectrum_dword dstPitch,
			     int width, int height )
{
  scaler_data_type *r;

  while (height--) {
    int i;
    r = (scaler_data_type*) dstPtr;

    if( ( height & 1 ) == 0 ) {
      for (i = 0; i < width; i+=2, ++r) {
        *r = *(((const scaler_data_type*) srcPtr) + i + 1);
      }
      dstPtr += dstPitch;
    }

    srcPtr += srcPitch;
  }
}

void 
FUNCTION( scaler_Normal1x )( const libspectrum_byte *srcPtr,
			     libspectrum_dword srcPitch,
			     libspectrum_byte *dstPtr,
			     libspectrum_dword dstPitch,
			     int width, int height )
{
  while( height-- ) {
    memcpy( dstPtr, srcPtr, SCALER_DATA_SIZE * width );
    srcPtr += srcPitch;
    dstPtr += dstPitch;
  }
}

void 
FUNCTION( scaler_Normal2x )( const libspectrum_byte *srcPtr,
			     libspectrum_dword srcPitch,
			     libspectrum_byte *dstPtr,
			     libspectrum_dword dstPitch,
			     int width, int height )
{
  const scaler_data_type *s;
  scaler_data_type i, *d, *d2;

  while( height-- ) {

    for( i = 0, s = (const scaler_data_type*)srcPtr,
	   d = (scaler_data_type*)dstPtr,
	   d2 = (scaler_data_type*)(dstPtr + dstPitch);
	 i < width;
	 i++ ) {
      *d++ = *d2++ = *s; *d++ = *d2++ = *s++;
    }

    srcPtr += srcPitch;
    dstPtr += dstPitch << 1;
  }
}

void 
FUNCTION( scaler_Normal3x )( const libspectrum_byte *srcPtr,
			     libspectrum_dword srcPitch,
			     libspectrum_byte *dstPtr,
			     libspectrum_dword dstPitch,
			     int width, int height )
{
  libspectrum_byte *r;
  libspectrum_dword dstPitch2 = dstPitch * 2;
  libspectrum_dword dstPitch3 = dstPitch * 3;

  while (height--) {
    int i;
    r = dstPtr;
    for (i = 0; i < width; ++i, r += 3 * SCALER_DATA_SIZE ) {
      scaler_data_type color = *(((const scaler_data_type*) srcPtr) + i);

      *(scaler_data_type*)( r +                    0             ) = color;
      *(scaler_data_type*)( r +     SCALER_DATA_SIZE             ) = color;
      *(scaler_data_type*)( r + 2 * SCALER_DATA_SIZE             ) = color;
      *(scaler_data_type*)( r +                    0 + dstPitch  ) = color;
      *(scaler_data_type*)( r +     SCALER_DATA_SIZE + dstPitch  ) = color;
      *(scaler_data_type*)( r + 2 * SCALER_DATA_SIZE + dstPitch  ) = color;
      *(scaler_data_type*)( r +                    0 + dstPitch2 ) = color;
      *(scaler_data_type*)( r +     SCALER_DATA_SIZE + dstPitch2 ) = color;
      *(scaler_data_type*)( r + 2 * SCALER_DATA_SIZE + dstPitch2 ) = color;
    }
    srcPtr += srcPitch;
    dstPtr += dstPitch3;
  }
}

void
FUNCTION( scaler_TV2x )( const libspectrum_byte *srcPtr,
			 libspectrum_dword srcPitch, libspectrum_byte *dstPtr,
			 libspectrum_dword dstPitch,
			 int width, int height )
{
  int i, j;
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  const scaler_data_type *p = (const scaler_data_type*)srcPtr;

  unsigned int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q = (scaler_data_type*)dstPtr;

  while(height--) {
    for (i = 0, j = 0; i < width; ++i, j += 2) {
      scaler_data_type p1 = *(p + i);
      scaler_data_type pi;

      pi  = (((p1 & redblueMask) * 7) >> 3) & redblueMask;
      pi |= (((p1 & greenMask  ) * 7) >> 3) & greenMask;

      *(q + j) = p1;
      *(q + j + 1) = p1;
      *(q + j + nextlineDst) = pi;
      *(q + j + nextlineDst + 1) = pi;
    }
    p += nextlineSrc;
    q += nextlineDst << 1;
  }
}

void
FUNCTION( scaler_TimexTV )( const libspectrum_byte *srcPtr,
			    libspectrum_dword srcPitch,
			    libspectrum_byte *dstPtr,
			    libspectrum_dword dstPitch,
			    int width, int height )
{
  int i, j;
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  const scaler_data_type *p = (const scaler_data_type *)srcPtr;

  unsigned int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q = (scaler_data_type *)dstPtr;

  while(height--) {
    if( ( height & 1 ) == 0 ) {
      for (i = 0, j = 0; i < width; ++i, j++ ) {
        scaler_data_type p1 = *(p + i);
        scaler_data_type pi;

	pi  = (((p1 & redblueMask) * 7) >> 3) & redblueMask;
	pi |= (((p1 & greenMask  ) * 7) >> 3) & greenMask;

        *(q + j) = p1;
        *(q + j + nextlineDst) = pi;
      }
      q += nextlineDst << 1;
    }
    p += nextlineSrc;
  }
}

static inline scaler_data_type DOT_16(scaler_data_type c, int j, int i) {
  return c - ((c >> 2) & *(dotmatrix + ((j & 3) << 2) + (i & 3)));
}

void
FUNCTION( scaler_DotMatrix )( const libspectrum_byte *srcPtr,
			      libspectrum_dword srcPitch,
			      libspectrum_byte *dstPtr,
			      libspectrum_dword dstPitch,
			      int width, int height )
{
  int i, j, ii, jj;
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  const scaler_data_type *p = (const scaler_data_type *)srcPtr;

  unsigned int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q = (scaler_data_type *)dstPtr;

  for (j = 0, jj = 0; j < height; ++j, jj += 2) {
    for (i = 0, ii = 0; i < width; ++i, ii += 2) {
      scaler_data_type c = *(p + i);
      *(q + ii) = DOT_16(c, jj, ii);
      *(q + ii + 1) = DOT_16(c, jj, ii + 1);
      *(q + ii + nextlineDst) = DOT_16(c, jj + 1, ii);
      *(q + ii + nextlineDst + 1) = DOT_16(c, jj + 1, ii + 1);
    }
    p += nextlineSrc;
    q += nextlineDst << 1;
  }
}
