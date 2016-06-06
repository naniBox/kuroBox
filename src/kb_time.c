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
#include "kb_externalDisplay.h"
#include "kb_gpio.h"

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
static smpte_timecode_t ltc_timecode_ext;
static uint32_t ltc_timecode_count;
static uint32_t max_frame_for_fps;
static uint32_t fps_format;
static smpte_timecode_t ltc_timecode_int;
static uint32_t this_edge_time;

//-----------------------------------------------------------------------------
// PLL stuff
uint32_t ltc_ext_ts;
uint32_t ltc_int_ts;

//-----------------------------------------------------------------------------
static uint32_t TIM2_period;
static uint32_t TIM9_period;
uint32_t TIM5_period;
int32_t ltc_diff;
int32_t factor;

int32_t cb_time;

//-----------------------------------------------------------------------------
#define FPS_29_970_NDF			0
#define FPS_29_970_DF			1
#define FPS_30_000_NDF			2
#define FPS_30_000_DF			3
#define FPS_24_000				4
#define FPS_25_000				5
#define FPS_23_976				6

// these correspond to the interval count based on a 1MHz clock and the FPS
// above
struct ltc_clock_stuff_t
{
	uint32_t clock;
	uint8_t fps;
	uint8_t real_clock;
	uint8_t df;
};

struct ltc_clock_stuff_t ltc_clock_stuff [] =
{
	{ 33367, 29, 0, 0 },	// 29.97 NDF
	{ 33367, 29, 0, 1 },	// 29.97 DF
	{ 33333, 29, 0, 0 },	// 30.00 NDF
	{ 33333, 29, 0, 1 },	// 30.00 DF
	{ 41667, 23, 0, 0 },	// 24
	{ 40000, 24, 0, 0 },	// 25
	{ 41667, 23, 0, 0 },	// 23.976
};

//-----------------------------------------------------------------------------
static void frame_to_time(smpte_timecode_t * smpte_timecode, ltc_frame_t * ltc_frame)
{
	smpte_timecode->hours = ltc_frame->hours_units + ltc_frame->hours_tens*10;
	smpte_timecode->minutes  = ltc_frame->minutes_units  + ltc_frame->minutes_tens*10;
	smpte_timecode->seconds  = ltc_frame->seconds_units  + ltc_frame->seconds_tens*10;
	smpte_timecode->frames = ltc_frame->frame_units + ltc_frame->frame_tens*10;
}

//-----------------------------------------------------------------------------
static void add_frame(smpte_timecode_t * out, smpte_timecode_t * in, struct ltc_clock_stuff_t * clock_stuff)
{
	out->frames  = in->frames+1;
	out->seconds = in->seconds;
	out->minutes = in->minutes;
	out->hours   = in->hours;

	if ( out->frames > clock_stuff->fps )
	{
		out->frames = 0;
		out->seconds++;

		if ( out->seconds > 59 )
		{
			if ( clock_stuff->df &&
				 ((out->seconds%10)!=0) )
			{
				// drop the first 2 frames...
				out->frames = 2;
			}
			out->seconds = 0;
			out->minutes++;
		}

		if ( out->minutes > 59 )
		{
			out->minutes = 0;
			out->hours++;
		}

		if ( out->hours > 23 )
		{
			out->hours = 0;
		}
	}

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
		chSysLockFromISR();
			frame_to_time(&ltc_timecode_ext, &ltc_frame);
			/**/kbed_dataReadyI();
			if ( ltc_timecode_ext.frames == 0 )
			{
				kbg_setGPIO(ltc_timecode_ext.seconds&0x01, GPIOA, GPIOA__A06);
				ltc_ext_ts = this_edge_time;
				ltc_timecode_count++;
				if ( ltc_timecode_count == 5 )
				{
					// create sync point here
					GPTD2.tim->CNT = 500;
					GPTD5.tim->CNT = 500;
					GPTD9.tim->CNT = 0;
					// quicker than memcpy
					ltc_timecode_int.hours   = ltc_timecode_ext.hours;
					ltc_timecode_int.minutes = ltc_timecode_ext.minutes;
					ltc_timecode_int.seconds = ltc_timecode_ext.seconds;
					ltc_timecode_int.frames  = ltc_timecode_ext.frames;
				}
			}
			if ( ltc_timecode_ext.frames > max_frame_for_fps )
				max_frame_for_fps = ltc_timecode_ext.frames;
			kbs_setLTC(&ltc_timecode_ext);
			kbs_err_setLTC(1);
			kbw_setLTC(&ltc_frame);
		chSysUnlockFromISR();
	}
}

//-----------------------------------------------------------------------------
void kbt_pulseExtiCB(EXTDriver *extp, expchannel_t channel)
{
	(void)extp;
	(void)channel;
	this_edge_time = halGetCounterValue();
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

	cb_time = halGetCounterValue() - this_edge_time;
}


//-----------------------------------------------------------------------------
const smpte_timecode_t * 
kbt_getLTC(void)
{
	return &ltc_timecode_ext;
}

//-----------------------------------------------------------------------------
// my 1 second precision clock

//-----------------------------------------------------------------------------
void
inc_ltc_int(void)
{
	ltc_int_ts = halGetCounterValue();
	ltc_timecode_int.seconds++;
	kbg_setGPIO(ltc_timecode_int.seconds&0x01, GPIOA, GPIOA__A07);
	ltc_timecode_int.frames = 0;
	GPTD9.tim->CNT = 0;


	if ( ltc_timecode_int.seconds > 59 )
	{
		if ( ltc_clock_stuff[fps_format].df &&
			 ((ltc_timecode_int.seconds%10)!=0) )
		{
			// drop the first 2 frames...
			ltc_timecode_int.frames = 2;
		}
		ltc_timecode_int.seconds = 0;
		ltc_timecode_int.minutes++;
	}

	if ( ltc_timecode_int.minutes > 59 )
	{
		ltc_timecode_int.minutes = 0;
		ltc_timecode_int.hours++;
	}

	if ( ltc_timecode_int.hours > 23 )
	{
		ltc_timecode_int.hours = 0;
	}

	kbw_incOneSecPPS();
}

//-----------------------------------------------------------------------------
static void
one_rsec_cb(GPTDriver * gptp)
{
	(void)gptp;

	chSysLockFromISR();
	if( ltc_clock_stuff[fps_format].real_clock )
		inc_ltc_int();
	chSysUnlockFromISR();
}

//-----------------------------------------------------------------------------
static void
one_fsec_cb(GPTDriver * gptp)
{
	(void)gptp;

	chSysLockFromISR();
	if( !ltc_clock_stuff[fps_format].real_clock )
		inc_ltc_int();
	chSysUnlockFromISR();
}

//-----------------------------------------------------------------------------
static void
fps_cb(GPTDriver * gptp)
{
	(void)gptp;

	chSysLockFromISR();
	kbw_setSMPTETime(&ltc_timecode_int);
	kbs_setSMPTETime(&ltc_timecode_int);
	ltc_timecode_int.frames++;
	chSysUnlockFromISR();
}

//-----------------------------------------------------------------------------
static const GPTConfig one_rsec_cfg =
{
	84000000,		// max clock!
	one_rsec_cb,
	0,
	0
};

//-----------------------------------------------------------------------------
static const GPTConfig one_fsec_cfg =
{
	84000000,		// max clock!
	one_fsec_cb,
	0,
	0
};

//-----------------------------------------------------------------------------
static const GPTConfig fps_cfg =
{
	1000000,
	fps_cb,
	0,
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
	TIM2_period = 84000000-drift_factor/2;
	uint64_t TIM5_period_calc = (84000000-drift_factor/2);
	TIM5_period_calc *= 1000;
	TIM5_period_calc /= 999;
	TIM5_period = TIM5_period_calc;
	TIM9_period = ltc_clock_stuff[fps_format].clock;

	gptStart(&GPTD2, &one_rsec_cfg);
	gptStart(&GPTD5, &one_fsec_cfg);
	gptStart(&GPTD9, &fps_cfg);

	gptStartContinuous(&GPTD2, TIM2_period);
	gptStartContinuous(&GPTD5, TIM5_period);
	gptStartContinuous(&GPTD9, TIM9_period);

}

//-----------------------------------------------------------------------------
void
kbt_getRTC(rtc_t * rtc)
{
	RTCDateTime timespec;
	struct tm t;
	rtcGetTime(&RTCD1, &timespec);
	rtcConvertDateTimeToStructTm(&timespec, &t, 0);
	rtc->tm_sec 	= t.tm_sec;
	rtc->tm_min 	= t.tm_min;
	rtc->tm_hour 	= t.tm_hour;
	rtc->tm_mday 	= t.tm_mday;
	rtc->tm_mon 	= t.tm_mon;
	rtc->tm_year 	= t.tm_year;
	rtc->tm_wday 	= t.tm_wday;
	rtc->tm_yday 	= t.tm_yday;
	rtc->tm_isdst 	= t.tm_isdst;
}
