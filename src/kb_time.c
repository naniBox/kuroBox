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

#include <ch.h>
#include <hal.h>
#include <string.h>
#include <chprintf.h>
#include "kb_time.h"
#include "kb_screen.h"
#include "kb_buttons.h"
#include "nanibox_util.h"

//-----------------------------------------------------------------------------
static Thread * timeThread;

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

 */

#define LTC_SHORT_MAX_US				300
#define LTC_SHORT_MIN_US				160

#define LTC_LONG_MAX_US					580
#define LTC_LONG_MIN_US					350

#define IS_LTC_LONG(x)	((x<LTC_LONG_MAX_US) && (x>LTC_LONG_MIN_US))
#define IS_LTC_SHORT(x)	((x<LTC_SHORT_MAX_US) && (x>LTC_SHORT_MIN_US))

#define LTC_SYNC_WORD 					(0xBFFC)

STATIC_ASSERT(sizeof(struct LTCFrame)==10, LTC_FRAME_SIZE); // 80bits

uint32_t last_edge_time;
bool_t was_last_edge_short;
uint32_t long_count,short_count,other_count;

struct PW{ uint32_t width;uint8_t pulse; };
struct PW pw[256];
struct PW pw_p[256];
uint16_t pw_idx;

uint8_t ltc_data[80];
uint8_t ltc_data_print[80];
uint8_t ltc_data_idx;

uint16_t current_word;
uint8_t * ltc_raw;
struct LTCFrame ltc_frame;
struct SMPTETimecode ltc_timecode;
void ltc_frame_to_time2(struct SMPTETimecode *stime, struct LTCFrame *frame) {
	stime->years  = 0;
	stime->months = 0;
	stime->days   = 0;
	stime->hours = frame->hours_units + frame->hours_tens*10;
	stime->mins  = frame->mins_units  + frame->mins_tens*10;
	stime->secs  = frame->secs_units  + frame->secs_tens*10;
	stime->frame = frame->frame_units + frame->frame_tens*10;
}

void ltc_store(uint8_t bit_set)
{
	for ( uint8_t idx = 0 ; idx < 9 ; idx++ )
	{
		ltc_raw[idx] >>= 1;
		if ( ltc_raw[idx+1] & 0x01 )
			ltc_raw[idx] |= 0x80;
	}
	ltc_raw[9] >>= 1;
	if ( bit_set )
		ltc_raw[9] |= 0x80;

	// debug comes here:
	if ( ltc_frame.sync_word == LTC_SYNC_WORD )
	{
		ltc_frame_to_time2(&ltc_timecode, &ltc_frame);
		kbs_setLTC(&ltc_timecode);
		palTogglePad(GPIOB, GPIOB_LED2);
	}

	return;
	ltc_data[ltc_data_idx++] = bit_set;
	if ( ltc_data_idx == sizeof(ltc_data) )
	{
		memcpy(ltc_data_print,ltc_data,sizeof(ltc_data_print));
		memcpy(pw_p,pw,sizeof(pw_p));
		memset(pw,0,sizeof(pw));

		chSysLockFromIsr();
		if (timeThread != NULL)
		{
			timeThread->p_u.rdymsg = (msg_t)1;
			chSchReadyI(timeThread);
			timeThread = NULL;
		}
		chSysUnlockFromIsr();
		ltc_data_idx = 0;
		pw_idx = 0;
	}
}

void ltc_exti_cb(EXTDriver *extp, expchannel_t channel)
{
	(void)extp;
	(void)channel;
	uint32_t this_edge_time = halGetCounterValue();
	uint32_t tdiff = RTT2US(this_edge_time - last_edge_time);

	pw[pw_idx].width = tdiff;
	if ( IS_LTC_LONG(tdiff) )
	{
		pw[pw_idx].pulse = 0;
		ltc_store(0);
		was_last_edge_short = FALSE;
		long_count++;
	}
	else if ( IS_LTC_SHORT(tdiff) )
	{
		pw[pw_idx].pulse = 1;
		if ( was_last_edge_short )
		{
			ltc_store(1);
			was_last_edge_short = FALSE;
		}
		else
		{
			was_last_edge_short = TRUE;
		}
		short_count++;
	}
	else
	{
		pw[pw_idx].pulse = 2;
		other_count++;
	}

	if ( pw_idx < 2556 )
		pw_idx++;

	kbs_setLTCS(tdiff, long_count, short_count, other_count);

	last_edge_time = this_edge_time;
}

//-----------------------------------------------------------------------------
static WORKING_AREA(waTime, 1024*4);
static msg_t 
thTime(void *arg) 
{
	(void)arg;
	chRegSetThreadName("Time");
	while( !chThdShouldTerminate() )
	{
		chSysLock();
		timeThread = chThdSelf();
		msg_t msg = chSchGoSleepTimeoutS(THD_STATE_SUSPENDED, MS2ST(10));
		chSysUnlock();		
		if ( msg == RDY_TIMEOUT )
		{
			continue;
		}

		/*
		BaseSequentialStream * prnt = (BaseSequentialStream *)&SD1;
		for ( uint32_t i = 0 ; i < 80; i++ )
			chprintf(prnt, "%d ", ltc_data_print[i]);
		chprintf(prnt, "\n\r");
		if ( is_btn_0_pressed() )
		{
			for ( uint32_t i = 0 ; i < 256; i++ )
				chprintf(prnt, "%d ", pw_p[i].pulse);
			chprintf(prnt, "\n\r");

			for ( uint32_t i = 0 ; i < 256; i++ )
				chprintf(prnt, "%d ", pw_p[i].width);
			chprintf(prnt, "\n\r");
		}
		chprintf(prnt, "\n\r\n\r");
		*/
	}
	return 0;
}
	
//-----------------------------------------------------------------------------
int kuroBoxTimeInit(void)
{
	memset(&ltc_frame,0,sizeof(ltc_frame));
	ltc_raw = (uint8_t*)&ltc_frame;

	/**///debug
	memset(ltc_data,0,sizeof(ltc_data));
	memset(ltc_data_print,0,sizeof(ltc_data_print));
	memset(pw,0,sizeof(pw));
	memset(pw_p,0,sizeof(pw_p));
	
	timeThread = chThdCreateStatic(waTime, sizeof(waTime), NORMALPRIO, thTime, NULL);
	
	return 0;
}
