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
    /*

	This file is important, so please listen up, this is how it works.

	There are 2 threads spawned in here, the Logger & Writer. There is also
	a buffer structure that stores the data. There are currently 2 buffers, both
	initially empty. 

	External data sources notify of new data via the kbw_setXXX() functions. That
	data gets copied into the "current_msg" struct. This means that data can
	change whenever, but whatever is in the "current_msg" struct is a snapshot
	of the system at any given point.

	The logger thread is triggered every 5ms (200Hz), it copies the contents of 
	the "current_msg" struct into a spare slot in the current buffer. If, for
	whatever reason, the thread can't copy the data in time, the "write_errors"
	count is increased (this is reset on every file), so that at least it is
	known when it happened.

	Once the current write buffer is full (currently 48 slots), the writer thread
	is woken. This thread just gets a writer buffer and writes it out to SD, then
	it returns that buffer to the pool, and goes back to sleep.

	It is important to have the buffers as big as possible, so that we can
	aim for maximum throughput with minimum interrups and therefore fewer drops.

	When the writer thread is trying to write out data, but the SD card is 
	blocking the write (because of internal SD Flash delay), the 2x48 slot buffers
	will fill up before the write finishes - meaning that we will start dropping
	data and accumilating write errors. 

	This, at some point, is unavoidable until I make kuroBox have at least 
	1MB of SRAM that can buffer up to 10 seconds of	data. Currently, with 48kB,
	we buffer about 400ms worth of data, and 64kB would give us ~600ms. 
	One possible improvement in this field is to try to put all stack data 
	into CCM to free up normal SRAM, but bigger buffer == better.

    */
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
#include "kb_writer.h"
#include "kb_screen.h"
#include "kb_util.h"
#include "kb_serial.h"
#include "ff.h"
#include "kb_gpio.h"
#include <hal.h>
#include <memstreams.h>
#include <chprintf.h>
#include <chrtclib.h>
#include <time.h>
#include <string.h>

//-----------------------------------------------------------------------------
//#define DEBG_WRITER_TIME_WRITES
#ifdef  DEBG_WRITER_TIME_WRITES
	#define DEBG_WRITER_TIME_WRITES_TIMER(x) uint32_t t##x = halGetCounterValue()
#else
	#define DEBG_WRITER_TIME_WRITES_TIMER(x)
#endif

//-----------------------------------------------------------------------------
#define LS_INIT					0
#define LS_WAIT_FOR_SD			1
#define LS_RUNNING				2
#define LS_EXITING				3

//-----------------------------------------------------------------------------
#define POLLING_COUNT			10
#define POLLING_DELAY			10

//-----------------------------------------------------------------------------
////////#define LOGGER_BUFFER_SIZE		4
#define LOGGER_FSYNC_INTERVAL			128

#define KBB_MSG_SIZE					512

// nBkB  backwards, since this architecture is little-endian, we
// pre-swizzle the bytes
#define KBB_PREAMBLE					0x426b426e
#define KBB_CHECKSUM_START				6

#define KBB_CLASS_HEADER				0x01
#define KBB_CLASS_DATA					0x10

#define KBB_SUBCLASS_HEADER_01			0x01
#define KBB_SUBCLASS_DATA_01			0x01

//-----------------------------------------------------------------------------
//
typedef struct kbb_header_t kbb_header_t;
struct __PACKED__ kbb_header_t
{
	uint32_t preamble;					// 4
	uint16_t checksum;					// 2
	uint8_t msg_class;					// 1
	uint8_t msg_subclass;				// 1
	uint16_t msg_size;					// 2
										// = 10 for HEADER block
};

//-----------------------------------------------------------------------------
// this msg is written at the start of every file
typedef struct kbb_01_01_t kbb_01_01_t; // the header packet
struct __PACKED__ kbb_01_01_t
{
	kbb_header_t header;				// 10 bytes

	uint8_t vnav_header[64];			// vnav stuff, dumped in here

	uint8_t __pad[512 - (10 + 64)];		// 438 left
};

STATIC_ASSERT(sizeof(kbb_01_01_t)==KBB_MSG_SIZE, KBB_MSG_SIZE);

//-----------------------------------------------------------------------------
typedef struct kbb_02_01_t kbb_02_01_t;	// the data packet
struct __PACKED__ kbb_02_01_t
{
	kbb_header_t header;				// 10 bytes

	uint32_t msg_num;					// 4
	uint32_t write_errors;				// 4
										// = 18 for HEADER block

	ltc_frame_t ltc_frame;				// 10
	struct tm rtc;						// 9*4=36
	uint32_t one_sec_pps;				// 4
										// = 50 for TIME (LTC+RTC) block

	uint32_t pps;						// 4
	ubx_nav_sol_t nav_sol;				// 60
										// = 64 for GPS block

	vnav_data_t vnav;					// 4*3+4 = 16
										// 16 for the VNAV block

	float altitude;						// 4
	float temperature;					// 4
										// 8 for the altimeter block

	uint32_t global_count;

	uint8_t __pad[512 - (18 + 50 + 64 + 16 + 8 + 4)];
};
typedef kbb_02_01_t kbb_current_msg_t;
STATIC_ASSERT(sizeof(kbb_02_01_t)==KBB_MSG_SIZE, KBB_MSG_SIZE);
STATIC_ASSERT(sizeof(kbb_current_msg_t)==KBB_MSG_SIZE, KBB_MSG_SIZE);

//-----------------------------------------------------------------------------
// threads and states. 
// Threads should only be woken if they're asleep, so we use the thread pointer
// for that. We also need to clean up on shutdown, and that's what the other 
// pointers are for
static Thread * loggerThread;
static Thread * writerThread;
static Thread * writerThreadForSleep;
static uint8_t logger_state;

//-----------------------------------------------------------------------------
// specifics for FatFS
static uint8_t fs_ready;
static uint8_t fs_write_protected;
static FATFS SDC_FS;
static FIL kbfile;
uint32_t cardsize_MB;

//-----------------------------------------------------------------------------
// this is our logging message - things get memcpy'd into this when the data
// is available, and this is copied into the write buffer at the specified
// timeslot
static kbb_current_msg_t current_msg;
static kbb_01_01_t header_msg;

//-----------------------------------------------------------------------------
// we have 2 buffers of 96 messages each: 48kB each (512bytes * 96), for a total
// usage of 96kB. This should be the largest possible - any free SRAM should be
// dedicated to this, so we have the least number of writes, increasing throughput
// and write latencies and possible skips
#define BUFFER_COUNT			2
#define BUFFER_SIZE				96		// size in "log_msg_v01_t" units

// 21845 * 48kB is *just* below the 1GB limit, which gives enough space for a
// header too. Make sure that if you adjust the buffer size, this changes too
#define MAX_BUFFERS_PER_FILE	21845
//#define MAX_BUFFERS_PER_FILE	21 // debug value for ~1mb files (1032192 bytes)
//#define MAX_BUFFERS_PER_FILE	2184 // debug value for ~100mb files

typedef struct write_buffer_t write_buffer_t;
struct write_buffer_t
{
	kbb_current_msg_t buffer[BUFFER_SIZE];

	// this *HAS* to be 32bit so the the whole struct is 32bit aligned and
	// packed so that writes happen on 32bit borders, and therefore, FAST
	// instead of byte-by-byte copying
	int32_t current_idx;
};
static Semaphore 				write_buffer_semaphore;
static write_buffer_t 			write_buffers[BUFFER_COUNT];

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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Buffers and stuff
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// when the logger thread needs a new buffer, we call this - it's semaphore 
// protected
static int8_t
new_write_buffer_idx_to_fill(void)
{
	int8_t buffer_idx = -1;
	chSemWait(&write_buffer_semaphore);
	for ( int8_t idx = 0 ; idx < BUFFER_COUNT ; idx++ )
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

	// if -1, then no buffers are free...
	return buffer_idx;
}

//-----------------------------------------------------------------------------
// when the logger thread finishes with a buffer, we notify the writer thread,
// if it's sleeping
static void
return_write_buffer_idx_after_filling(int8_t idx)
{
	// lets lock the system before checking just in case
	chSysLock();
	if (writerThreadForSleep)
	{
		writerThreadForSleep->p_u.rdymsg = (msg_t)idx+1;
		chSchReadyI(writerThreadForSleep);
		writerThreadForSleep = NULL;
	}
	chSysUnlock();
}

//-----------------------------------------------------------------------------
static int8_t
new_write_buffer_idx_to_write(void)
{
	int8_t buffer_idx = -1;
	chSemWait(&write_buffer_semaphore);
	for ( int8_t idx = 0 ; idx < BUFFER_COUNT ; idx++ )
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
	// @TODO: optional? Do we really need to memset it? we ARE going to be
	// overwriting it
	memset(&write_buffers[idx],0,sizeof(write_buffers[idx]));
	// if the current idx is -1, then we can assume it's good to go
	write_buffers[idx].current_idx = -1;
	chSemSignal(&write_buffer_semaphore);
}

//-----------------------------------------------------------------------------
static void
flush_counters(void)
{
	current_msg.msg_num = 0;
	current_msg.write_errors = 0;
	current_msg.pps = 0;
}

//-----------------------------------------------------------------------------
// new file scenario / etc. 
static void
flush_write_buffers(void)
{
	chSemWait(&write_buffer_semaphore);
	memset(&write_buffers, 0, sizeof(write_buffers));
	for ( int8_t idx = 0 ; idx < BUFFER_COUNT ; idx++ )
		write_buffers[idx].current_idx = -1;

	flush_counters();

	chSemSignal(&write_buffer_semaphore);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// FatFS related stuff
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// If there's no "nanibox" directory, create it, doesn't do anything if it 
// exists
static uint8_t
make_dirs(void)
{
	FRESULT stat;
	DIR naniBox_dir;
	stat = f_opendir(&naniBox_dir, NANIBOX_DNAME);
	if (FR_NO_PATH == stat)
	{
		stat = f_mkdir(NANIBOX_DNAME);
	}
	return stat;
}

//-----------------------------------------------------------------------------
// This function kinda sucks, but it's understandable.
// We sequentially try to create files called KUROxxxx.KBB, where xxxx 0->0999,
// with the flags set to only open if file doesn't exists. So if the file
// does exists, we get a FR_EXIST return value, and try the next.
static uint8_t
new_file(void)
{
	char charbuf[64];
	MemoryStream msb;
	FRESULT stat = FR_OK;

	if ( kbfile.fs )
	{
		// file is open, close it first
		f_close(&kbfile);
	}

	uint16_t fnum = 0;
	for( ; fnum < 1000 ; fnum++ )
	{
		// the joys of doing an sprintf();
		memset(charbuf,0,sizeof(charbuf));
		msObjectInit(&msb, (uint8_t*)charbuf, sizeof(charbuf), 0);
		chprintf((BaseSequentialStream *)&msb, "%s/%s%.4d%s", NANIBOX_DNAME,
				KUROBOX_FNAME_STEM, fnum, KUROBOX_FNAME_EXT);
		stat = f_open(&kbfile, charbuf, FA_WRITE|FA_CREATE_NEW);

		if (stat == FR_EXIST)
			// it exists, just try the next
			continue;
		else if (stat != FR_OK)
			// we got a return code that's not FR_EXIST or FR_OK, which means
			// a different type of error, return from this.
			return stat;
		else
			// new file created, lets break from loop
			break;
	}

	// we can only get an OK if we opened the file
	if (stat == FR_OK)
	{
		// make sure that changes are flushed out to disk
		f_sync(&kbfile);

		memset(charbuf,0,sizeof(charbuf));
		msObjectInit(&msb, (uint8_t*)charbuf, sizeof(charbuf), 0);
		chprintf((BaseSequentialStream *)&msb, "%s%.4d", KUROBOX_FNAME_STEM, fnum);
		// since we don't want the full path or the extension, lets do that here
		kbs_setFName(charbuf);

		// here we write out the header to the file
		uint8_t * buf = (uint8_t*) &header_msg;
		// but first, checksum...
		header_msg.header.checksum = calc_checksum_16(buf+KBB_CHECKSUM_START,
									KBB_MSG_SIZE-KBB_CHECKSUM_START);
		UINT bytes_written = 0;
		stat = f_write(&kbfile, buf, header_msg.header.msg_size, &bytes_written);
		if (bytes_written != sizeof(header_msg.header.msg_size) || stat != FR_OK)
			current_msg.write_errors++;

		stat = f_sync(&kbfile);
		if (stat != FR_OK)
			current_msg.write_errors++;
	}

	return stat;
}

//-----------------------------------------------------------------------------
// the SD card has been inserted, do the necessary steps to get a sane system
static int
on_insert(void)
{
	FRESULT stat;

	kbs_setFName(KUROBOX_LOADING_NAME);
	if (sdcConnect(&SDCD1) != CH_SUCCESS)
	{
		kbs_setFName(KUROBOX_ERR1);
		return KB_NOT_OK;
	}

	stat = f_mount(0, &SDC_FS);
	if (stat != FR_OK)
	{
		kbs_setFName(KUROBOX_ERR2);
		sdcDisconnect(&SDCD1);
		return KB_NOT_OK;
	}

	chThdSleepMilliseconds(100);
	uint32_t clusters;
	FATFS * fsp = NULL;
	stat = f_getfree("/", &clusters, &fsp);
	if (stat != FR_OK)
	{
		kbs_setFName(KUROBOX_ERR3);
		sdcDisconnect(&SDCD1);
		return KB_NOT_OK;
	}
	uint64_t cardsize = clusters * (((uint32_t)fsp->csize * (uint32_t)MMCSD_BLOCK_SIZE) / 1024);
	cardsize_MB = cardsize / 1024;

	// @TODO: this can be moved to above the check for free space
	fs_write_protected = sdc_lld_is_write_protected(&SDCD1);
	if ( fs_write_protected )
	{
		// -1 means that it's write protected, display that
		kbs_setSDCFree(-1);
		kbs_setFName(KUROBOX_WP_NAME);
		return KB_NOT_OK;
	}
	kbs_setSDCFree(cardsize_MB);

	stat = make_dirs();
	if (stat != FR_OK)
	{
		kbs_setFName(KUROBOX_ERR4);
		sdcDisconnect(&SDCD1);
		return KB_NOT_OK;
	}

	stat = new_file();
	if (stat != FR_OK)
	{
		kbs_setFName(KUROBOX_ERR5);
		sdcDisconnect(&SDCD1);
		return KB_NOT_OK;
	}

	fs_ready = TRUE;
	logger_state = LS_RUNNING;

	kbs_err_setSD(1);

	return KB_OK;
}

//-----------------------------------------------------------------------------
// usually alreay too late to actually disconnect, but it's good to have
// things in a known state
static void
on_remove(void)
{
	sdcDisconnect(&SDCD1);
	kbs_setSDCFree(0);
	kbs_setFName(KUROBOX_BLANK_FNAME);
	kbs_err_setSD(0);
	fs_ready = FALSE;
}

//-----------------------------------------------------------------------------
// This is called by the writer thread, and doesn't return until the SD
// is in a known state, the inital header is written out, and all is ready to
// go for normal writing. It will exit if we have requested the thread should
// terminate (for shutdown)
static uint8_t
wait_for_sd(void)
{
	uint8_t sd_det_count = POLLING_COUNT;
	while(1)
	{
		if (chThdShouldTerminate())
		{
			logger_state = LS_EXITING;
			break;
		}

		if (sdc_lld_is_card_inserted(&SDCD1))
		{
			if (--sd_det_count == 0)
			{
				if ( on_insert() == KB_OK )
				{
					break;
				}
				else
				{
					sd_det_count = POLLING_COUNT;
					continue;
				}
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
// just checks up on the current status, and if there's no card, makes sure
// on_remove() is called
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
		}
	}
	return ret;
}

//-----------------------------------------------------------------------------
// main writer thread function - is called when SD card is ready and we're just
// going to keep writing until SD card is ejected.
// @TODO: when file exceeds 1GB, start a new file
static uint8_t
writing_run(void)
{
	uint8_t ret = 1;
	uint32_t writer_buffers_written = 0;
	while(LS_RUNNING == logger_state)
	{
		if (chThdShouldTerminate())
		{
			logger_state = LS_EXITING;
			f_sync(&kbfile);
			f_close(&kbfile);
			sdcDisconnect(&SDCD1);
			break;
		}

		int8_t idx = new_write_buffer_idx_to_write();
		if ( idx == -1 )
		{
			// no more buffers to write, go to sleep, wait to be notified
			chSysLock();
				writerThreadForSleep = chThdSelf();
			    chSchGoSleepS(THD_STATE_SUSPENDED);
			chSysUnlock();

			// we can get woken up by either a buffer ready to be written
			// or a request to terminate, either way, back to the start
			// of the loop to check.
			continue;
		}

		//----------------------------------------------------------------------
		// start of write
		kbg_setLED1(1);

		// here we have a full buffer ready to write
		write_buffer_t * buf = &write_buffers[idx];

		UINT bytes_written = 0;
		FRESULT stat = FR_OK;

		DEBG_WRITER_TIME_WRITES_TIMER(0);
		stat = f_write(&kbfile, buf->buffer, sizeof(buf->buffer), &bytes_written);
		DEBG_WRITER_TIME_WRITES_TIMER(1);

		kbg_setLED1(0);
		// end of write
		//----------------------------------------------------------------------

		if (bytes_written != sizeof(buf->buffer) || stat != FR_OK)
			current_msg.write_errors++;

		//----------------------------------------------------------------------
		// start of flush
		//kbg_setLED2(1);
		
		DEBG_WRITER_TIME_WRITES_TIMER(2);
		stat = f_sync(&kbfile);
		DEBG_WRITER_TIME_WRITES_TIMER(3);

		if (stat != FR_OK)
			current_msg.write_errors++;

		// we're done, return it
		return_write_buffer_idx_after_writing(idx);

		//kbg_setLED2(0);
		// end of flush
		//----------------------------------------------------------------------

#ifdef DEBG_WRITER_TIME_WRITES
		chprintf(DEBG, "%12f, %12f, %12f, %d, %6d, 0x%.8d\n",
				(t1-t0) / 168000.0f, (t3-t2) / 168000.0f, (t3-t0) / 168000.0f,
				idx, bytes_written, buf->buffer);
#endif // DEBG_WRITER_TIME_WRITES

		kbs_setWriteCount(current_msg.msg_num);
		kbs_setWriteErrors(current_msg.write_errors);

		// after X buffers, start a new file. this has to be in sync with
		// the logger thread, so that you both do roll over at the same time
		writer_buffers_written++;
		if ( writer_buffers_written > MAX_BUFFERS_PER_FILE )
		{
			writer_buffers_written = 0;
			// we've hit the file size limit, close the old, open a new file
			if ( new_file() != FR_OK )
			{
				// no new file? that's bad!!
				logger_state = LS_WAIT_FOR_SD;
				break;
			}
		}

		sd_card_status();
		if (LS_RUNNING != logger_state)
			break;

	}
	return ret;
}

//-----------------------------------------------------------------------------
// The writer thread run function, only returns on shutdown. It gets the SD card
// ready, and then just goes to sleep until there's something to write
// @TODO: reduce the working area size to its reasonable limit
static WORKING_AREA(waWriter, 1024);
static msg_t 
thWriter(void *arg)
{
	ASSERT(LS_INIT == logger_state, "thWriter, 1", "logger_state is not LS_INIT");
	(void)arg;

	chRegSetThreadName("Writer");

	// this data never changes, until we start added in new message types
	current_msg.header.preamble = KBB_PREAMBLE;
	current_msg.header.msg_class = KBB_CLASS_DATA;
	current_msg.header.msg_subclass = KBB_SUBCLASS_DATA_01;
	current_msg.header.msg_size = KBB_MSG_SIZE;

	header_msg.header.preamble = KBB_PREAMBLE;
	header_msg.header.msg_class = KBB_CLASS_HEADER;
	header_msg.header.msg_subclass = KBB_SUBCLASS_HEADER_01;
	header_msg.header.msg_size = KBB_MSG_SIZE;

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

	return KB_OK;
}

//-----------------------------------------------------------------------------
// The logger thread run function. 
// This thread copies the "current_msg" to the write buffer every 5ms. If a
// time slot is missed, it is noted and will try again next slot.
static WORKING_AREA(waLogger, 256);
static msg_t
thLogger(void *arg)
{
	(void)arg;

	chRegSetThreadName("Logger");

	// These nested while() loops require some explanation.
	// If the current state is running, we should be in the inner loop
	// always - just storing, etc. But if the SD is not ready yet,
	// we will be in the outer loop, flushing buffers, and signalling
	// 

	uint32_t logger_buffers_written = 0;
	current_msg.global_count = 0;

	while( !chThdShouldTerminate() && LS_EXITING != logger_state)
	{
		flush_write_buffers();
		int8_t current_idx = -1;
		systime_t sleep_until = chTimeNow() + MS2ST(5);

		while( !chThdShouldTerminate() && LS_RUNNING == logger_state )
		{
			if ( current_idx == -1 )
			{
				current_idx = new_write_buffer_idx_to_fill();
				if ( current_idx == -1 )
				{
					// no buffers? sleep and try again
					chThdSleepMilliseconds(1);
					continue;
				}
			}

			// here we'll have a good buffer to fill up until it's all full!
			write_buffer_t * wb = &write_buffers[current_idx];
			{
				kbb_current_msg_t * lm = &wb->buffer[wb->current_idx];

				chSysLock();
				// make sure that we complete a write uninterrupted
				memcpy(lm, &current_msg, sizeof(current_msg));
				chSysUnlock();

				rtcGetTimeTm(&RTCD1, &lm->rtc);

				uint8_t * buf = (uint8_t*) lm;
				// NOTHING must get written after this, ok?
				lm->header.checksum = calc_checksum_16(buf+KBB_CHECKSUM_START,
						KBB_MSG_SIZE-KBB_CHECKSUM_START);
			}

			// chTimeNow() will roll over every ~49 days
			// @TODO: make this code handle that (or not, 49 days is a lot!!)
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
			current_msg.global_count++;

			// @TODO: fix this!
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

				// now we check to see if we need to roll over into a new
				// file, this has to be in sync with the writer thread!!
				logger_buffers_written++;
				if ( logger_buffers_written > MAX_BUFFERS_PER_FILE )
				{
					logger_buffers_written = 0;
					// we've hit the file size limit, restart counters
					flush_counters();
				}
			}
		}
		chThdSleepMilliseconds(1);
	}
	return KB_OK;
}

//-----------------------------------------------------------------------------
int
kuroBoxWriterInit(void)
{
	ASSERT(LS_INIT == logger_state, "kuroBoxLogger, 1", "logger_state is not LS_INIT");

	chSemInit(&write_buffer_semaphore, 1);
	kbs_setFName(KUROBOX_BLANK_FNAME);
	// start with the status set to no SD, just to make sure, on_insert() will
	// change it
	kbs_err_setSD(0);

	loggerThread = chThdCreateStatic(waLogger, sizeof(waLogger), NORMALPRIO, thLogger, NULL);
	writerThread = chThdCreateStatic(waWriter, sizeof(waWriter), HIGHPRIO, thWriter, NULL);

	return KB_OK;
}

//-----------------------------------------------------------------------------
int
kuroBoxWriterStop(void)
{
	chThdTerminate(loggerThread);
	chThdTerminate(writerThread);

	chThdWait(loggerThread);
	// this thread may be in its own sleep
	chSysLock();
	if (writerThreadForSleep)
	{
		writerThreadForSleep->p_u.rdymsg = (msg_t)10; // just a random non-0
		chSchReadyI(writerThreadForSleep);
		writerThreadForSleep = NULL;
	}
	chSysUnlock();

	chThdWait(writerThread);

	return KB_OK;
}

//-----------------------------------------------------------------------------
void
kbw_setLTC(ltc_frame_t * ltc_frame)
{
	memcpy(&current_msg.ltc_frame, ltc_frame, sizeof(current_msg.ltc_frame));
}

//-----------------------------------------------------------------------------
void
kbw_incPPS(void)
{
	current_msg.pps++;
}

//-----------------------------------------------------------------------------
void kbw_incOneSecPPS(void)
{
	current_msg.one_sec_pps++;
}

//-----------------------------------------------------------------------------
void
kbw_setGpsNavSol(ubx_nav_sol_t * nav_sol)
{
	memcpy(&current_msg.nav_sol, nav_sol, sizeof(current_msg.nav_sol));
}

//-----------------------------------------------------------------------------
void
kbw_setVNav(vnav_data_t * vnav)
{
	memcpy(&current_msg.vnav, vnav, sizeof(current_msg.vnav));
}

//-----------------------------------------------------------------------------
void
kbw_setAltitude(float altitude, float temperature)
{
	current_msg.altitude = altitude;
	current_msg.temperature = temperature;
}

//-----------------------------------------------------------------------------
void 
kbwh_setVNav(uint8_t * data, uint16_t length)
{
	ASSERT(sizeof(header_msg.vnav_header) == length, "kbw_header_vnav", "Length of vnav_header does not match");
	memcpy(&header_msg.vnav_header, data, sizeof(header_msg.vnav_header));
}
