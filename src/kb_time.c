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
#include "kb_time.h"
#include "kb_screen.h"

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
void ltc_icu_period_cb(ICUDriver *icup)
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
static WORKING_AREA(waTime, 128);
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
			continue;

		while (ltc_decoder_read(ltc_decoder, &ltc_frame))
		{
			ltc_frame_to_time(&ltc_smpte_time, &ltc_frame.ltc, 1);
		}
		kbs_setLTC(&ltc_smpte_time);
	}
	return 0;
}
	
//-----------------------------------------------------------------------------
int kuroBoxTimeInit(void)
{
	memset(ltc_buffer, 0, sizeof(ltc_buffer));
	which_ltc_buffer = 0;
	
	int apv = 1920; // audio frames per video frame, ballpark only
	ltc_decoder = ltc_decoder_create(apv, 0);
	
	timeThread = chThdCreateStatic(waTime, sizeof(waTime), NORMALPRIO, thTime, NULL);
	
	return 0;
}
