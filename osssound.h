/* osssound.h: OSS (e.g. Linux) sound I/O
   Copyright (c) 2000-2002 Russell Marks, Matan Ziv-Av, Philip Kendall

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

*/

#ifndef FUSE_OSSSOUND_H
#define FUSE_OSSSOUND_H

int osssound_init(const char *device,int *freqptr,int *stereoptr);
void osssound_end(void);
void osssound_frame(unsigned char *data,int len);

#endif				/* #ifndef FUSE_OSSSOUND_H */
