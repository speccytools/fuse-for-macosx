/* sound.h: Sound support
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

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

#ifndef FUSE_SOUND_H
#define FUSE_SOUND_H

void sound_init(const char *device);
void sound_end(void);
void sound_ay_write(int reg,int val,DWORD now);
void sound_ay_reset(void);
void sound_frame(void);
void sound_beeper(int is_tape,int on);

extern int sound_enabled;
extern int sound_freq;
extern int sound_stereo;
extern int sound_stereo_beeper;
extern int sound_stereo_ay;
extern int sound_stereo_ay_abc;
extern int sound_stereo_ay_narrow;

#endif				/* #ifndef FUSE_SOUND_H */
