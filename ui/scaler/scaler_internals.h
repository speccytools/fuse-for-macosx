/* scaler_internals.h: functions internal to the scaler code
   Copyright (c) 2003 Fredrick Meunier, Philip Kendall

   $Id$

   Originally taken from ScummVM - Scumm Interpreter
   Copyright (C) 2001  Ludvig Strigeus
   Copyright (C) 2001/2002 The ScummVM project

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

#ifndef FUSE_SCALER_INTERNALS_H
#define FUSE_SCALER_INTERNALS_H

void scaler_2xSaI_16( BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr,
		      BYTE *dstPtr, DWORD dstPitch, int width, int height );
void scaler_Super2xSaI_16( BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr,
			   BYTE *dstPtr, DWORD dstPitch,
			   int width, int height );
void scaler_SuperEagle_16( BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr,
			   BYTE *dstPtr, DWORD dstPitch,
			   int width, int height);
void scaler_AdvMame2x_16( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			  BYTE *dstPtr, DWORD dstPitch,
			  int width, int height );
void scaler_Half_16( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
	             BYTE *dstPtr, DWORD dstPitch, int width, int height );
void scaler_HalfSkip_16( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			 BYTE *dstPtr, DWORD dstPitch, int width, int height );
void scaler_Normal1x_16( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			 BYTE *dstPtr, DWORD dstPitch, int width, int height );
void scaler_Normal2x_16( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			 BYTE *dstPtr, DWORD dstPitch, int width, int height );
void scaler_Normal3x_16( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			 BYTE *dstPtr, DWORD dstPitch, int width, int height );
void scaler_TV2x_16( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
		     BYTE *dstPtr, DWORD dstPitch, int width, int height );
void scaler_TimexTV_16( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			BYTE *dstPtr, DWORD dstPitch, int width, int height );
void scaler_DotMatrix_16( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			  BYTE *dstPtr, DWORD dstPitch,
			  int width, int height );

void scaler_2xSaI_32( BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr,
		      BYTE *dstPtr, DWORD dstPitch, int width, int height );
void scaler_Super2xSaI_32( BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr,
			   BYTE *dstPtr, DWORD dstPitch,
			   int width, int height );
void scaler_SuperEagle_32( BYTE *srcPtr, DWORD srcPitch, BYTE *deltaPtr,
			   BYTE *dstPtr, DWORD dstPitch,
			   int width, int height);
void scaler_AdvMame2x_32( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			  BYTE *dstPtr, DWORD dstPitch,
			  int width, int height );
void scaler_Half_32( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
	             BYTE *dstPtr, DWORD dstPitch, int width, int height );
void scaler_HalfSkip_32( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			 BYTE *dstPtr, DWORD dstPitch, int width, int height );
void scaler_Normal1x_32( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			 BYTE *dstPtr, DWORD dstPitch, int width, int height );
void scaler_Normal2x_32( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			 BYTE *dstPtr, DWORD dstPitch, int width, int height );
void scaler_Normal3x_32( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			 BYTE *dstPtr, DWORD dstPitch, int width, int height );
void scaler_TV2x_32( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
		     BYTE *dstPtr, DWORD dstPitch, int width, int height );
void scaler_TimexTV_32( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			BYTE *dstPtr, DWORD dstPitch, int width, int height );
void scaler_DotMatrix_32( BYTE *srcPtr, DWORD srcPitch, BYTE *null,
			  BYTE *dstPtr, DWORD dstPitch,
			  int width, int height );

#endif				/* #ifndef FUSE_SCALER_INTERNALS_H */
