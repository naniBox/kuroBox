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
#include "kb_logger.h"
#include "kb_screen.h"
#include "fatfsWrapper.h"
#include <hal.h>

//-----------------------------------------------------------------------------
#define POLLING_INTERVAL                10
#define POLLING_DELAY                   10

//-----------------------------------------------------------------------------
static Thread * loggerThread;
uint32_t counter;
static VirtualTimer poll_timer;
uint8_t sd_det_count;
static EventSource inserted_event, removed_event;
struct EventListener inserted_event_listener, removed_event_listener;
//-----------------------------------------------------------------------------
static bool_t fs_ready;
static FATFS SDC_FS;

//-----------------------------------------------------------------------------
static void
poll_sd_stat(void *p)
{
	palTogglePad(GPIOA, GPIOA_LED3);

	BaseBlockDevice *bbdp = p;

	/* The presence check is performed only while the driver is not in a
		transfer state because it is often performed by changing the mode of
		the pin connected to the CS/D3 contact of the card, this could disturb
		the transfer.*/
	blkstate_t state = blkGetDriverState(bbdp);
	chSysLockFromIsr();
	if ((state != BLK_READING) && (state != BLK_WRITING))
	{
		/* Safe to perform the check.*/
		if (sd_det_count > 0)
		{
			if (blkIsInserted(bbdp))
			{
				if (--sd_det_count == 0)
				{
					chEvtBroadcastI(&inserted_event);
				}
			}
			else
				sd_det_count = POLLING_INTERVAL;
		}
		else
		{
			if (!blkIsInserted(bbdp))
			{
				sd_det_count = POLLING_INTERVAL;
				chEvtBroadcastI(&removed_event);
			}
		}
	}
	chVTSetI(&poll_timer, MS2ST(POLLING_DELAY), poll_sd_stat, bbdp);
	chSysUnlockFromIsr();
}

//-----------------------------------------------------------------------------
static void InsertHandler(eventid_t id)
{
	FRESULT err;

	(void)id;
	if (sdcConnect(&SDCD1))
	{
		return;
	}
	err = f_mount(0, &SDC_FS);
	if (err != FR_OK)
	{
		sdcDisconnect(&SDCD1);
		return;
	}
	fs_ready = TRUE;
}

//-----------------------------------------------------------------------------
static void RemoveHandler(eventid_t id)
{
	(void)id;
	sdcDisconnect(&SDCD1);
	fs_ready = FALSE;
}

//-----------------------------------------------------------------------------
void
waitForSD(void)
{

}

//-----------------------------------------------------------------------------
static const evhandler_t evhndl[] = {
		InsertHandler,
		RemoveHandler
};

//-----------------------------------------------------------------------------
static WORKING_AREA(waLogger, 128);
static msg_t 
thLogger(void *arg) 
{
	(void)arg;
	chRegSetThreadName("Logger");
	systime_t sleep_until = chTimeNow();
	while( !chThdShouldTerminate() )
	{
		sleep_until += MS2ST(5);            // Next deadline
		// LOG IT!

	    chEvtDispatch(evhndl, ALL_EVENTS);
		counter++;
		kbs_setCounter(counter);
		
		// chTimeNow() will roll over every ~49 days 
		// @TODO: make this code handle that
		while ( sleep_until < chTimeNow() )
			sleep_until += MS2ST(5);
		chThdSleepUntil(sleep_until);

	}
	return 0;
}
	

//-----------------------------------------------------------------------------
int kuroBoxLogger(void)
{
	loggerThread = chThdCreateStatic(waLogger, sizeof(waLogger), NORMALPRIO, thLogger, NULL);
	return 0;
	chEvtRegister(&inserted_event, &inserted_event_listener, 0);
	chEvtRegister(&removed_event, &removed_event_listener, 1);
	chEvtInit(&inserted_event);
	chEvtInit(&removed_event);
	chSysLock();
	sd_det_count = POLLING_INTERVAL;
	chVTSetI(&poll_timer, MS2ST(POLLING_DELAY), poll_sd_stat, NULL);
	chSysUnlock();
	
	return 0;
}
