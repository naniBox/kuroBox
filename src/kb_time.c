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
#include <ltc.h>
#include <string.h>
#include <chprintf.h>
#include "kb_time.h"
#include "kb_screen.h"
#include "kb_buttons.h"

//-----------------------------------------------------------------------------
static Thread * timeThread;
#define LTC_BUFFER_SIZE 	1024
#define LTC_BUFFER_COUNT	2
Semaphore ltc_buffer_semaphore;

ltcsnd_sample_t ltc_buffer[LTC_BUFFER_COUNT][LTC_BUFFER_SIZE];
volatile int which_ltc_buffer;
int ltc_buffer_pos;

LTCDecoder *ltc_decoder;
LTCFrameExt ltc_frame;
SMPTETimecode ltc_smpte_time;

//-----------------------------------------------------------------------------
/*
static icucnt_t long_period;
static icucnt_t short_period;
static icucnt_t long_width;
static icucnt_t short_width;
*/
uint8_t close_by(icucnt_t target, icucnt_t subject, uint8_t percentage)
{
	uint32_t margin = target * 100 / percentage;
	return ( subject < (target + margin) && subject > (target - margin) );
}

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
void ltc_store(uint8_t which)
{
	ltc_data[ltc_data_idx++] = which;
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
void ltc_icu_period_cb(ICUDriver *icup)
{
	icucnt_t period = icuGetPeriod(icup);
	icucnt_t width = icuGetWidth(icup);

	kbs_setLTCS(period, width, 0,0);
	/*
	if ( long_period == 0 )
	{
		// first!
		long_period = short_period = period;
		long_width = long_width = width;
	}

	if ( )
	*/
}

//-----------------------------------------------------------------------------
void ltc_icu_period_cb_old(ICUDriver *icup)
{
	icucnt_t period = icuGetPeriod(icup);
	icucnt_t width = icuGetWidth(icup);
	while( period )
	{
		period--;
		ltc_buffer[which_ltc_buffer][ltc_buffer_pos++] = width?255:0;
		if ( width )
		{
			width--;
		}
		if ( ltc_buffer_pos == LTC_BUFFER_SIZE )
		{
			palSetPad(GPIOB, GPIOB_LED1);
			ltc_decoder_write(ltc_decoder, ltc_buffer[which_ltc_buffer], ltc_buffer_pos, 0);
			palClearPad(GPIOB, GPIOB_LED1);
			ltc_buffer_pos = 0;
			
			palSetPad(GPIOB, GPIOB_LED2);
			chSysLockFromIsr();
			if (timeThread != NULL) 
			{
				timeThread->p_u.rdymsg = (msg_t)1;
				chSchReadyI(timeThread);
				timeThread = NULL;
			}
			chSysUnlockFromIsr();
			palClearPad(GPIOB, GPIOB_LED2);

		}
	}
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

/*		while (ltc_decoder_read(ltc_decoder, &ltc_frame))
		{
			ltc_frame_to_time(&ltc_smpte_time, &ltc_frame.ltc, 1);
		}
		kbs_setLTC(&ltc_smpte_time);
*/
	}
	return 0;
}
	
//-----------------------------------------------------------------------------
int kuroBoxTimeInit(void)
{
	memset(ltc_buffer, 0, sizeof(ltc_buffer));
	which_ltc_buffer = 0;
	
	memset(ltc_data,0,sizeof(ltc_data));
	memset(ltc_data_print,0,sizeof(ltc_data_print));
	memset(pw,0,sizeof(pw));
	memset(pw_p,0,sizeof(pw_p));

	int apv = 1920; // audio frames per video frame, ballpark only
	ltc_decoder = ltc_decoder_create(apv, 0);
	
	timeThread = chThdCreateStatic(waTime, sizeof(waTime), NORMALPRIO, thTime, NULL);
	
	return 0;
}
