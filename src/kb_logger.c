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
#include "nanibox_util.h"
#include "fatfsWrapper.h"
#include <hal.h>

//-----------------------------------------------------------------------------
#define LS_INIT			0
#define LS_WAIT_FOR_SD	1
#define LS_RUNNING		2
#define LS_EXITING		3

//-----------------------------------------------------------------------------
#define POLLING_COUNT			10
#define POLLING_DELAY			10

//-----------------------------------------------------------------------------
static Thread * loggerThread;
uint32_t counter;
uint8_t sd_det_count;
uint8_t logger_state;
//-----------------------------------------------------------------------------
static bool_t fs_ready;
static FATFS SDC_FS;

//-----------------------------------------------------------------------------
static void
on_insert(void)
{
	FRESULT err;

	if (sdcConnect(&SDCD1) != CH_SUCCESS)
	{
		return;
	}
	err = f_mount(0, &SDC_FS);
	if (err != FR_OK)
	{
		sdcDisconnect(&SDCD1);
		return;
	}

	chThdSleepMilliseconds(100);
	uint32_t clusters;
	FATFS *fsp;
	err = f_getfree("/", &clusters, &fsp);
	if (err != FR_OK)
	{
		sdcDisconnect(&SDCD1);
		return;
	}
	uint64_t cardsize = clusters * (uint32_t)fsp->csize * (uint32_t)MMCSD_BLOCK_SIZE;
	uint32_t cardsize_MB = cardsize / (1024*1024);
	kbs_setSDCFree(cardsize_MB);

	fs_ready = TRUE;
	logger_state = LS_RUNNING;
	counter = 0;
}

//-----------------------------------------------------------------------------
static void
on_remove(void)
{
	sdcDisconnect(&SDCD1);
	fs_ready = FALSE;
	counter = 10000000;
	kbs_setSDCFree(0);
}

//-----------------------------------------------------------------------------
static uint8_t
wait_for_sd(void)
{
	while(1)
	{
		if (sdc_lld_is_card_inserted(&SDCD1))
		{
			if (--sd_det_count == 0)
			{
				on_insert();
				break;
			}
		}
		else
		{
			sd_det_count = POLLING_COUNT;
		}
		chThdSleepMilliseconds(POLLING_DELAY);
	}
	return KB_OK;
}

//-----------------------------------------------------------------------------
static uint8_t
sd_card_status(void)
{
	uint8_t ret = KB_OK;
	blkstate_t state = blkGetDriverState(&SDCD1);
	if ((state != BLK_READING) && (state != BLK_WRITING))
	{
		if (!sdc_lld_is_card_inserted(&SDCD1))
		{
			on_remove();
			logger_state = LS_WAIT_FOR_SD;
			sd_det_count = POLLING_COUNT;
		}
	}
	return ret;
}

//-----------------------------------------------------------------------------
static uint8_t
run(void)
{
	uint8_t ret = 1;
	systime_t sleep_until = chTimeNow() + MS2ST(5);
	while(LS_RUNNING == logger_state)
	{
		if (chThdShouldTerminate())
		{
			logger_state = LS_EXITING;
			break;
		}

		//------------------------------------------------------
		// LOG IT!
		//------------------------------------------------------

		//------------------------------------------------------
		counter++;
		kbs_setCounter(counter);

		sd_card_status();
		if (LS_RUNNING != logger_state)
			break;
		//------------------------------------------------------

		// chTimeNow() will roll over every ~49 days
		// @TODO: make this code handle that
		while ( sleep_until < chTimeNow() )
			sleep_until += MS2ST(5);
		chThdSleepUntil(sleep_until);
		sleep_until += MS2ST(5);            // Next deadline
	}
	return ret;
}

//-----------------------------------------------------------------------------
static WORKING_AREA(waLogger, 1024*1);
static msg_t 
thLogger(void *arg) 
{
	chDbgAssert(LS_INIT == logger_state, "thLogger, 1", "logger_state is not LS_INIT");
	(void)arg;
	chRegSetThreadName("Logger");

	sd_det_count = POLLING_COUNT;
	logger_state = LS_WAIT_FOR_SD;
	while( !chThdShouldTerminate() && LS_EXITING != logger_state)
	{
		switch ( logger_state )
		{
		case LS_INIT:
			chDbgAssert(0, "thLogger, 2", "logger_state is should not be LS_INIT");
			break;
		case LS_WAIT_FOR_SD:
			wait_for_sd();
			break;
		case LS_RUNNING:
			run();
			break;
		case LS_EXITING:
			break;
		}
	}
	return 0;
}
	

//-----------------------------------------------------------------------------
int kuroBoxLogger(void)
{
	chDbgAssert(LS_INIT == logger_state, "kuroBoxLogger, 1", "logger_state is not LS_INIT");

	loggerThread = chThdCreateStatic(waLogger, sizeof(waLogger), NORMALPRIO, thLogger, NULL);
	return 0;
}
