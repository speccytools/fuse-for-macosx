/* sound.c: Sound support
   Copyright (c) 2000-2001 Russell Marks, Matan Ziv-Av, Philip Kendall

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

#ifndef FUSE_SOUND_H
#define FUSE_SOUND_H

void sound_init(void);
void sound_end(void);
void sound_ay_write(int reg,int val);
void sound_frame(void);
void sound_beeper(int on);

extern int sound_freq,sound_channels,sound_enabled;

#endif				/* #ifndef FUSE_SOUND_H */
