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

/* 48/128 sound support by Russell Marks */

/* some AY details (volume levels, white noise RNG algorithm) based on
 * info from MAME's ay8910.c - MAME's licence explicitly permits free
 * use of info (even encourages it).
 */

/* NB: I know some of this stuff looks fairly CPU-hogging.
 * For example, there's some use of floating-point in places
 * (though I've tried to minimise this), and the AY code tracks
 * changes with sub-frame timing in a rather hairy way.
 * But if you measure the CPU use, it doesn't actually seem
 * very high at all. And I speak as a Cyrix owner. :-)
 */

/* XXX TODO: 128 sound subsampling. Doesn't need the excruciating
 * exactness of the 48 code (multiple writes per sample dealt with
 * correctly (I hope :-))), but subsampling one edge per channel
 * per sample correctly is at least worth trying out.
 */

#include <config.h>

#include "sound.h"
#include "spectrum.h"

/* configuration */
int sound_freq=32000;
int sound_channels=1;	/* 1=mono, 2=ACB stereo */
int sound_enabled=1;


#if !defined( HAVE_SYS_SOUNDCARD_H )

/* (mostly) dummy routines */

void sound_init(void)
{
  sound_enabled=0;
}

void sound_frame(void)
{
}

void sound_end(void)
{
}

void sound_beeper(int on)
{
}

void sound_ay_write(int reg,int val)
{
}

#else	/* HAVE_SYS_SOUNDCARD_H */

/* OSS sound for e.g. Linux */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/soundcard.h>


#define AY_CLOCK		1773400

/* assume all three tone channels together match the beeper volume.
 * Must be <=127 for all channels; 4 x 31 = 124.
 */
#define AMPL_BEEPER		31
#define AMPL_AY_TONE		31	/* three of these */

/* full range of beeper volume */
#define VOL_BEEPER		(AMPL_BEEPER*2)

/* using 256 byte frags for 8kHz, scale up for higher */
#define BASE_SOUND_FRAG_PWR	8

/* max. number of sub-frame AY port writes allowed;
 * given the number of port writes theoretically possible in a
 * 50th I think this should be plenty.
 */
#define AY_CHANGE_MAX		8000

static int soundfd=-1;
static int sound_framesiz;

static int stereo;

static unsigned char ay_tone_levels[16];

static unsigned char *sound_buf,*sound_ptr;
static int sound_oldpos,sound_fillpos,sound_oldval;

static double ay_tone_tick[3],ay_noise_tick,ay_env_tick;
static double ay_tick_incr;
static int ay_tone_period[3],ay_noise_period,ay_env_period;
static int env_held=0,env_alternating=0;

static int beeper_last_subpos=0;
static int beeper_first_write=1;

/* Local copy of the AY registers */
static BYTE sound_ay_registers[15];

struct ay_change_tag
  {
  DWORD tstates;
  unsigned short ofs;
  unsigned char reg,val;
  };

static struct ay_change_tag ay_change[AY_CHANGE_MAX];
static int ay_change_count;



void sound_ay_init(void)
{
int f;
double v;

/* logarithmic volume levels, 3dB per step */
v=AMPL_AY_TONE;
for(f=15;f>0;f--)
  {
  ay_tone_levels[f]=(int)(v+0.5);
  /* 10^3/20 = 3dB */
  v/=1.4125375446;
  }
ay_tone_levels[0]=0;

ay_noise_tick=ay_noise_period=0;
ay_env_tick=ay_env_period=0;
for(f=0;f<3;f++)
  ay_tone_tick[f]=ay_tone_period[f]=0;

ay_tick_incr=(double)AY_CLOCK/sound_freq;

ay_change_count=0;
}


void sound_init(void)
{
int frag,tmp;

if(!sound_enabled) return;

sound_oldval=128;
sound_oldpos=sound_fillpos=0;
sound_ptr=sound_buf;

if((soundfd=open("/dev/dsp",O_WRONLY))<0)
  {
  sound_enabled=0;
  return;
  }

/* XXX are there 16-bit-only devices? */
tmp=AFMT_U8;
if((ioctl(soundfd,SNDCTL_DSP_SETFMT,&tmp))==-1)
  {
  close(soundfd);
  sound_enabled=0;
  return;
  }

/* XXX should it warn if it wanted stereo and didn't get it? */
tmp=(sound_channels>1);
if(ioctl(soundfd,SNDCTL_DSP_STEREO,&tmp)<0)
  sound_channels=1;	/* just use mono */

frag=(0x20000|BASE_SOUND_FRAG_PWR);

if(ioctl(soundfd,SNDCTL_DSP_SPEED,&sound_freq)<0)
  {
  close(soundfd);
  sound_enabled=0;
  return;
  }

sound_framesiz=sound_freq/50;

if((sound_buf=malloc(sound_framesiz*sound_channels))==NULL)
  {
  close(soundfd);
  sound_enabled=0;
  return;
  }

if(sound_freq>8000) frag++;
if(sound_freq>16000) frag++;
if(sound_freq>32000) frag++;
if(sound_channels>1) frag++;

if(ioctl(soundfd,SNDCTL_DSP_SETFRAGMENT,&frag)<0)
  {
  close(soundfd);
  free(sound_buf);
  sound_enabled=0;
  return;
  }

stereo=(sound_channels>1);

sound_ay_init();
}


void sound_end(void)
{
if(sound_enabled)
  {
  free(sound_buf);
  close(soundfd);
  }
}


static void sound_ay_overlay_tone(unsigned char *ptr,int chan,int level)
{
if(level && ay_tone_tick[chan]>=ay_tone_period[chan])
  (*ptr)+=level;
else
  (*ptr)-=level;

if(ay_tone_tick[chan]>=ay_tone_period[chan]*2)
  ay_tone_tick[chan]-=ay_tone_period[chan]*2;

ay_tone_tick[chan]+=ay_tick_incr;
}


static void sound_ay_overlay(void)
{
static int rng=1;
static int noise_toggle=1;
static int env_level=0;
int tone_level[3];
int mixer,envshape;
int f,g;
int v=0;
unsigned char *ptr;
struct ay_change_tag *change_ptr=ay_change;
int changes_left=ay_change_count;
int reg,r;

/* If nothing has written to the AY registers, don't need to do anything
   here */
if(!ay_change_count) return;

/* convert change times to sample offsets */
for(f=0;f<ay_change_count;f++)
  ay_change[f].ofs=(ay_change[f].tstates*sound_freq)/machine.hz;

for(f=0,ptr=sound_buf;f<sound_framesiz;f++,ptr+=sound_channels)
  {
  /* update ay registers. All this sub-frame change stuff
   * is pretty hairy, but how else would you handle the
   * samples in Robocop? :-) It also clears up some other
   * glitches.
   */
  while(changes_left && f>=change_ptr->ofs)
    {
    sound_ay_registers[reg=change_ptr->reg]=change_ptr->val;
    change_ptr++; changes_left--;

    /* fix things as needed for some register changes */
    switch(reg)
      {
      case 0: case 1: case 2: case 3: case 4: case 5:
        r=reg>>1;
        ay_tone_period[r]=8*(sound_ay_registers[reg&~1]|
                             (sound_ay_registers[reg|1]&15)<<8);

        /* important to get this right, otherwise e.g. Ghouls 'n' Ghosts
         * has really scratchy, horrible-sounding vibrato.
         */
        if(ay_tone_period[r])
          while(ay_tone_tick[r]>=ay_tone_period[r]*2)
            ay_tone_tick[r]-=ay_tone_period[r]*2;
        break;
      case 6:
        ay_noise_tick=0.;
        ay_noise_period=16*(sound_ay_registers[reg]&31);
        break;
      case 11: case 12:
        ay_env_period=256*(sound_ay_registers[11]|
                           (sound_ay_registers[12])<<8);
        break;
      case 13:
        ay_env_tick=0.;
        env_held=env_alternating=0;
        break;
      }
    }
  
  /* the tone level if no enveloping is being used */
  for(g=0;g<3;g++)
    tone_level[g]=ay_tone_levels[sound_ay_registers[8+g]&15];

  /* envelope */
  if(ay_env_period)
    {
    envshape=sound_ay_registers[13];
    if(!env_held)
      {
      v=((int)ay_env_tick*15)/ay_env_period;
      if(v<0) v=0;
      if(v>15) v=15;
      if((envshape&4)==0) v=15-v;
      if(env_alternating) v=15-v;
      env_level=ay_tone_levels[v];
      }
  
    for(g=0;g<3;g++)
      if(sound_ay_registers[8+g]&16)
        tone_level[g]=env_level;
  
    ay_env_tick+=ay_tick_incr;
    if(ay_env_tick>=ay_env_period)
      {
      ay_env_tick-=ay_env_period;
      if(!env_held && ((envshape&1) || (envshape&8)==0))
        {
        env_held=1;
        if((envshape&2) || (envshape&0xc)==4)
          env_level=ay_tone_levels[15-v];
        }
      if(!env_held && (envshape&2))
        env_alternating=!env_alternating;
      }
    }

  /* generate tone+noise */
  /* channel C first to make ACB easier */
  mixer=sound_ay_registers[7];
  if((mixer&4)==0 || (mixer&0x20)==0)
    {
    sound_ay_overlay_tone(ptr,2,
                          (noise_toggle || (mixer&0x20))?tone_level[2]:0);
    if(stereo) ptr[1]=*ptr;
    }
  if((mixer&1)==0 || (mixer&0x08)==0)
    sound_ay_overlay_tone(ptr,0,
                          (noise_toggle || (mixer&0x08))?tone_level[0]:0);
  if((mixer&2)==0 || (mixer&0x10)==0)
    sound_ay_overlay_tone(ptr+stereo,1,
                          (noise_toggle || (mixer&0x10))?tone_level[1]:0);

  /* update noise RNG/filter */
  ay_noise_tick+=ay_tick_incr;
  if(ay_noise_tick>=ay_noise_period)
    {
    if((rng&1)^((rng&2)?1:0))
      noise_toggle=!noise_toggle;
    
    /* rng is 17-bit shift reg, bit 0 is output.
     * input is bit 0 xor bit 2.
     */
    rng|=((rng&1)^((rng&4)?1:0))?0x20000:0;
    rng>>=1;
    
    ay_noise_tick-=ay_noise_period;
    }
  }
}


/* don't make the change immediately; record it for later,
 * to be made by sound_frame() (via sound_ay_overlay()).
 */
void sound_ay_write(int reg,int val)
{
if(tstates>=0 && ay_change_count<AY_CHANGE_MAX)
  {
  ay_change[ay_change_count].tstates=tstates;
  ay_change[ay_change_count].reg=reg;
  ay_change[ay_change_count].val=val;
  ay_change_count++;
  }
}


void sound_frame(void)
{
unsigned char *ptr;
int f,ret,ofs,siz=sound_framesiz*sound_channels;
static unsigned char write_buf[128*1024];	/* a fair size, I think */
static int write_count=0;

ptr=sound_buf+(stereo?sound_fillpos*2:sound_fillpos);
for(f=sound_fillpos;f<sound_framesiz;f++)
  {
  *ptr++=sound_oldval;
  if(stereo)
    *ptr++=sound_oldval;
  }

sound_ay_overlay();

if(write_count+siz<sizeof(write_buf))
  {
  memcpy(write_buf+write_count,sound_buf,siz);
  write_count+=siz;
  }

ret=ofs=0;

/* write as much as we can, waiting when it's a bad time,
 * and saving any samples we really can't write for
 * next time. This seems to be about the best we can do
 * without taking 100% CPU...
 */
while(write_count)
  {
  ret=write(soundfd,write_buf+ofs,write_count-ofs);
  if(ret>0)
    ofs+=ret,write_count-=ret;
  else
    {
    if(ret==0)
      usleep(5000);
    else
      break;
    }
  }

if(write_count)
  {
  if(ofs>=write_count)
    memcpy(write_buf,write_buf+ofs,write_count);
  else
    memmove(write_buf,write_buf+ofs,write_count);
  }

sound_oldpos=sound_fillpos=0;
sound_ptr=sound_buf;
beeper_first_write=1;

ay_change_count=0;
}


void sound_beeper(int on)
{
unsigned char *ptr;
int newpos,subpos;
int val,subval;
int f;
double dval;

val=(on?128+AMPL_BEEPER:128-AMPL_BEEPER);

if(val==sound_oldval) return;

/* the FP in the first bit below is painful, but a fixed-point approach
 * would need more than a 32-bit int to be tolerably accurate, and using
 * a lookup table would require regenerating the table whenever
 * machine.hz were changed.
 *
 * (But (XXX) lookup table may be worth the effort even so.)
 */
dval=(tstates*sound_freq)/(double)machine.hz;
newpos=(int)dval;
subpos=(int)(VOL_BEEPER*(dval-newpos));

/* if we already wrote here (and not first beeper write of frame),
 * adjust the level.
 */
if(newpos==sound_oldpos && !beeper_first_write)
  {
  /* adjust it as if the rest of the sample period were all in
   * the new state. (Often it will be, but if not, we'll fix
   * it later by doing this again.)
   */
  if(on)
    beeper_last_subpos+=VOL_BEEPER-subpos;
  else
    beeper_last_subpos-=VOL_BEEPER-subpos;
  }
else
  {
  beeper_first_write=0;
  beeper_last_subpos=(on?VOL_BEEPER-subpos:subpos);
  }

subval=128-AMPL_BEEPER+beeper_last_subpos;

if(newpos>=0)
  {
  /* fill gap from previous position */
  ptr=sound_buf+(stereo?sound_fillpos*2:sound_fillpos);
  for(f=sound_fillpos;f<newpos && f<sound_framesiz;f++)
    {
    *ptr++=sound_oldval;
    if(stereo)
      *ptr++=sound_oldval;
    }

  if(newpos<sound_framesiz)
    {
    /* newpos may be less than sound_fillpos, so... */
    ptr=sound_buf+(stereo?newpos*2:newpos);
    
    /* write subsample value */
    *ptr=subval;
    if(stereo)
      *++ptr=subval;
    }
  }

sound_oldpos=newpos;
sound_fillpos=newpos+1;
sound_oldval=val;
}

#endif	/* HAVE_SYS_SOUNDCARD_H */
