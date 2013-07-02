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
#define LOGGER_MSG_START_OF_CHECKSUM	5

//-----------------------------------------------------------------------------
struct __PACKED__ log_msg_v01_t
{
	uint32_t preamble;					// 4
	uint8_t checksum;					// 1
	uint8_t version;					// 1
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
STATIC_ASSERT(sizeof(struct log_msg_v01_t)==LOGGER_MESSAGE_SIZE, LOGGER_MESSAGE_SIZE);

//-----------------------------------------------------------------------------
static Thread * loggerThread;
static Thread * writerThread;
static uint8_t sd_det_count;
static uint8_t logger_state;

//-----------------------------------------------------------------------------
static uint8_t fs_ready;
static uint8_t fs_write_protected;
static FATFS SDC_FS;
uint32_t cardsize_MB;

//-----------------------------------------------------------------------------
static struct log_msg_v01_t current_msg;

#define BUFFER_NUM			3
#define BUFFER_SIZE			32		// size in "log_msg_v01_t" units
struct write_buffer_t
{
	struct log_msg_v01_t buffer[BUFFER_SIZE];
	int8_t current_idx;
};
static Semaphore 				write_buffer_semaphore;
static struct write_buffer_t 	write_buffers[BUFFER_NUM];

#define NANIBOX_DNAME "/nanibox"
#define KUROBOX_FNAME_STEM "kuro"
#define KUROBOX_FNAME_EXT ".kbb"
#define KUROBOX_BLANK_FNAME "----"
#define KUROBOX_LOADING_NAME "<load |>"
#define KUROBOX_WP_NAME "<Locked>"
#define KUROBOX_ERR1 "<load />"
#define KUROBOX_ERR2 "<load ->"
#define KUROBOX_ERR3 "<load \\>"
#define KUROBOX_ERR4 "<load |>"
#define KUROBOX_ERR5 "<load />"
FIL kbfile;

//-----------------------------------------------------------------------------
static int8_t
new_write_buffer_idx_to_fill(void)
{
	int8_t buffer_idx = -1;
	chSemWait(&write_buffer_semaphore);
	for ( int8_t idx = 0 ; idx < BUFFER_NUM ; idx++ )
	{
		// we know it's empty and ready to be filled when current_idx == -1
		if ( write_buffers[idx].current_idx == -1 )
		{
			// ok, we're going to return this one, initialise it ready!
			write_buffers[idx].current_idx = 0;
			buffer_idx = idx;
			break;
		}
	}

	chSemSignal(&write_buffer_semaphore);
	return buffer_idx;
}

//-----------------------------------------------------------------------------
static void
return_write_buffer_idx_after_filling(int8_t idx)
{
	if (writerThread)
	{
		chSysLock();
		writerThread->p_u.rdymsg = (msg_t)idx+1;
		chSchReadyI(writerThread);
		writerThread = NULL;
		chSysUnlock();
	}
}

//-----------------------------------------------------------------------------
static int8_t
new_write_buffer_idx_to_write(void)
{
	int8_t buffer_idx = -1;
	chSemWait(&write_buffer_semaphore);
	for ( int8_t idx = 0 ; idx < BUFFER_NUM ; idx++ )
	{
		// we know it's full and ready to be written out when current_idx is
		// at the end
		if ( write_buffers[idx].current_idx == BUFFER_SIZE )
		{
			buffer_idx = idx;
			break;
		}
	}

	chSemSignal(&write_buffer_semaphore);
	return buffer_idx;
}

//-----------------------------------------------------------------------------
static void
return_write_buffer_idx_after_writing(int8_t idx)
{
	chSemWait(&write_buffer_semaphore);
	memset(&write_buffers[idx],0,sizeof(write_buffers[idx]));
	write_buffers[idx].current_idx = -1;
	chSemSignal(&write_buffer_semaphore);
}

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
		chprintf((BaseSequentialStream *)&msb, "%s%.4d", KUROBOX_FNAME_STEM, fnum);
		kbs_setFName(charbuf);
	}

	return err;
}

//-----------------------------------------------------------------------------
static void
on_insert(void)
{
	FRESULT err;

	kbs_setFName(KUROBOX_LOADING_NAME);
	if (sdcConnect(&SDCD1) != CH_SUCCESS)
	{
		kbs_setFName(KUROBOX_ERR1);
		return;
	}
	err = f_mount(0, &SDC_FS);
	if (err != FR_OK)
	{
		kbs_setFName(KUROBOX_ERR2);
		sdcDisconnect(&SDCD1);
		return;
	}

	chThdSleepMilliseconds(100);
	uint32_t clusters;
	FATFS *fsp;
	err = f_getfree("/", &clusters, &fsp);
	if (err != FR_OK)
	{
		kbs_setFName(KUROBOX_ERR3);
		sdcDisconnect(&SDCD1);
		return;
	}
	uint64_t cardsize = clusters * (((uint32_t)fsp->csize * (uint32_t)MMCSD_BLOCK_SIZE) / 1024);
	cardsize_MB = cardsize / 1024;

	fs_write_protected = sdc_lld_is_write_protected(&SDCD1);
	if ( fs_write_protected )
	{
		kbs_setSDCFree(-1);
		kbs_setFName(KUROBOX_WP_NAME);
		return;
	}

	err = make_dirs();
	if (err != FR_OK)
	{
		kbs_setFName(KUROBOX_ERR4);
		sdcDisconnect(&SDCD1);
		return;
	}

	err = new_file();
	if (err != FR_OK)
	{
		kbs_setFName(KUROBOX_ERR5);
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
			kbs_setFName(KUROBOX_BLANK_FNAME);
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
writing_run(void)
{
	uint8_t ret = 1;
	while(LS_RUNNING == logger_state)
	{
		if (chThdShouldTerminate())
		{
			logger_state = LS_EXITING;
			break;
		}

		int8_t idx = new_write_buffer_idx_to_write();
		if ( idx == -1 )
		{
			chSysLock();
			    writerThread = chThdSelf();
			    chSchGoSleepS(THD_STATE_SUSPENDED);
			chSysUnlock();
			continue;
		}

		struct write_buffer_t * buf = &write_buffers[idx];

		UINT bytes_written = 0;
		FRESULT err = FR_OK;
		err = f_write(&kbfile, buf->buffer, sizeof(buf->buffer), &bytes_written);
		if (bytes_written != sizeof(buf->buffer) || err != FR_OK)
			current_msg.write_errors++;
		err = f_sync(&kbfile);
		if (err != FR_OK)
			current_msg.write_errors++;
		kbs_setWriteCount(current_msg.msg_num);
		kbs_setWriteErrors(current_msg.write_errors);
		sd_card_status();
		if (LS_RUNNING != logger_state)
			break;

		return_write_buffer_idx_after_writing(idx);
	}
	return ret;
}

//-----------------------------------------------------------------------------
static WORKING_AREA(waWriter, 1024*8);
static msg_t 
thWriter(void *arg)
{
	chDbgAssert(LS_INIT == logger_state, "thWriter, 1", "logger_state is not LS_INIT");
	(void)arg;
	chRegSetThreadName("Writer");

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
			writing_run();
			break;
		case LS_EXITING:
			break;
		}
	}
	return 0;
}
	
//-----------------------------------------------------------------------------
static WORKING_AREA(waLogger, 1024*1);
static msg_t
thLogger(void *arg)
{
	(void)arg;

	// initialise the buffers
	memset(&write_buffers, 0, sizeof(write_buffers));
	for ( int8_t idx = 0 ; idx < BUFFER_NUM ; idx++ )
		write_buffers[idx].current_idx = -1;

	systime_t sleep_until = chTimeNow() + MS2ST(5);
	int8_t current_idx = -1;
	while( !chThdShouldTerminate() && LS_EXITING != logger_state)
	{
		if ( current_idx == -1 )
		{
			current_idx = new_write_buffer_idx_to_fill();
			if ( current_idx == -1 )
			{
				chThdSleepMilliseconds(1);
				continue;
			}
		}

		// here we'll have a good buffer to fill up until it's all full!
		struct write_buffer_t * wb = &write_buffers[current_idx];
		{
			struct log_msg_v01_t * lm = &wb->buffer[wb->current_idx];

			chSysLock();
			// make sure that we complete a write uninterrupted
			memcpy(lm, &current_msg, sizeof(current_msg));
			chSysUnlock();

			rtcGetTimeTm(&RTCD1, &lm->rtc);

			uint8_t * buf = (uint8_t*) lm;
			// NOTHING must get written after this, ok?
			lm->checksum = calc_checksum_8(buf+LOGGER_MSG_START_OF_CHECKSUM,
					LOGGER_MESSAGE_SIZE-LOGGER_MSG_START_OF_CHECKSUM);
		}

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

		wb->current_idx++;
		if ( wb->current_idx == BUFFER_SIZE )
		{
			// we're full up, return it for a new one
			return_write_buffer_idx_after_filling(current_idx);

			// now set it to -1, it will get a new one on next loop
			current_idx = -1;
		}
	}
	return 0;
}


//-----------------------------------------------------------------------------
int kuroBoxLoggerInit(void)
{
	chDbgAssert(LS_INIT == logger_state, "kuroBoxLogger, 1", "logger_state is not LS_INIT");

	chSemInit(&write_buffer_semaphore, 1);
	kbs_setFName(KUROBOX_BLANK_FNAME);

	loggerThread = chThdCreateStatic(waLogger, sizeof(waLogger), NORMALPRIO, thLogger, NULL);
	// don't store it here, because it needs to be kept NULL until the first sleep
	/* writerThread = */chThdCreateStatic(waWriter, sizeof(waWriter), HIGHPRIO, thWriter, NULL);

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
