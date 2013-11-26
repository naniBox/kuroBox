/*
	kuroBox / naniBox
	Copyright (c) 2013
	david morris-oliveros // naniBox.com

    This file is part of kuroBox / naniBox.

    kuroBox / naniBox is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    kuroBox / naniBox is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

//-----------------------------------------------------------------------------
#include <ch.h>
#include <hal.h>
#include <string.h>
#include <chprintf.h>
#include "kb_time.h"
#include "kb_screen.h"
#include "kb_writer.h"
#include "kb_util.h"

//-----------------------------------------------------------------------------
/*
  The samples are going to be coming in from LTC from 24fps to 30fps.
  That gives us max frequency of 30fps * 80bits * 2(biphse) = 4800Hz and
  min of 24fps * 80bits * 1(biphase) = 1920Hz.
  Range of max frequency:
	30*80*2 = 4800Hz (208.3us)
	24*80*2 = 3840Hz (260.4us)
  Range of min frequency:
  	30*80*1 = 2400Hz (416.7us)
  	24*80*1 = 1920Hz (520.8us)
  Given this, we should be able to detect quite well the long and short pulses.

  @TODO: implement some type of tracking algorithm, that can track the short &
  		 long pulses. Note, the current algorithm works fine for realtime
  		 playback of LTC, the only time we will need tracking is for when LTC
  		 is sped up or slowed down

 */

#define LTC_SHORT_MAX_US				300
#define LTC_SHORT_MIN_US				160

#define LTC_LONG_MAX_US					580
#define LTC_LONG_MIN_US					350

#define IS_LTC_LONG(x)	(((x)<LTC_LONG_MAX_US)  && ((x)>LTC_LONG_MIN_US))
#define IS_LTC_SHORT(x)	(((x)<LTC_SHORT_MAX_US) && ((x)>LTC_SHORT_MIN_US))

#define LTC_SYNC_WORD 					(0xBFFC)

//-----------------------------------------------------------------------------
static uint32_t last_edge_time;
static bool_t was_last_edge_short;
static ltc_frame_t ltc_frame;
static smpte_timecode_t ltc_timecode;

//-----------------------------------------------------------------------------
static void frame_to_time(smpte_timecode_t * smpte_timecode, ltc_frame_t * ltc_frame)
{
	smpte_timecode->hours = ltc_frame->hours_units + ltc_frame->hours_tens*10;
	smpte_timecode->minutes  = ltc_frame->minutes_units  + ltc_frame->minutes_tens*10;
	smpte_timecode->seconds  = ltc_frame->seconds_units  + ltc_frame->seconds_tens*10;
	smpte_timecode->frames = ltc_frame->frame_units + ltc_frame->frame_tens*10;
}

//-----------------------------------------------------------------------------
static void ltc_store(uint8_t bit_set)
{
	// cast it into an array so it's easier to deal with for bitshifting
	uint8_t * ltc_raw = (uint8_t*)&ltc_frame;

	// we are pushing a new bit at the end of the struct - bit 7 of byte 9
	// so we need to push everything down a bit. First, bit 0 of byte 0
	// gets lost by shifting it out, then we push in bit 0 of byte 1 into
	// bit 7 of byte 0, this is repeat for all but the last byte
	for ( uint8_t idx = 0 ; idx < 9 ; idx++ )
	{
		ltc_raw[idx] >>= 1;
		ltc_raw[idx] |= (ltc_raw[idx+1] & 0x01) << 7;
	}

	// here, we've already pushed bit 0 of byte 9 into bit 7 of byte 8 in
	// the above loop, we now shift things down and push in the new bit
	ltc_raw[9] >>= 1;
	ltc_raw[9] |= (bit_set & 0x01) << 7; // this will either be 0x80 or 0x00;

	// if we *just* got the sync_word, then this bit is the end of the frame,
	// but remember, right now is the time JUST past, since the sync word
	// happens at the end of the frame.
	if ( ltc_frame.sync_word == LTC_SYNC_WORD )
	{
		chSysLockFromIsr();
		frame_to_time(&ltc_timecode, &ltc_frame);
		kbs_setLTC(&ltc_timecode);
		kbs_err_setLTC(1);
		kbw_setLTC(&ltc_frame);
		chSysUnlockFromIsr();
	}
}

//-----------------------------------------------------------------------------
void kbt_pulseExtiCB(EXTDriver *extp, expchannel_t channel)
{
	(void)extp;
	(void)channel;
	uint32_t this_edge_time = halGetCounterValue();
	uint32_t tdiff = RTT2US(this_edge_time - last_edge_time);
	last_edge_time = this_edge_time;

	/*
		the LTC comes in at differential manchester code, or biphase mark code
		it doesn't matter whether a signal is high or low, but rather the
		interval between transitions. DiffManch is self-clocking, and the
		output code is modulated with the clock.

		two short transitions indicate a logic 1, a long transition a logic 0.
	*/
	if ( IS_LTC_LONG(tdiff) )
	{
		ltc_store(0);
		was_last_edge_short = FALSE;
	}
	else if ( IS_LTC_SHORT(tdiff) )
	{
		if ( was_last_edge_short )
		{
			ltc_store(1);
			// the last edge *was* short, but since we just processed this one
			// we need to reset it so that it happens again.
			was_last_edge_short = FALSE;
		}
		else
		{
			was_last_edge_short = TRUE;
		}
	}
	else
	{
		// @TODO: if too many errors are happening, maybe we need to start
		// 	implementing a clock-tracking algorithm
		//other_count++;
	}
}


//-----------------------------------------------------------------------------
const smpte_timecode_t * 
kbt_getLTC(void)
{
	return &ltc_timecode;
}

//-----------------------------------------------------------------------------
// my 1 second precision clock

//-----------------------------------------------------------------------------
static void
one_sec_cb(GPTDriver * gptp)
{
	(void)gptp;

	chSysLockFromIsr();
		kbw_incOneSecPPS();
	chSysUnlockFromIsr();
}

//-----------------------------------------------------------------------------
static const GPTConfig one_sec_cfg =
{
	84000000,		// max clock!
	one_sec_cb,
	0
};

//-----------------------------------------------------------------------------
int 
kuroBoxTimeInit(void)
{
	memset(&ltc_frame,0,sizeof(ltc_frame));
	kbs_err_setLTC(0);
	return KB_OK;
}

//-----------------------------------------------------------------------------
int 
kuroBoxTimeStop(void)
{
	return KB_OK;
}

//-----------------------------------------------------------------------------
void
kbt_startOneSec(int32_t drift_factor)
{
	gptStart(&GPTD2, &one_sec_cfg);
	gptStartContinuous(&GPTD2, 84000000-drift_factor);
}
