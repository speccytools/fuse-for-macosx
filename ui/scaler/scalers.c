/* scalers.c: the actual graphics scalers
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
#include "settings.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"

/* The actual code for the scalers starts here */

#if SCALER_DATA_SIZE == 2

typedef WORD scaler_data_type;
#define FUNCTION( name ) name##_16

static DWORD colorMask;
static DWORD lowPixelMask;
static DWORD qcolorMask;
static DWORD qlowpixelMask;
static DWORD redblueMask;
static DWORD greenMask;

static const WORD dotmatrix_565[16] = {
  0x01E0, 0x0007, 0x3800, 0x0000,
  0x39E7, 0x0000, 0x39E7, 0x0000,
  0x3800, 0x0000, 0x01E0, 0x0007,
  0x39E7, 0x0000, 0x39E7, 0x0000
};
static const WORD dotmatrix_555[16] = {
  0x00E0, 0x0007, 0x1C00, 0x0000,
  0x1CE7, 0x0000, 0x1CE7, 0x0000,
  0x1C00, 0x0000, 0x00E0, 0x0007,
  0x1CE7, 0x0000, 0x1CE7, 0x0000
};
static const WORD *dotmatrix;

int 
scaler_select_bitformat( DWORD BitFormat )
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

typedef DWORD scaler_data_type;
#define FUNCTION( name ) name##_32

/* The assumption here is that the colour fields are laid out in
   memory as (LSB) red|green|blue|padding (MSB). We wish to access
   these as 32-bit entities, so make sure we get our masks the right
   way round. */

#ifdef WORDS_BIGENDIAN

const static DWORD colorMask = 0xFEFEFE00;
const static DWORD lowPixelMask = 0x01010100;
const static DWORD qcolorMask = 0xFCFCFC00;
const static DWORD qlowpixelMask = 0x03030300;
const static QWORD redblueMask = 0xFF00FF00;
const static QWORD greenMask = 0x00FF0000;

static const DWORD dotmatrix[16] = {
  0x003F0000, 0x00003F00, 0x3F000000, 0x00000000,
  0x3F3F3F00, 0x00000000, 0x3F3F3F00, 0x00000000,
  0x3F000000, 0x00000000, 0x003F0000, 0x00003F00,
  0x3F3F3F00, 0x00000000, 0x3F3F3F00, 0x00000000
};

#else				/* #ifdef WORDS_BIGENDIAN */

const static DWORD colorMask = 0x00FEFEFE;
const static DWORD lowPixelMask = 0x00010101;
const static DWORD qcolorMask = 0x00FCFCFC;
const static DWORD qlowpixelMask = 0x00030303;
const static QWORD redblueMask = 0x00FF00FF;
const static QWORD greenMask = 0x0000FF00;

static const DWORD dotmatrix[16] = {
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
GetResult1(DWORD A, DWORD B, DWORD C, DWORD D, DWORD E)
{
  int x = 0;
  int y = 0;
  int r = 0;

  if (A == C)
    x += 1;
  else if (B == C)
    y += 1;
  if (A == D)
    x += 1;
  else if (B == D)
    y += 1;
  if (x <= 1)
    r += 1;
  if (y <= 1)
    r -= 1;
  return r;
}

static inline int 
GetResult2(DWORD A, DWORD B, DWORD C, DWORD D, DWORD E)
{
  int x = 0;
  int y = 0;
  int r = 0;

  if (A == C)
    x += 1;
  else if (B == C)
    y += 1;
  if (A == D)
    x += 1;
  else if (B == D)
    y += 1;
  if (x <= 1)
    r -= 1;
  if (y <= 1)
    r += 1;
  return r;
}

static inline int 
GetResult(DWORD A, DWORD B, DWORD C, DWORD D)
{
  int x = 0;
  int y = 0;
  int r = 0;

  if (A == C)
    x += 1;
  else if (B == C)
    y += 1;
  if (A == D)
    x += 1;
  else if (B == D)
    y += 1;
  if (x <= 1)
    r += 1;
  if (y <= 1)
    r -= 1;
  return r;
}

static inline DWORD 
INTERPOLATE(DWORD A, DWORD B)
{
  if (A != B) {
    return (((A & colorMask) >> 1) + ((B & colorMask) >> 1) + (A & B & lowPixelMask));
  } else
    return A;
}

static inline DWORD 
Q_INTERPOLATE(DWORD A, DWORD B, DWORD C, DWORD D)
{
  register DWORD x = ((A & qcolorMask) >> 2) +
  ((B & qcolorMask) >> 2) + ((C & qcolorMask) >> 2) + ((D & qcolorMask) >> 2);
  register DWORD y = (A & qlowpixelMask) +
  (B & qlowpixelMask) + (C & qlowpixelMask) + (D & qlowpixelMask);

  y = (y >> 2) & qlowpixelMask;
  return x + y;
}

void 
FUNCTION( scaler_Super2xSaI )( BYTE *srcPtr, DWORD srcPitch,
	BYTE *deltaPtr, BYTE *dstPtr, DWORD dstPitch, int width, int height)
{
  scaler_data_type *bP, *dP;

  {
    DWORD Nextline = srcPitch / sizeof( scaler_data_type );
    DWORD nextDstLine = dstPitch / sizeof( scaler_data_type );
    DWORD finish;

    while (height--) {
      bP = (scaler_data_type*)srcPtr;
      dP = (scaler_data_type*)dstPtr;

      for( finish = width; finish; finish-- ) {
	DWORD color4, color5, color6;
	DWORD color1, color2, color3;
	DWORD colorA0, colorA1, colorA2, colorA3, colorB0, colorB1, colorB2,
	 colorB3, colorS1, colorS2;
	DWORD product1a, product1b, product2a, product2b;

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
      deltaPtr += srcPitch;
    }				/* while (height--) */
  }
}

void 
FUNCTION( scaler_SuperEagle )( BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr,
			       BYTE *dstPtr, DWORD dstPitch,
			       int width, int height )
{
  scaler_data_type *bP, *dP;

  {
    DWORD finish;
    DWORD Nextline = srcPitch / sizeof( scaler_data_type );
    DWORD nextDstLine = dstPitch / sizeof( scaler_data_type );

    while (height--) {
      bP = (scaler_data_type*)srcPtr;
      dP = (scaler_data_type*)dstPtr;
      for( finish = width; finish; finish-- ) {
	DWORD color4, color5, color6;
	DWORD color1, color2, color3;
	DWORD colorA1, colorA2, colorB1, colorB2, colorS1, colorS2;
	DWORD product1a, product1b, product2a, product2b;

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
      deltaPtr += srcPitch;
    }				/* endof: while (height--) */
  }
}

void 
FUNCTION( scaler_2xSaI )( BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr,
			  BYTE *dstPtr, DWORD dstPitch, int width, int height )
{
  scaler_data_type *bP, *dP;

  {
    DWORD Nextline = srcPitch / sizeof( scaler_data_type );
    DWORD nextDstLine = dstPitch / sizeof( scaler_data_type );

    while (height--) {
      DWORD finish;
      bP = (scaler_data_type*)srcPtr;
      dP = (scaler_data_type*)dstPtr;

      for( finish = width; finish; finish-- ) {

	register DWORD colorA, colorB;
	DWORD colorC, colorD, colorE, colorF, colorG, colorH, colorI, colorJ,
	 colorK, colorL, colorM, colorN, colorO, colorP;
	DWORD product, product1, product2;

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

	    r += GetResult1(colorA, colorB, colorG, colorE, colorI);
	    r += GetResult2(colorB, colorA, colorK, colorF, colorJ);
	    r += GetResult2(colorB, colorA, colorH, colorN, colorM);
	    r += GetResult1(colorA, colorB, colorL, colorO, colorP);

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
      deltaPtr += srcPitch;
    }				/* endof: while (height--) */
  }
}

void 
FUNCTION( scaler_AdvMame2x )( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			      BYTE *dstPtr, DWORD dstPitch,
			      int width, int height )
{
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  scaler_data_type *p = (scaler_data_type*) srcPtr;

  unsigned nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q = (scaler_data_type*) dstPtr;

  while (height--) {
    int i;
    for (i = 0; i < width; ++i) {
      /* short A = *(p + i - nextlineSrc - 1); */
      scaler_data_type B = *(p + i - nextlineSrc);
      /* short C = *(p + i - nextlineSrc + 1); */
      scaler_data_type D = *(p + i - 1);
      scaler_data_type E = *(p + i);
      scaler_data_type F = *(p + i + 1);
      /* short G = *(p + i + nextlineSrc - 1); */
      scaler_data_type H = *(p + i + nextlineSrc);
      /* short I = *(p + i + nextlineSrc + 1); */

      *(q + (i << 1)) = D == B && B != F && D != H ? D : E;
      *(q + (i << 1) + 1) = B == F && B != D && F != H ? F : E;
      *(q + (i << 1) + nextlineDst) = D == H && D != B && H != F ? D : E;
      *(q + (i << 1) + nextlineDst + 1) = H == F && D != H && B != F ? F : E;
    }
    p += nextlineSrc;
    q += nextlineDst << 1;
  }
}

void 
FUNCTION( scaler_Half )( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			 BYTE *dstPtr, DWORD dstPitch, int width, int height )
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
FUNCTION( scaler_HalfSkip )( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			     BYTE *dstPtr, DWORD dstPitch, int width,
			     int height )
{
  scaler_data_type *r;

  while (height--) {
    int i;
    r = (scaler_data_type*) dstPtr;

    if( ( height & 1 ) == 0 ) {
      for (i = 0; i < width; i+=2, ++r) {
        *r = *(((scaler_data_type*) srcPtr) + i + 1);
      }
      dstPtr += dstPitch;
    }

    srcPtr += srcPitch;
  }
}

void 
FUNCTION( scaler_Normal1x )( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			     BYTE *dstPtr, DWORD dstPitch,
			     int width, int height )
{
  while( height-- ) {
    memcpy( dstPtr, srcPtr, SCALER_DATA_SIZE * width );
    srcPtr += srcPitch;
    dstPtr += dstPitch;
  }
}

void 
FUNCTION( scaler_Normal2x )( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			     BYTE *dstPtr, DWORD dstPitch,
			     int width, int height )
{
  scaler_data_type i, *s, *d, *d2;

  while( height-- ) {

    for( i = 0, s = (scaler_data_type*)srcPtr,
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
FUNCTION( scaler_Normal3x )( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			     BYTE *dstPtr, DWORD dstPitch,
			     int width, int height )
{
  BYTE *r;
  DWORD dstPitch2 = dstPitch * 2;
  DWORD dstPitch3 = dstPitch * 3;

  while (height--) {
    int i;
    r = dstPtr;
    for (i = 0; i < width; ++i, r += 6) {
      WORD color = *(((WORD *) srcPtr) + i);

      *(WORD *) (r + 0) = color;
      *(WORD *) (r + 2) = color;
      *(WORD *) (r + 4) = color;
      *(WORD *) (r + 0 + dstPitch) = color;
      *(WORD *) (r + 2 + dstPitch) = color;
      *(WORD *) (r + 4 + dstPitch) = color;
      *(WORD *) (r + 0 + dstPitch2) = color;
      *(WORD *) (r + 2 + dstPitch2) = color;
      *(WORD *) (r + 4 + dstPitch2) = color;
    }
    srcPtr += srcPitch;
    dstPtr += dstPitch3;
  }
}

void
FUNCTION( scaler_TV2x )( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			 BYTE *dstPtr, DWORD dstPitch, int width, int height )
{
  int i, j;
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  scaler_data_type *p = (scaler_data_type*)srcPtr;

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
FUNCTION( scaler_TimexTV )( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			    BYTE *dstPtr, DWORD dstPitch,
			    int width, int height )
{
  int i, j;
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  scaler_data_type *p = (scaler_data_type *)srcPtr;

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

static inline scaler_data_type DOT(scaler_data_type c, int j, int i) {
  return c - ((c >> 2) & *(dotmatrix + ((j & 3) << 2) + (i & 3)));
}

void
FUNCTION( scaler_DotMatrix )( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			    BYTE *dstPtr, DWORD dstPitch,
			    int width, int height )
{
  int i, j, ii, jj;
  unsigned int nextlineSrc = srcPitch / sizeof( scaler_data_type );
  scaler_data_type *p = (scaler_data_type *)srcPtr;

  unsigned int nextlineDst = dstPitch / sizeof( scaler_data_type );
  scaler_data_type *q = (scaler_data_type *)dstPtr;

  for (j = 0, jj = 0; j < height; ++j, jj += 2) {
    for (i = 0, ii = 0; i < width; ++i, ii += 2) {
      scaler_data_type c = *(p + i);
      *(q + ii) = DOT(c, jj, ii);
      *(q + ii + 1) = DOT(c, jj, ii + 1);
      *(q + ii + nextlineDst) = DOT(c, jj + 1, ii);
      *(q + ii + nextlineDst + 1) = DOT(c, jj + 1, ii + 1);
    }
    p += nextlineSrc;
    q += nextlineDst << 1;
  }
}
