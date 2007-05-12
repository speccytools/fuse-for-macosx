/* sound.c: Sound support
   Copyright (c) 2000-2005 Russell Marks, Matan Ziv-Av, Philip Kendall

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

/* The AY white noise RNG algorithm is based on info from MAME's ay8910.c -
 * MAME's licence explicitly permits free use of info (even encourages it).
 */

/* NB: I know some of this stuff looks fairly CPU-hogging.
 * For example, the AY code tracks changes with sub-frame timing
 * in a rather hairy way, and there's subsampling here and there.
 * But if you measure the CPU use, it doesn't actually seem
 * very high at all. And I speak as a Cyrix owner. :-)
 */

#include <config.h>

#include <libspectrum.h>

#include "fuse.h"
#include "machine.h"
#include "settings.h"
#include "sound.h"
#include "sound/lowlevel.h"
#include "tape.h"
#include "ui/ui.h"

/* Do we have any of our sound devices available? */

/* configuration */
int sound_enabled=0;		/* Are we currently using the sound card */
int sound_enabled_ever=0;	/* if it's *ever* been in use; see
                                   sound_ay_write() and sound_ay_reset() */
int sound_stereo=0;		/* true for stereo *output sample* (only) */
int sound_stereo_ay_abc=0;	/* (AY stereo) true for ABC stereo, else ACB */
int sound_stereo_ay_narrow=0;	/* (AY stereo) true for narrow AY st. sep. */

int sound_stereo_ay=0;	/* local copy of settings_current.stereo_ay */
int sound_stereo_beeper=0;   /* and settings_current.stereo_beeper */


/* assume all three tone channels together match the beeper volume (ish).
 * Must be <=127 for all channels; 50+2+(24*3) = 124.
 * (Now scaled up for 16-bit.)
 */
#define AMPL_BEEPER		(50*256)
#define AMPL_TAPE		(2*256)
#define AMPL_AY_TONE		(24*256)	/* three of these */

/* max. number of sub-frame AY port writes allowed;
 * given the number of port writes theoretically possible in a
 * 50th I think this should be plenty.
 */
#define AY_CHANGE_MAX		8000

int sound_framesiz;

static int sound_channels;

static unsigned int ay_tone_levels[16];

static libspectrum_signed_word *sound_buf,*tape_buf;

/* beeper stuff */
static int sound_oldpos[2],sound_fillpos[2];
static int sound_oldval[2],sound_oldval_orig[2];

/* foo_subcycles are fixed-point with low 16 bits as fractional part.
 * The other bits count as the chip does.
 */
static unsigned int ay_tone_tick[3],ay_tone_high[3],ay_noise_tick;
static unsigned int ay_tone_subcycles,ay_env_subcycles;
static unsigned int ay_env_internal_tick,ay_env_tick;
static unsigned int ay_tick_incr;
static unsigned int ay_tone_period[3],ay_noise_period,ay_env_period;

static int beeper_last_subpos[2]={0,0};

/* Local copy of the AY registers */
static libspectrum_byte sound_ay_registers[16];

struct ay_change_tag
  {
  libspectrum_dword tstates;
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
/* AY output doesn't match the claimed levels; these levels are based
 * on the measurements posted to comp.sys.sinclair in Dec 2001 by
 * Matthew Westcott, adjusted as I described in a followup to his post,
 * then scaled to 0..0xffff.
 */
static const int levels[16]=
  {
  0x0000, 0x0385, 0x053D, 0x0770,
  0x0AD7, 0x0FD5, 0x15B0, 0x230C,
  0x2B4C, 0x43C1, 0x5A4B, 0x732F,
  0x9204, 0xAFF1, 0xD921, 0xFFFF
  };
int f;

/* scale the values down to fit */
for(f=0;f<16;f++)
  ay_tone_levels[f]=(levels[f]*AMPL_AY_TONE+0x8000)/0xffff;

ay_noise_tick=ay_noise_period=0;
ay_env_internal_tick=ay_env_tick=ay_env_period=0;
ay_tone_subcycles=ay_env_subcycles=0;
for(f=0;f<3;f++)
  ay_tone_tick[f]=ay_tone_high[f]=0,ay_tone_period[f]=1;

ay_change_count=0;
}


void sound_init(const char *device)
{
static int first_init=1;
int f,ret;
float hz;

/* if we don't have any sound I/O code compiled in, don't do sound */
#ifndef HAVE_SOUND
return;
#endif

if( !(!sound_enabled && settings_current.sound &&
      settings_current.emulation_speed == 100) )
  return;

sound_stereo_ay=settings_current.stereo_ay;
sound_stereo_beeper=settings_current.stereo_beeper;

/* only try for stereo if we need it */
if(sound_stereo_ay || sound_stereo_beeper)
  sound_stereo=1;
ret=sound_lowlevel_init(device,&settings_current.sound_freq,&sound_stereo);

if(ret)
  return;

/* important to override these settings if not using stereo
 * (it would probably be confusing to mess with the stereo
 * settings in settings_current though, which is why we make copies
 * rather than using the real ones).
 */
if(!sound_stereo)
  {
  sound_stereo_ay=0;
  sound_stereo_beeper=0;
  }

sound_enabled=sound_enabled_ever=1;

sound_channels=(sound_stereo?2:1);

hz = (float)machine_current->timings.processor_speed /
     machine_current->timings.tstates_per_frame;

sound_framesiz = settings_current.sound_freq / hz;

if((sound_buf=malloc(sizeof(libspectrum_signed_word)*
                     sound_framesiz*sound_channels))==NULL ||
   (tape_buf=malloc(sizeof(libspectrum_signed_word)*sound_framesiz))==NULL)
  {
  if(sound_buf)
    {
    free(sound_buf);
    sound_buf=NULL;
    }
  sound_end();
  return;
  }

/* if we're resuming, we need to be careful about what
 * gets reset. The minimum we can do is the beeper
 * buffer positions, so that's here.
 */
sound_oldpos[0]=sound_oldpos[1]=-1;
sound_fillpos[0]=sound_fillpos[1]=0;

/* this stuff should only happen on the initial call.
 * (We currently assume the new sample rate will be the
 * same as the previous one, hence no need to recalculate
 * things dependent on that.)
 */
if(first_init)
  {
  first_init=0;

  for(f=0;f<2;f++)
    sound_oldval[f]=sound_oldval_orig[f]=0;
  }

if(sound_stereo_beeper)
  {  
  for(f=0;f<STEREO_BUF_SIZE;f++)
    pstereobuf[f]=0;
  pstereopos=0;
  pstereobufsiz=(settings_current.sound_freq*psgap)/22000;
  }

if(sound_stereo_ay)
  {
  int pos=(sound_stereo_ay_narrow?3:6)*settings_current.sound_freq/8000;

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

ay_tick_incr=(int)(65536.*
                   libspectrum_timings_ay_speed(machine_current->machine)/
                   settings_current.sound_freq);
}

void sound_pause(void)
{
if(sound_enabled) sound_end();
}


void sound_unpause(void)
{
/* No sound if fastloading in progress */
if(settings_current.fastload && tape_is_playing()) return;

sound_init(settings_current.sound_device);
}


void sound_end(void)
{
if(sound_enabled)
  {
  if(sound_buf)
    {
    free(sound_buf);
    sound_buf=NULL;
    free(tape_buf);
    tape_buf=NULL;
    }
  sound_lowlevel_end();
  sound_enabled=0;
  }
}


/* write sample to buffer as pseudo-stereo */
static void sound_write_buf_pstereo(libspectrum_signed_word *out,int c)
{
int bl=(c-pstereobuf[pstereopos])/2;
int br=(c+pstereobuf[pstereopos])/2;

if(bl<-AMPL_BEEPER) bl=-AMPL_BEEPER;
if(br<-AMPL_BEEPER) br=-AMPL_BEEPER;
if(bl> AMPL_BEEPER) bl= AMPL_BEEPER;
if(br> AMPL_BEEPER) br= AMPL_BEEPER;

*out=bl; out[1]=br;

pstereobuf[pstereopos]=c;
pstereopos++;
if(pstereopos>=pstereobufsiz)
  pstereopos=0;
}



/* not great having this as a macro to inline it, but it's only
 * a fairly short routine, and it saves messing about.
 * (XXX ummm, possibly not so true any more :-))
 */
#define AY_GET_SUBVAL(chan)	(level*2*ay_tone_tick[chan]/tone_count)

#define AY_DO_TONE(var,chan) \
  (var)=0;								\
  is_low=0;								\
  if(level)								\
    {									\
    if(ay_tone_high[chan])						\
      (var)= (level);							\
    else								\
      (var)=-(level),is_low=1;						\
    }									\
  									\
  ay_tone_tick[chan]+=tone_count;					\
  count=0;								\
  while(ay_tone_tick[chan]>=ay_tone_period[chan])			\
    {									\
    count++;								\
    ay_tone_tick[chan]-=ay_tone_period[chan];				\
    ay_tone_high[chan]=!ay_tone_high[chan];				\
    									\
    /* has to be here, unfortunately... */				\
    if(count==1 && level && ay_tone_tick[chan]<tone_count)		\
      {									\
      if(is_low)							\
        (var)+=AY_GET_SUBVAL(chan);					\
      else								\
        (var)-=AY_GET_SUBVAL(chan);					\
      }									\
    }									\
  									\
  /* if it's changed more than once during the sample, we can't */	\
  /* represent it faithfully. So, just hope it's a sample.      */	\
  /* (That said, this should also help avoid aliasing noise.)   */	\
  if(count>1)								\
    (var)=-(level)


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



/* bitmasks for envelope */
#define AY_ENV_CONT	8
#define AY_ENV_ATTACK	4
#define AY_ENV_ALT	2
#define AY_ENV_HOLD	1


static void sound_ay_overlay(void)
{
static int rng=1;
static int noise_toggle=0;
static int env_first=1,env_rev=0,env_counter=15;
int tone_level[3];
int mixer,envshape;
int f,g,level,count;
libspectrum_signed_word *ptr;
struct ay_change_tag *change_ptr=ay_change;
int changes_left=ay_change_count;
int reg,r;
int is_low;
int chan1,chan2,chan3;
unsigned int tone_count,noise_count;

/* If no AY chip, don't produce any AY sound (!) */
if(!machine_current->capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_AY) return;

/* convert change times to sample offsets */
for(f=0;f<ay_change_count;f++)
  ay_change[f].ofs=(ay_change[f].tstates*settings_current.sound_freq)/
                   machine_current->timings.processor_speed;

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
        /* a zero-len period is the same as 1 */
        ay_tone_period[r]=(sound_ay_registers[reg&~1]|
                           (sound_ay_registers[reg|1]&15)<<8);
        if(!ay_tone_period[r])
          ay_tone_period[r]++;

        /* important to get this right, otherwise e.g. Ghouls 'n' Ghosts
         * has really scratchy, horrible-sounding vibrato.
         */
        if(ay_tone_tick[r]>=ay_tone_period[r]*2)
          ay_tone_tick[r]%=ay_tone_period[r]*2;
        break;
      case 6:
        ay_noise_tick=0;
        ay_noise_period=(sound_ay_registers[reg]&31);
        break;
      case 11: case 12:
        /* this one *isn't* fixed-point */
        ay_env_period=sound_ay_registers[11]|(sound_ay_registers[12]<<8);
        break;
      case 13:
        ay_env_internal_tick=ay_env_tick=ay_env_subcycles=0;
        env_first=1;
        env_rev=0;
        env_counter=(sound_ay_registers[13]&AY_ENV_ATTACK)?0:15;
        break;
      }
    }
  
  /* the tone level if no enveloping is being used */
  for(g=0;g<3;g++)
    tone_level[g]=ay_tone_levels[sound_ay_registers[8+g]&15];

  /* envelope */
  envshape=sound_ay_registers[13];
  level=ay_tone_levels[env_counter];
  
  for(g=0;g<3;g++)
    if(sound_ay_registers[8+g]&16)
      tone_level[g]=level;

  /* envelope output counter gets incr'd every 16 AY cycles.
   * Has to be a while, as this is sub-output-sample res.
   */
  ay_env_subcycles+=ay_tick_incr;
  noise_count=0;
  while(ay_env_subcycles>=(16<<16))
    {
    ay_env_subcycles-=(16<<16);
    noise_count++;
    ay_env_tick++;
    while(ay_env_tick>=ay_env_period)
      {
      ay_env_tick-=ay_env_period;

      /* do a 1/16th-of-period incr/decr if needed */
      if(env_first ||
         ((envshape&AY_ENV_CONT) && !(envshape&AY_ENV_HOLD)))
        {
        if(env_rev)
          env_counter-=(envshape&AY_ENV_ATTACK)?1:-1;
        else
          env_counter+=(envshape&AY_ENV_ATTACK)?1:-1;
        if(env_counter<0) env_counter=0;
        if(env_counter>15) env_counter=15;
        }
      
      ay_env_internal_tick++;
      while(ay_env_internal_tick>=16)
        {
        ay_env_internal_tick-=16;

        /* end of cycle */
        if(!(envshape&AY_ENV_CONT))
          env_counter=0;
        else
          {
          if(envshape&AY_ENV_HOLD)
            {
            if(env_first && (envshape&AY_ENV_ALT))
              env_counter=(env_counter?0:15);
            }
          else
            {
            /* non-hold */
            if(envshape&AY_ENV_ALT)
              env_rev=!env_rev;
            else
              env_counter=(envshape&AY_ENV_ATTACK)?0:15;
            }
          }
        
        env_first=0;
        }

      /* don't keep trying if period is zero */
      if(!ay_env_period) break;
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
  
  ay_tone_subcycles+=ay_tick_incr;
  tone_count=ay_tone_subcycles>>(3+16);
  ay_tone_subcycles&=(8<<16)-1;
  
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
  ay_noise_tick+=noise_count;
  while(ay_noise_tick>=ay_noise_period)
    {
    ay_noise_tick-=ay_noise_period;
    
    if((rng&1)^((rng&2)?1:0))
      noise_toggle=!noise_toggle;
    
    /* rng is 17-bit shift reg, bit 0 is output.
     * input is bit 0 xor bit 2.
     */
    rng|=((rng&1)^((rng&4)?1:0))?0x20000:0;
    rng>>=1;
    
    /* don't keep trying if period is zero */
    if(!ay_noise_period) break;
    }
  }
}


/* don't make the change immediately; record it for later,
 * to be made by sound_frame() (via sound_ay_overlay()).
 */
void sound_ay_write(int reg,int val,libspectrum_dword now)
{
if(ay_change_count<AY_CHANGE_MAX)
  {
  ay_change[ay_change_count].tstates=now;
  ay_change[ay_change_count].reg=(reg&15);
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

/* recalculate timings based on new machines ay clock */
sound_ay_init();

ay_change_count=0;
for(f=0;f<16;f++)
  sound_ay_write(f,0,0);
for(f=0;f<3;f++)
  ay_tone_high[f]=0;
ay_tone_subcycles=ay_env_subcycles=0;
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

/* the tape version works by writing to a separate mono buffer,
 * which gets added after being generated.
 */
#define SOUND_WRITE_BUF(is_tape,ptr,val) \
  if(is_tape)					\
    *(ptr)++=(val);				\
  else						\
    SOUND_WRITE_BUF_BEEPER(ptr,val)


void sound_frame(void)
{
libspectrum_signed_word *ptr,*tptr;
int f,bchan;
int ampl=AMPL_BEEPER;

if(!sound_enabled) return;

/* fill in remaining beeper/tape sound */
ptr=sound_buf+(sound_stereo?sound_fillpos[0]*2:sound_fillpos[0]);
for(bchan=0;bchan<2;bchan++)
  {
  for(f=sound_fillpos[bchan];f<sound_framesiz;f++)
    SOUND_WRITE_BUF(bchan,ptr,sound_oldval[bchan]);

  ptr=tape_buf+sound_fillpos[1];
  ampl=AMPL_TAPE;
  }

/* overlay tape sound */
ptr=sound_buf;
tptr=tape_buf;
for(f=0;f<sound_framesiz;f++,tptr++)
  {
  (*ptr++)+=*tptr;
  if(sound_stereo)
    (*ptr++)+=*tptr;
  }

/* overlay AY sound */
sound_ay_overlay();

sound_lowlevel_frame(sound_buf,sound_framesiz*sound_channels);

sound_oldpos[0]=sound_oldpos[1]=-1;
sound_fillpos[0]=sound_fillpos[1]=0;

ay_change_count=0;
}


/* two beepers are supported - the real beeper (call with is_tape==0)
 * and a `fake' beeper which lets you hear when a tape is being played.
 */
void sound_beeper(int is_tape,int on)
{
libspectrum_signed_word *ptr;
int newpos,subpos;
int val,subval;
int f;
int bchan=(is_tape?1:0);
int ampl=(is_tape?AMPL_TAPE:AMPL_BEEPER);
int vol=ampl*2;

if(!sound_enabled) return;

val=(on?-ampl:ampl);

if(val==sound_oldval_orig[bchan]) return;

/* XXX a lookup table might help here, but would need to regenerate it
 * whenever cycles_per_frame were changed (i.e. when machine type changed).
 */
newpos=(tstates*sound_framesiz)/machine_current->timings.tstates_per_frame;
subpos=(((libspectrum_signed_qword)tstates)*sound_framesiz*vol)/
       (machine_current->timings.tstates_per_frame)-vol*newpos;

/* if we already wrote here, adjust the level.
 */
if(newpos==sound_oldpos[bchan])
  {
  /* adjust it as if the rest of the sample period were all in
   * the new state. (Often it will be, but if not, we'll fix
   * it later by doing this again.)
   */
  if(on)
    beeper_last_subpos[bchan]+=vol-subpos;
  else
    beeper_last_subpos[bchan]-=vol-subpos;
  }
else
  beeper_last_subpos[bchan]=(on?vol-subpos:subpos);

subval=ampl-beeper_last_subpos[bchan];

if(newpos>=0)
  {
  /* fill gap from previous position */
  if(is_tape)
    ptr=tape_buf+sound_fillpos[1];
  else
    ptr=sound_buf+(sound_stereo?sound_fillpos[0]*2:sound_fillpos[0]);
  
  for(f=sound_fillpos[bchan];f<newpos && f<sound_framesiz;f++)
    SOUND_WRITE_BUF(bchan,ptr,sound_oldval[bchan]);

  if(newpos<sound_framesiz)
    {
    /* newpos may be less than sound_fillpos, so... */
    if(is_tape)
      ptr=tape_buf+newpos;
    else
      ptr=sound_buf+(sound_stereo?newpos*2:newpos);

    /* write subsample value */
    SOUND_WRITE_BUF(bchan,ptr,subval);
    }
  }

sound_oldpos[bchan]=newpos;
sound_fillpos[bchan]=newpos+1;
sound_oldval[bchan]=sound_oldval_orig[bchan]=val;
}
