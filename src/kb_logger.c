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
#include "kb_util.h"
#include "ff.h"
#include <hal.h>
#include <memstreams.h>
#include <chprintf.h>
#include <chrtclib.h>
#include <time.h>
#include <string.h>

//-----------------------------------------------------------------------------
#define LS_INIT					0
#define LS_WAIT_FOR_SD			1
#define LS_RUNNING				2
#define LS_EXITING				3

//-----------------------------------------------------------------------------
#define POLLING_COUNT			10
#define POLLING_DELAY			10

//-----------------------------------------------------------------------------
#define LOGGER_BUFFER_SIZE		4
#define LOGGER_MESSAGE_SIZE		512
#define LOGGER_FSYNC_INTERVAL	128

// nBkB  backwards, since this architecture is little-endian, we have to
// swizzle the bytes
#define LOGGER_PREAMBLE			0x426b426e
#define LOGGER_VERSION			11

//-----------------------------------------------------------------------------
struct __PACKED__ log_msg_v01
{
	uint32_t preamble;					// 4
	uint8_t version;					// 1
	uint8_t checksum;					// 1
	uint16_t msg_size;					// 2
	uint32_t msg_num;					// 4
	uint32_t write_errors;				// 4
										// = 16 for HEADER block

	struct ltc_frame_t ltc_frame;		// 10
	struct tm rtc;						// 9*4=36
										// = 46 for TIME block

	uint32_t pps;						// 4
	struct ubx_nav_sol_t nav_sol;		// 60
										// = 64 for GPS block

	uint8_t __pad[512 - (16 + 46 + 64)];
};
STATIC_ASSERT(sizeof(struct log_msg_v01)==LOGGER_MESSAGE_SIZE, LOGGER_MESSAGE_SIZE);

//-----------------------------------------------------------------------------
static Thread * loggerThread;
static uint8_t sd_det_count;
static uint8_t logger_state;

//-----------------------------------------------------------------------------
static uint8_t fs_ready;
static uint8_t fs_write_protected;
static FATFS SDC_FS;
uint32_t cardsize_MB;

//-----------------------------------------------------------------------------
static struct log_msg_v01 current_msg,writing_msg;

#define NANIBOX_DNAME "/nanibox"
#define KUROBOX_FNAME_STEM "kuro"
#define KUROBOX_FNAME_EXT ".kbb"
#define KUROBOX_BLANK_FNAME "--"
FIL kbfile;

//-----------------------------------------------------------------------------
static uint8_t
make_dirs(void)
{
	FRESULT err;
	DIR naniBox_dir;
	err = f_opendir(&naniBox_dir, NANIBOX_DNAME);
	if (FR_NO_PATH == err)
	{
		err = f_mkdir(NANIBOX_DNAME);
	}
	return err;
}

//-----------------------------------------------------------------------------
static uint8_t
new_file(void)
{
	char charbuf[64];
	MemoryStream msb;
	FRESULT err = FR_OK;
	uint16_t fnum = 0;
	for( ; fnum < 1000 ; fnum++ )
	{
		memset(charbuf,0,sizeof(charbuf));
		msObjectInit(&msb, (uint8_t*)charbuf, sizeof(charbuf), 0);
		chprintf((BaseSequentialStream *)&msb, "%s/%s%.4d%s", NANIBOX_DNAME,
				KUROBOX_FNAME_STEM, fnum, KUROBOX_FNAME_EXT);
		err = f_open(&kbfile, charbuf, FA_WRITE|FA_CREATE_NEW);
		f_sync(&kbfile);
		if (err == FR_EXIST)
			continue;
		else if (err != FR_OK)
			return err;
		else
			break;
	}

	// we can only get an OK if we opened the file
	if (err == FR_OK)
	{
		memset(charbuf,0,sizeof(charbuf));
		msObjectInit(&msb, (uint8_t*)charbuf, sizeof(charbuf), 0);
		chprintf((BaseSequentialStream *)&msb, "%s%.4d%s", KUROBOX_FNAME_STEM,
				fnum, KUROBOX_FNAME_EXT);
		kbs_setFName(charbuf);
	}

	return err;
}

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
	cardsize_MB = cardsize / (1024*1024);

	fs_write_protected = sdc_lld_is_write_protected(&SDCD1);
	if ( fs_write_protected )
		cardsize_MB = -cardsize_MB;
	kbs_setSDCFree(cardsize_MB);

	err = make_dirs();
	if (err != FR_OK)
	{
		sdcDisconnect(&SDCD1);
		return;
	}

	err = new_file();
	if (err != FR_OK)
	{
		sdcDisconnect(&SDCD1);
		return;
	}

	fs_ready = TRUE;
	logger_state = LS_RUNNING;
	current_msg.msg_num = 0;
}

//-----------------------------------------------------------------------------
static void
on_remove(void)
{
	sdcDisconnect(&SDCD1);
	kbs_setSDCFree(0);
	kbs_setFName(KUROBOX_BLANK_FNAME);
	fs_ready = FALSE;
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
		rtcGetTimeTm(&RTCD1, &current_msg.rtc);

		// the only really critical thing here:
		chSysLock();
		memcpy(&writing_msg, &current_msg, sizeof(writing_msg));
		chSysUnlock();

		uint8_t * buf = (uint8_t*) &writing_msg;
		writing_msg.checksum = calc_checksum_8(buf+16, LOGGER_MESSAGE_SIZE-16);

		UINT bytes_written = 0;
		FRESULT err = FR_OK;
		err = f_write(&kbfile, &writing_msg, sizeof(writing_msg), &bytes_written);
		if (bytes_written != sizeof(writing_msg) || err != FR_OK)
			current_msg.write_errors++;
		if (current_msg.msg_num%LOGGER_FSYNC_INTERVAL==0)
		{
			err = f_sync(&kbfile);
			if (err != FR_OK)
				current_msg.write_errors++;
		}
		//------------------------------------------------------

		//------------------------------------------------------
		kbs_setCounter(current_msg.msg_num);

		sd_card_status();
		if (LS_RUNNING != logger_state)
			break;
		//------------------------------------------------------

		// chTimeNow() will roll over every ~49 days
		// @TODO: make this code handle that
		while ( sleep_until < chTimeNow() )
		{
			// this code handles for when we skipped a writing-slot
			// i'm counting a skipped msg as a write error
			current_msg.write_errors++;
			current_msg.msg_num++;
			sleep_until += MS2ST(5);
		}
		chThdSleepUntil(sleep_until);
		sleep_until += MS2ST(5);            // Next deadline
		current_msg.msg_num++;
		if ( current_msg.msg_num%2048 == 0 ) // we write 1MB every 2048 msgs
		{
			kbs_setSDCFree(--cardsize_MB);
		}
	}
	return ret;
}

//-----------------------------------------------------------------------------
static WORKING_AREA(waLogger, 1024*8);
static msg_t 
thLogger(void *arg) 
{
	chDbgAssert(LS_INIT == logger_state, "thLogger, 1", "logger_state is not LS_INIT");
	(void)arg;
	chRegSetThreadName("Logger");

	current_msg.preamble = LOGGER_PREAMBLE;
	current_msg.version = LOGGER_VERSION;
	current_msg.msg_size = LOGGER_MESSAGE_SIZE;

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
int kuroBoxLoggerInit(void)
{
	chDbgAssert(LS_INIT == logger_state, "kuroBoxLogger, 1", "logger_state is not LS_INIT");

	loggerThread = chThdCreateStatic(waLogger, sizeof(waLogger), HIGHPRIO, thLogger, NULL);
	kbs_setFName(KUROBOX_BLANK_FNAME);
	return 0;
}

//-----------------------------------------------------------------------------
void kbl_setLTC(struct ltc_frame_t * ltc_frame)
{
	memcpy(&current_msg.ltc_frame, ltc_frame, sizeof(current_msg.ltc_frame));
}

//-----------------------------------------------------------------------------
void kbl_incPPS(void)
{
	current_msg.pps++;
}

//-----------------------------------------------------------------------------
void kbl_setGpsNavSol(struct ubx_nav_sol_t * nav_sol)
{
	memcpy(&current_msg.nav_sol, nav_sol, sizeof(current_msg.nav_sol));
}
