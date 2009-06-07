/* sound.cpp: Sound support
   Copyright (c) 2000-2009 Russell Marks, Matan Ziv-Av, Philip Kendall,
                           Fredrick Meunier

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

/* The AY white noise RNG algorithm is based on info from MAME's ay8910.c -
 * MAME's licence explicitly permits free use of info (even encourages it).
 */

extern "C" {
#include "fuse.h"
#include "machine.h"
#include "options.h"
#include "settings.h"
#include "sound.h"
#include "tape.h"
#include "ui/ui.h"
}

#include "sound/Blip_Buffer.h"

/* Do we have any of our sound devices available? */

/* configuration */
int sound_enabled = 0;		/* Are we currently using the sound card */
int sound_enabled_ever = 0;	/* if it's *ever* been in use; see
				   sound_ay_write() and sound_ay_reset() */
int sound_stereo = 0;		/* true for stereo *output sample* (only) */
int sound_stereo_ay = 0;	/* local copy of settings_current.stereo_ay */

/* assume all three tone channels together match the beeper volume (ish).
 * Must be <=127 for all channels; 50+2+(24*3) = 124.
 * (Now scaled up for 16-bit.)
 */
#define AMPL_BEEPER		( 50 * 256)
#define AMPL_TAPE		( 5 * 256 )
#define AMPL_AY_TONE		( 24 * 256 )	/* three of these */

/* max. number of sub-frame AY port writes allowed;
 * given the number of port writes theoretically possible in a
 * 50th I think this should be plenty.
 */
#define AY_CHANGE_MAX		8000

int sound_freq;
int sound_framesiz;

static int sound_channels;

static unsigned int ay_tone_levels[16];

static unsigned int ay_tone_tick[3], ay_tone_high[3], ay_noise_tick;
static unsigned int ay_tone_cycles, ay_env_cycles;
static unsigned int ay_env_internal_tick, ay_env_tick;
static unsigned int ay_tone_period[3], ay_noise_period, ay_env_period;

/* Local copy of the AY registers */
static libspectrum_byte sound_ay_registers[16];

struct ay_change_tag
{
  libspectrum_dword tstates;
  unsigned char reg, val;
};

static struct ay_change_tag ay_change[ AY_CHANGE_MAX ];
static int ay_change_count;

Blip_Buffer *left_buf;
Blip_Buffer *right_buf;
blip_sample_t *samples;

typedef Blip_Synth<blip_good_quality, blip_unscaled> fuse_synth;
fuse_synth *left_beeper_synth, *right_beeper_synth;

fuse_synth *ay_a_synth, *ay_b_synth, *ay_c_synth;
fuse_synth *ay_a_synth_r, *ay_b_synth_r, *ay_c_synth_r;

struct speaker_type_tag
{
  int bass;
  double treble;
};

static struct speaker_type_tag speaker_type[] =
  { { 16, -8.0 }, { 180, -8.0 }, { 2000, -47.0 } };

static double
sound_get_volume( int volume )
{
  if( volume < 0 ) volume = 0;
  else if( volume > 100 ) volume = 100;

  return volume / 100.0;
}

bool
sound_init_blip( Blip_Buffer **buf, fuse_synth **synth )
{
  *buf = new Blip_Buffer();
  (*buf)->clock_rate( machine_current->timings.processor_speed );
  if ( (*buf)->set_sample_rate( settings_current.sound_freq ) ) {
    sound_end();
    ui_error( UI_ERROR_ERROR, "out of memory at %s:%d", __FILE__, __LINE__ );
    return false;
  }

  *synth = new fuse_synth();

  (*synth)->volume( sound_get_volume( settings_current.volume_beeper ) );
  (*synth)->output( *buf );

  (*buf)->bass_freq( speaker_type[ option_enumerate_sound_speaker_type() ].bass );
  (*synth)->treble_eq( speaker_type[ option_enumerate_sound_speaker_type() ].treble );

  return true;
}

static void
sound_ay_init( void )
{
  /* AY output doesn't match the claimed levels; these levels are based
   * on the measurements posted to comp.sys.sinclair in Dec 2001 by
   * Matthew Westcott, adjusted as I described in a followup to his post,
   * then scaled to 0..0xffff.
   */
  static const int levels[16] = {
    0x0000, 0x0385, 0x053D, 0x0770,
    0x0AD7, 0x0FD5, 0x15B0, 0x230C,
    0x2B4C, 0x43C1, 0x5A4B, 0x732F,
    0x9204, 0xAFF1, 0xD921, 0xFFFF
  };
  int f;

  /* scale the values down to fit */
  for( f = 0; f < 16; f++ )
    ay_tone_levels[f] = ( levels[f] * AMPL_AY_TONE + 0x8000 ) / 0xffff;

  ay_noise_tick = ay_noise_period = 0;
  ay_env_internal_tick = ay_env_tick = ay_env_period = 0;
  ay_tone_cycles = ay_env_cycles = 0;
  for( f = 0; f < 3; f++ )
    ay_tone_tick[f] = ay_tone_high[f] = 0, ay_tone_period[f] = 1;

  ay_change_count = 0;
}

void
sound_init( const char *device )
{
  int ret;
  float hz;

  if( !( !sound_enabled && settings_current.sound &&
	 settings_current.emulation_speed == 100 ) )
    return;

  sound_stereo_ay = settings_current.stereo_ay;

  /* only try for stereo if we need it */
  if( sound_stereo_ay )
    sound_stereo = 1;

  ret =
    sound_lowlevel_init( device, &settings_current.sound_freq, &sound_stereo );

  if( ret )
    return;

  if( !sound_init_blip(&left_buf, &left_beeper_synth) ) return;
  if( sound_stereo && !sound_init_blip(&right_buf, &right_beeper_synth) ) return;

  ay_a_synth = new fuse_synth();
  ay_a_synth->volume( sound_get_volume( settings_current.volume_ay) );
  ay_a_synth->output( left_buf );
  ay_a_synth->treble_eq( speaker_type[ option_enumerate_sound_speaker_type() ].treble );

  ay_b_synth = new fuse_synth();
  ay_b_synth->volume( sound_get_volume( settings_current.volume_ay) );
  ay_b_synth->treble_eq( speaker_type[ option_enumerate_sound_speaker_type() ].treble );

  ay_c_synth = new fuse_synth();
  ay_c_synth->volume( sound_get_volume( settings_current.volume_ay) );
  ay_c_synth->output( left_buf );
  ay_c_synth->treble_eq( speaker_type[ option_enumerate_sound_speaker_type() ].treble );

  /* important to override these settings if not using stereo
   * (it would probably be confusing to mess with the stereo
   * settings in settings_current though, which is why we make copies
   * rather than using the real ones).
   */
  if( !sound_stereo ) {
    sound_stereo_ay = 0;
  }

  ay_a_synth_r = 0;
  ay_b_synth_r = 0;
  ay_c_synth_r = 0;

  if( sound_stereo ) {
    ay_c_synth_r = new fuse_synth();
    ay_c_synth_r->volume( sound_get_volume( settings_current.volume_ay ) );
    ay_c_synth_r->output( right_buf );

    if( sound_stereo_ay ) {
      /* stereo with ACB stereo. */
      ay_b_synth->output( right_buf );
    } else {
      ay_a_synth_r = new fuse_synth();
      ay_a_synth_r->volume( sound_get_volume( settings_current.volume_ay ) );
      ay_a_synth_r->output( right_buf );

      ay_b_synth->output( left_buf );

      ay_b_synth_r = new fuse_synth();
      ay_b_synth_r->volume( sound_get_volume( settings_current.volume_ay ) );
      ay_b_synth_r->output( right_buf );
    }
  } else {
    ay_b_synth->output( left_buf );
  }

  sound_enabled = sound_enabled_ever = 1;

  sound_channels = ( sound_stereo ? 2 : 1 );

  hz = ( float ) machine_current->timings.processor_speed /
    machine_current->timings.tstates_per_frame;

  sound_framesiz = ( float ) settings_current.sound_freq / hz;
  sound_framesiz++;

  samples = (blip_sample_t *)calloc( sound_framesiz * sound_channels,
                                     sizeof(blip_sample_t) );
}

void
sound_pause( void )
{
  if( sound_enabled )
    sound_end();
}

void
sound_unpause( void )
{
  /* No sound if fastloading in progress */
  if( settings_current.fastload && tape_is_playing() )
    return;

  sound_init( settings_current.sound_device );
}

void
sound_end( void )
{
  if( sound_enabled ) {
    delete left_beeper_synth; left_beeper_synth = 0;
    delete left_buf; left_buf = 0;
    delete right_beeper_synth; right_beeper_synth = 0;
    delete right_buf; right_buf = 0;

    delete ay_a_synth; ay_a_synth = 0;
    delete ay_b_synth; ay_b_synth = 0;
    delete ay_c_synth; ay_c_synth = 0;
    delete ay_a_synth_r; ay_a_synth_r = 0;
    delete ay_b_synth_r; ay_b_synth_r = 0;
    delete ay_c_synth_r; ay_c_synth_r = 0;

    sound_lowlevel_end();
    free( samples );
    sound_enabled = 0;
  }
}

static inline void
ay_do_tone( int level, unsigned int tone_count, int& var, int chan )
{
  var = 0;

  ay_tone_tick[ chan ] += tone_count;

  if( ay_tone_tick[ chan ] >= ay_tone_period[ chan ] ) {
    ay_tone_tick[ chan ] -= ay_tone_period[ chan ];
    ay_tone_high[ chan ] = !ay_tone_high[ chan ];
  }

  if( level ) {
    if( ay_tone_high[ chan ] )
      var = level;
    else {
      var = -level;
    }
  }

  /* The AY output goes from 0 to the maximum volume, so there
   * is a DC component that is half the maxmum volume.
   * Robocop uses a high frequency square wave with a tone
   * period of one to average out to being like a DC offset at
   * around half the maximum volume. This is used as a base for
   * the sample playback.
   * This seems to intefere with our attempt to remove the
   * returned DC offset, so for now we just ignore the high
   * frequency wave and hope it's a sample
   */
  if( ay_tone_period[ chan ] == 1 ) {
      var = -level;
  }
}

/* bitmasks for envelope */
#define AY_ENV_CONT	8
#define AY_ENV_ATTACK	4
#define AY_ENV_ALT	2
#define AY_ENV_HOLD	1

/* the AY steps down the external clock by 16 for tone and noise
   generators */
#define AY_CLOCK_DIVISOR 16
/* all Spectrum models and clones with an AY seem to count down the
   master clock by 2 to drive the AY */
#define AY_CLOCK_RATIO 2

static void
sound_ay_overlay( void )
{
  static int rng = 1;
  static int noise_toggle = 0;
  static int env_first = 1, env_rev = 0, env_counter = 15;
  int tone_level[3];
  int mixer, envshape;
  int g, level;
  libspectrum_dword f;
  struct ay_change_tag *change_ptr = ay_change;
  int changes_left = ay_change_count;
  int reg, r;
  int chan1, chan2, chan3;
  int last_chan1, last_chan2, last_chan3;
  unsigned int tone_count, noise_count;

  /* If no AY chip, don't produce any AY sound (!) */
  if( !periph_fuller_active && 
      ( !machine_current->capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_AY ) )
    return;

  for( f = 0; f < machine_current->timings.tstates_per_frame;
       f+= AY_CLOCK_DIVISOR * AY_CLOCK_RATIO ) {
    /* update ay registers. */
    while( changes_left && f >= change_ptr->tstates ) {
      sound_ay_registers[ reg = change_ptr->reg ] = change_ptr->val;
      change_ptr++;
      changes_left--;

      /* fix things as needed for some register changes */
      switch ( reg ) {
      case 0: case 1: case 2: case 3: case 4: case 5:
        r = reg >> 1;
        /* a zero-len period is the same as 1 */
        ay_tone_period[r] = ( sound_ay_registers[ reg & ~1 ] |
                              ( sound_ay_registers[ reg | 1 ] & 15 ) << 8 );
        if( !ay_tone_period[r] )
          ay_tone_period[r]++;

        /* important to get this right, otherwise e.g. Ghouls 'n' Ghosts
         * has really scratchy, horrible-sounding vibrato.
         */
        if( ay_tone_tick[r] >= ay_tone_period[r] * 2 )
          ay_tone_tick[r] %= ay_tone_period[r] * 2;
        break;
      case 6:
        ay_noise_tick = 0;
        ay_noise_period = ( sound_ay_registers[ reg ] & 31 );
        break;
      case 11: case 12:
        ay_env_period =
          sound_ay_registers[11] | ( sound_ay_registers[12] << 8 );
        break;
      case 13:
        ay_env_internal_tick = ay_env_tick = ay_env_cycles = 0;
        env_first = 1;
        env_rev = 0;
        env_counter = ( sound_ay_registers[13] & AY_ENV_ATTACK ) ? 0 : 15;
        break;
      }
    }

    /* the tone level if no enveloping is being used */
    for( g = 0; g < 3; g++ )
      tone_level[g] = ay_tone_levels[ sound_ay_registers[ 8 + g ] & 15 ];

    /* envelope */
    envshape = sound_ay_registers[13];
    level = ay_tone_levels[ env_counter ];

    for( g = 0; g < 3; g++ )
      if( sound_ay_registers[ 8 + g ] & 16 )
        tone_level[g] = level;

    /* envelope output counter gets incr'd every 16 AY cycles. */
    ay_env_cycles += AY_CLOCK_DIVISOR;
    noise_count = 0;
    while( ay_env_cycles >= 16 ) {
      ay_env_cycles -= 16;
      noise_count++;
      ay_env_tick++;
      while( ay_env_tick >= ay_env_period ) {
        ay_env_tick -= ay_env_period;

        /* do a 1/16th-of-period incr/decr if needed */
        if( env_first ||
            ( ( envshape & AY_ENV_CONT ) && !( envshape & AY_ENV_HOLD ) ) ) {
          if( env_rev )
            env_counter -= ( envshape & AY_ENV_ATTACK ) ? 1 : -1;
          else
            env_counter += ( envshape & AY_ENV_ATTACK ) ? 1 : -1;
          if( env_counter < 0 )
            env_counter = 0;
          if( env_counter > 15 )
            env_counter = 15;
        }

        ay_env_internal_tick++;
        while( ay_env_internal_tick >= 16 ) {
          ay_env_internal_tick -= 16;

          /* end of cycle */
          if( !( envshape & AY_ENV_CONT ) )
            env_counter = 0;
          else {
            if( envshape & AY_ENV_HOLD ) {
              if( env_first && ( envshape & AY_ENV_ALT ) )
                env_counter = ( env_counter ? 0 : 15 );
            } else {
              /* non-hold */
              if( envshape & AY_ENV_ALT )
                env_rev = !env_rev;
              else
                env_counter = ( envshape & AY_ENV_ATTACK ) ? 0 : 15;
            }
          }

          env_first = 0;
        }

        /* don't keep trying if period is zero */
        if( !ay_env_period )
          break;
      }
    }

    /* generate tone+noise... or neither.
     * (if no tone/noise is selected, the chip just shoves the
     * level out unmodified. This is used by some sample-playing
     * stuff.)
     */
    chan1 = tone_level[0];
    chan2 = tone_level[1];
    chan3 = tone_level[2];
    mixer = sound_ay_registers[7];

    ay_tone_cycles += AY_CLOCK_DIVISOR;
    tone_count = ay_tone_cycles >> 3;
    ay_tone_cycles &= 7;

    if( ( mixer & 1 ) == 0 ) {
      level = chan1;
      ay_do_tone( level, tone_count, chan1, 0 );
    }
    if( ( mixer & 0x08 ) == 0 && noise_toggle )
      chan1 = 0;

    if( ( mixer & 2 ) == 0 ) {
      level = chan2;
      ay_do_tone( level, tone_count, chan2, 1 );
    }
    if( ( mixer & 0x10 ) == 0 && noise_toggle )
      chan2 = 0;

    if( ( mixer & 4 ) == 0 ) {
      level = chan3;
      ay_do_tone( level, tone_count, chan3, 2 );
    }
    if( ( mixer & 0x20 ) == 0 && noise_toggle )
      chan3 = 0;

    if( last_chan1 != chan1 ) {
      ay_a_synth->update( f, chan1 );
      if( ay_a_synth_r ) ay_a_synth_r->update( f, chan1 );
      last_chan1 = chan1;
    }
    if( last_chan2 != chan2 ) {
      ay_b_synth->update( f, chan2 );
      if( ay_b_synth_r ) ay_b_synth_r->update( f, chan1 );
      last_chan2 = chan2;
    }
    if( last_chan3 != chan3 ) {
      ay_c_synth->update( f, chan3 );
      if( ay_c_synth_r ) ay_c_synth_r->update( f, chan1 );
      last_chan3 = chan3;
    }

    /* update noise RNG/filter */
    ay_noise_tick += noise_count;
    while( ay_noise_tick >= ay_noise_period ) {
      ay_noise_tick -= ay_noise_period;

      if( ( rng & 1 ) ^ ( ( rng & 2 ) ? 1 : 0 ) )
        noise_toggle = !noise_toggle;

      /* rng is 17-bit shift reg, bit 0 is output.
       * input is bit 0 xor bit 2.
       */
      rng |= ( ( rng & 1 ) ^ ( ( rng & 4 ) ? 1 : 0 ) ) ? 0x20000 : 0;
      rng >>= 1;

      /* don't keep trying if period is zero */
      if( !ay_noise_period )
        break;
    }
  }
}

/* don't make the change immediately; record it for later,
 * to be made by sound_frame() (via sound_ay_overlay()).
 */
void
sound_ay_write( int reg, int val, libspectrum_dword now )
{
  if( ay_change_count < AY_CHANGE_MAX ) {
    ay_change[ ay_change_count ].tstates = now;
    ay_change[ ay_change_count ].reg = ( reg & 15 );
    ay_change[ ay_change_count ].val = val;
    ay_change_count++;
  }
}

/* no need to call this initially, but should be called
 * on reset otherwise.
 */
void
sound_ay_reset( void )
{
  int f;

  /* recalculate timings based on new machines ay clock */
  sound_ay_init();

  ay_change_count = 0;
  for( f = 0; f < 16; f++ )
    sound_ay_write( f, 0, 0 );
  for( f = 0; f < 3; f++ )
    ay_tone_high[f] = 0;
  ay_tone_cycles = ay_env_cycles = 0;
}

void
sound_frame( void )
{
  long count;

  if( !sound_enabled )
    return;

  /* overlay AY sound */
  sound_ay_overlay();

  left_buf->end_frame( machine_current->timings.tstates_per_frame );

  if( sound_stereo ) {
    right_buf->end_frame( machine_current->timings.tstates_per_frame );

    /* Read left channel into even samples, right channel into odd samples:
       LRLRLRLRLR... */
    count = left_buf->read_samples( samples, sound_framesiz, 1 );
    right_buf->read_samples( samples + 1, count, 1 );
    count <<= 1;
  } else {
    count = left_buf->read_samples( samples, sound_framesiz );
  }

  sound_lowlevel_frame( samples, count );

  ay_change_count = 0;
}

void
sound_beeper( int on )
{
  static int beeper_ampl[] = { 0, AMPL_TAPE, AMPL_BEEPER, AMPL_BEEPER+AMPL_TAPE };

  int val;
  int ampl;

  if( !sound_enabled )
    return;

  /* Timex machines have no loading noise */
  if( tape_is_playing() &&
      ( !settings_current.sound_load || machine_current->timex ) )
    on = on & 0x02;

  ampl = beeper_ampl[on];

  val = -beeper_ampl[3] + ampl*2;

  left_beeper_synth->update( tstates, val );
  if( sound_stereo ) {
    right_beeper_synth->update( tstates, val );
  }
}
