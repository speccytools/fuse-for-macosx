/* types.h: Define 8, 16 and 32-bit types
   Copyright (c) 1999-2000 Philip Kendall

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

#ifndef FUSE_TYPES_H
#define FUSE_TYPES_H

#if   SIZEOF_CHAR  == 1
typedef   signed  char SBYTE;
typedef unsigned  char  BYTE;
#elif SIZEOF_SHORT == 1
typedef   signed short SBYTE
typedef unsigned short  BYTE;
#else
#error No plausible 8 bit types found
#endif

#if   SIZEOF_SHORT == 2
typedef   signed short SWORD;
typedef unsigned short  WORD;
#elif SIZEOF_INT   == 2
typedef   signed   int SWORD;
typedef unsigned   int  WORD;
#else
#error No plausible 16 bit types found
#endif

#if   SIZEOF_INT   == 4
typedef   signed  int SDWORD;
typedef unsigned  int  DWORD;
#elif SIZEOF_LONG  == 4
typedef   signed long SDWORD;
typedef unsigned long  DWORD;
#else
#error No plausible 32 bit types found
#endif

#if   SIZEOF_INT   == 8
typedef   signed  int SQWORD;
typedef unsigned  int  QWORD;
#elif SIZEOF_LONG  == 8
typedef   signed long SQWORD;
typedef unsigned long  QWORD;
#else
#error No plausible 64 bit types found
#endif

#endif			/* #ifndef FUSE_TYPES_H */
