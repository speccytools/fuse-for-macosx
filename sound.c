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

   Author contact information:

   E-mail: pak21-fuse@srcf.ucam.org
   Postal address: 15 Crescent Road, Wokingham, Berks, RG40 2DB, England

*/

/* 48/128 sound support by Russell Marks */

/* some AY details (volume levels, white noise RNG algorithm) based on
 * info from MAME's ay8910.c - MAME's licence explicitly permits free
 * use of info (even encourages it).
 */

/* NB: I know some of this stuff looks fairly CPU-hogging.
 * For example, the AY code tracks changes with sub-frame timing
 * in a rather hairy way, and there's subsampling here and there.
 * But if you measure the CPU use, it doesn't actually seem
 * very high at all. And I speak as a Cyrix owner. :-)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <config.h>

#if defined(HAVE_SYS_SOUNDCARD_H)
#include "osssound.h"
#endif

#include "fuse.h"
#include "machine.h"
#include "sound.h"
#include "spectrum.h"

/* configuration */
int sound_enabled=0;		/* Are we currently using the sound card;
				   cf fuse.c:fuse_sound_in_use */
int sound_freq=32000;
int sound_stereo=0;		/* true for stereo *output sample* (only) */
int sound_stereo_beeper=0;	/* beeper pseudo-stereo */
int sound_stereo_ay=0;		/* AY stereo separation */
int sound_stereo_ay_abc=0;	/* (AY stereo) true for ABC stereo, else ACB */
int sound_stereo_ay_narrow=0;	/* (AY stereo) true for narrow AY st. sep. */


#define AY_CLOCK		1773400

/* assume all three tone channels together match the beeper volume (ish).
 * Must be <=127 for all channels; 52+(24*3) = 124.
 */
#define AMPL_BEEPER		52
#define AMPL_AY_TONE		24	/* three of these */

/* full range of beeper volume */
#define VOL_BEEPER		(AMPL_BEEPER*2)

/* max. number of sub-frame AY port writes allowed;
 * given the number of port writes theoretically possible in a
 * 50th I think this should be plenty.
 */
#define AY_CHANGE_MAX		8000

static int sound_framesiz;

static int sound_channels;

static unsigned char ay_tone_levels[16];

static unsigned char *sound_buf;
static unsigned char *sound_ptr;
static int sound_oldpos,sound_fillpos,sound_oldval,sound_oldval_orig;

/* timer used for fadeout after beeper-toggle;
 * fixed-point with low 24 bits as fractional part.
 */
static unsigned int beeper_tick,beeper_tick_incr;

/* tick/incr/periods are all fixed-point with low 16 bits as
 * fractional part, except ay_env_{tick,period} which count as the chip does.
 */
static unsigned int ay_tone_tick[3],ay_noise_tick;
static unsigned int ay_env_tick,ay_env_subcycles;
static unsigned int ay_tick_incr;
static unsigned int ay_tone_period[3],ay_noise_period,ay_env_period;

static int env_held=0,env_alternating=0;

static int beeper_last_subpos=0;

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


#define STEREO_BUF_SIZE 1024

static int pstereobuf[STEREO_BUF_SIZE];
static int pstereobufsiz,pstereopos;
static int psgap=250;
static int rstereobuf_l[STEREO_BUF_SIZE],rstereobuf_r[STEREO_BUF_SIZE];
static int rstereopos,rchan1pos,rchan2pos,rchan3pos;


static void sound_ay_init(void)
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

ay_tick_incr=(int)(65536.*AY_CLOCK/sound_freq);

ay_change_count=0;
}


void sound_init(void)
{
int f,ret;

/* if we don't have any sound I/O code compiled in, don't do sound */
#if !defined(HAVE_SYS_SOUNDCARD_H)	/* only type for now */
return;
#endif

#if defined(HAVE_SYS_SOUNDCARD_H)
ret=osssound_init(&sound_freq,&sound_stereo);
#endif

if(ret)
  return;

/* important to override this if not using stereo */
if(!sound_stereo)
  {
  sound_stereo_ay=0;
  sound_stereo_beeper=0;
  }

sound_enabled=1;
sound_channels=(sound_stereo?2:1);
sound_framesiz=sound_freq/50;

if((sound_buf=malloc(sound_framesiz*sound_channels))==NULL)
  {
  sound_end();
  return;
  }

sound_oldval=sound_oldval_orig=128;
sound_oldpos=-1;
sound_fillpos=0;
sound_ptr=sound_buf;

beeper_tick=0;
beeper_tick_incr=(1<<24)/sound_freq;

sound_ay_init();

if(sound_stereo_beeper)
  {  
  for(f=0;f<STEREO_BUF_SIZE;f++)
    pstereobuf[f]=0;
  pstereopos=0;
  pstereobufsiz=(sound_freq*psgap)/22000;
  }

if(sound_stereo_ay)
  {
  int pos=(sound_stereo_ay_narrow?3:6)*sound_freq/8000;

  for(f=0;f<STEREO_BUF_SIZE;f++)
    rstereobuf_l[f]=rstereobuf_r[f]=0;
  rstereopos=0;

  /* the actual ACB/ABC bit :-) */
  rchan1pos=-pos;
  if(sound_stereo_ay_abc)
    rchan2pos=0,  rchan3pos=pos;
  else
    rchan2pos=pos,rchan3pos=0;
  }

fuse_sound_in_use=1;
}


void sound_end(void)
{
if(sound_enabled)
  {
  if(sound_buf)
    free(sound_buf);
#if defined(HAVE_SYS_SOUNDCARD_H)
  osssound_end();
#endif
  sound_enabled=0;
  }
}


/* write sample to buffer as pseudo-stereo */
void sound_write_buf_pstereo(unsigned char *out,int c)
{
int bl=(c-128-pstereobuf[pstereopos])/2;
int br=(c-128+pstereobuf[pstereopos])/2;

if(bl<-AMPL_BEEPER) bl=-AMPL_BEEPER;
if(br<-AMPL_BEEPER) br=-AMPL_BEEPER;
if(bl> AMPL_BEEPER) bl= AMPL_BEEPER;
if(br> AMPL_BEEPER) br= AMPL_BEEPER;

bl+=128; br+=128;
*out=(unsigned char)bl; out[1]=(unsigned char)br;

pstereobuf[pstereopos]=c-128;
pstereopos++;
if(pstereopos>=pstereobufsiz)
  pstereopos=0;
}



/* not great having this as a macro to inline it, but it's only
 * a fairly short routine, and it saves messing about.
 * (XXX ummm, possibly not so true any more :-))
 */
#define AY_GET_SUBVAL(tick,period) \
  (level*2*(tick-period)/ay_tick_incr)

#define AY_DO_TONE(var,chan) \
  (var)=0;								\
  was_high=0;								\
  if(level)								\
    {									\
    if(ay_tone_tick[chan]>=ay_tone_period[chan])			\
      (var)=-(level),was_high=1;					\
    else								\
      (var)= (level);							\
    }									\
  									\
  ay_tone_tick[chan]+=ay_tick_incr;					\
  if(level && !was_high && ay_tone_tick[chan]>=ay_tone_period[chan])	\
    (var)-=AY_GET_SUBVAL(ay_tone_tick[chan],ay_tone_period[chan]);	\
  									\
  if(ay_tone_tick[chan]>=ay_tone_period[chan]*2)			\
    {									\
    ay_tone_tick[chan]-=ay_tone_period[chan]*2;				\
    /* sanity check needed to avoid making samples sound terrible */ 	\
    if(level && ay_tone_tick[chan]<ay_tone_period[chan]) 		\
      (var)+=AY_GET_SUBVAL(ay_tone_tick[chan],0);			\
    }

/* add val, correctly delayed on either left or right buffer,
 * to add the AY stereo positioning. This doesn't actually put
 * anything directly in sound_buf, though.
 */
#define GEN_STEREO(pos,val) \
  if((pos)<0)							\
    {								\
    rstereobuf_l[rstereopos]+=(val);				\
    rstereobuf_r[(rstereopos-pos)%STEREO_BUF_SIZE]+=(val);	\
    }								\
  else								\
    {								\
    rstereobuf_l[(rstereopos+pos)%STEREO_BUF_SIZE]+=(val);	\
    rstereobuf_r[rstereopos]+=(val);				\
    }


static void sound_ay_overlay(void)
{
static int rng=1;
static int noise_toggle=0;
static int env_level=0;
int tone_level[3];
int mixer,envshape;
int f,g,level;
int v=0;
unsigned char *ptr;
struct ay_change_tag *change_ptr=ay_change;
int changes_left=ay_change_count;
int reg,r;
int was_high;
int chan1,chan2,chan3;

/* If no AY chip, don't produce any AY sound (!) */
if(!machine_current->ay.present) return;

/* convert change times to sample offsets */
for(f=0;f<ay_change_count;f++)
  ay_change[f].ofs=(ay_change[f].tstates*sound_freq)/
                   machine_current->timings.hz;

for(f=0,ptr=sound_buf;f<sound_framesiz;f++)
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
        ay_tone_period[r]=(8*(sound_ay_registers[reg&~1]|
                              (sound_ay_registers[reg|1]&15)<<8))<<16;

        /* important to get this right, otherwise e.g. Ghouls 'n' Ghosts
         * has really scratchy, horrible-sounding vibrato.
         */
        if(ay_tone_period[r] && ay_tone_tick[r]>=ay_tone_period[r]*2)
          ay_tone_tick[r]%=ay_tone_period[r]*2;
        break;
      case 6:
        ay_noise_tick=0;
        ay_noise_period=(16*(sound_ay_registers[reg]&31))<<16;
        break;
      case 11: case 12:
        /* this one *isn't* fixed-point */
        ay_env_period=sound_ay_registers[11]|(sound_ay_registers[12]<<8);
        break;
      case 13:
        ay_env_tick=ay_env_subcycles=0;
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

    /* envelope gets incr'd every 256 AY cycles */
    ay_env_subcycles+=ay_tick_incr;
    if(ay_env_subcycles>=(256<<16))
      {
      ay_env_subcycles-=(256<<16);
      
      ay_env_tick++;
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
    }

  /* generate tone+noise... or neither.
   * (if no tone/noise is selected, the chip just shoves the
   * level out unmodified. This is used by some sample-playing
   * stuff.)
   */
  chan1=tone_level[0];
  chan2=tone_level[1];
  chan3=tone_level[2];
  mixer=sound_ay_registers[7];
  
  if((mixer&1)==0)
    {
    level=chan1;
    AY_DO_TONE(chan1,0);
    }
  if((mixer&0x08)==0 && noise_toggle)
    chan1=0;
  
  if((mixer&2)==0)
    {
    level=chan2;
    AY_DO_TONE(chan2,1);
    }
  if((mixer&0x10)==0 && noise_toggle)
    chan2=0;
  
  if((mixer&4)==0)
    {
    level=chan3;
    AY_DO_TONE(chan3,2);
    }
  if((mixer&0x20)==0 && noise_toggle)
    chan3=0;

  /* write the sample(s) */
  if(!sound_stereo)
    {
    /* mono */
    (*ptr++)+=chan1+chan2+chan3;
    }
  else
    {
    if(!sound_stereo_ay)
      {
      /* stereo output, but mono AY sound; still,
       * incr separately in case of beeper pseudostereo.
       */
      (*ptr++)+=chan1+chan2+chan3;
      (*ptr++)+=chan1+chan2+chan3;
      }
    else
      {
      /* stereo with ACB/ABC AY positioning.
       * Here we use real stereo positions for the channels.
       * Just because, y'know, it's cool and stuff. No, really. :-)
       * This is a little tricky, as it works by delaying sounds
       * on the left or right channels to model the delay you get
       * in the real world when sounds originate at different places.
       */
      GEN_STEREO(rchan1pos,chan1);
      GEN_STEREO(rchan2pos,chan2);
      GEN_STEREO(rchan3pos,chan3);
      (*ptr++)+=rstereobuf_l[rstereopos];
      (*ptr++)+=rstereobuf_r[rstereopos];
      rstereobuf_l[rstereopos]=rstereobuf_r[rstereopos]=0;
      rstereopos++;
      if(rstereopos>=STEREO_BUF_SIZE)
        rstereopos=0;
      }
    }
  
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
void sound_ay_write(int reg,int val,DWORD tstates)
{
if(!sound_enabled) return;

if(tstates>=0 && ay_change_count<AY_CHANGE_MAX)
  {
  ay_change[ay_change_count].tstates=tstates;
  ay_change[ay_change_count].reg=reg;
  ay_change[ay_change_count].val=val;
  ay_change_count++;
  }
}


/* no need to call this initially, but should be called
 * on reset otherwise.
 */
void sound_ay_reset(void)
{
int f;

if(!sound_enabled) return;

ay_change_count=0;
for(f=0;f<15;f++)
  sound_ay_write(f,0,0);
}


/* it should go without saying that the beeper was hardly capable of
 * generating perfect square waves. :-) What seems to have happened is
 * that after the `click' in the intended direction away from the rest
 * (zero) position, it faded out gradually over about 1/50th of a second
 * back down to zero - the bulk of the fade being over after about
 * a 1/500th.
 *
 * The true behaviour is awkward to model accurately, but a compromise
 * emulation (doing a linear fadeout over 1/150th) seems to work quite
 * well and IMHO sounds a little more like a real speccy than most
 * emulations. It also has the considerable advantage of having a zero
 * rest position, which I'm a lot happier with. :-)
 */

#define BEEPER_FADEOUT	(((1<<24)/150)/AMPL_BEEPER)

#define BEEPER_OLDVAL_ADJUST \
  beeper_tick+=beeper_tick_incr;	\
  if(beeper_tick>=BEEPER_FADEOUT)	\
    {					\
    beeper_tick-=BEEPER_FADEOUT;	\
    if(sound_oldval>128)		\
      sound_oldval--;			\
    else				\
      if(sound_oldval<128)		\
        sound_oldval++;			\
    }

/* write stereo or mono beeper sample, and incr ptr */
#define SOUND_WRITE_BUF_BEEPER(ptr,val) \
  do						\
    {						\
    if(sound_stereo_beeper)			\
      {						\
      sound_write_buf_pstereo((ptr),(val));	\
      (ptr)+=2;					\
      }						\
    else					\
      {						\
      *(ptr)++=(val);				\
      if(sound_stereo)				\
        *(ptr)++=(val);				\
      }						\
    }						\
  while(0)


void sound_frame(void)
{
unsigned char *ptr;
int f;

if(!sound_enabled) return;

ptr=sound_buf+(sound_stereo?sound_fillpos*2:sound_fillpos);
for(f=sound_fillpos;f<sound_framesiz;f++)
  {
  BEEPER_OLDVAL_ADJUST;
  SOUND_WRITE_BUF_BEEPER(ptr,sound_oldval);
  }

sound_ay_overlay();

#if defined(HAVE_SYS_SOUNDCARD_H)
osssound_frame(sound_buf,sound_framesiz*sound_channels);
#endif

sound_oldpos=-1;
sound_fillpos=0;
sound_ptr=sound_buf;

ay_change_count=0;
}


void sound_beeper(int on)
{
unsigned char *ptr;
int newpos,subpos;
int val,subval;
int f;

if(!sound_enabled) return;

val=(on?128+AMPL_BEEPER:128-AMPL_BEEPER);

if(val==sound_oldval_orig) return;

/* XXX a lookup table might help here, but would need to regenerate it
 * whenever cycles_per_frame were changed (i.e. when machine type changed).
 */
newpos=(tstates*sound_framesiz)/machine_current->timings.cycles_per_frame;
/* the >>1s are to avoid overflow when int is 32-bit */
subpos=((tstates>>1)*sound_framesiz*VOL_BEEPER)/
       (machine_current->timings.cycles_per_frame>>1)-VOL_BEEPER*newpos;

/* if we already wrote here, adjust the level.
 */
if(newpos==sound_oldpos)
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
  beeper_last_subpos=(on?VOL_BEEPER-subpos:subpos);

subval=128-AMPL_BEEPER+beeper_last_subpos;

if(newpos>=0)
  {
  /* fill gap from previous position */
  ptr=sound_buf+(sound_stereo?sound_fillpos*2:sound_fillpos);
  for(f=sound_fillpos;f<newpos && f<sound_framesiz;f++)
    {
    BEEPER_OLDVAL_ADJUST;
    SOUND_WRITE_BUF_BEEPER(ptr,sound_oldval);
    }

  if(newpos<sound_framesiz)
    {
    /* newpos may be less than sound_fillpos, so... */
    ptr=sound_buf+(sound_stereo?newpos*2:newpos);

    /* limit subval in case of faded beeper level,
     * to avoid slight spikes on ordinary tones.
     */
    if((sound_oldval<128 && subval<sound_oldval) ||
       (sound_oldval>=128 && subval>sound_oldval))
      subval=sound_oldval;

    /* write subsample value */
    SOUND_WRITE_BUF_BEEPER(ptr,subval);
    }
  }

sound_oldpos=newpos;
sound_fillpos=newpos+1;
sound_oldval=sound_oldval_orig=val;
}
