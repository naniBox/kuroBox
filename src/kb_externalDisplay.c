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
#include "kb_externalDisplay.h"
#include "kb_serial.h"
#include "kb_writer.h"
#include <chprintf.h>
#include <string.h>

//-----------------------------------------------------------------------------
#include "kbb_types.h"
KBB_TYPES_VERSION_CHECK(0x0002)

//-----------------------------------------------------------------------------
static int ed_interval;
#define EXTERNAL_DISPLAY_INTERVAL_LTC			0
#define EXTERNAL_DISPLAY_INTERVAL_ONE_SEC		1
#define EXTERNAL_DISPLAY_INTERVAL_VN			2
SerialDriver * ed_serialPort;

static kbb_display_t kbb_display;

//-----------------------------------------------------------------------------
static Thread * eDisplayThread;
static Thread * eDisplayThreadForSleep;
static WORKING_AREA(waEDisplay, 512);
static msg_t
thEDisplay(void *arg)
{
	(void)arg;
	chRegSetThreadName("eDisplay");

	kbb_display.header.preamble = KBB_PREAMBLE;
	kbb_display.header.msg_class = KBB_CLASS_EXTERNAL;
	kbb_display.header.msg_subclass = KBB_SUBCLASS_EXTERNAL_01;
	kbb_display.header.msg_size = KBB_DISPLAY_SIZE;

	while( !chThdShouldTerminate() )
	{
		chSysLock();
			eDisplayThreadForSleep = chThdSelf();
		    chSchGoSleepS(THD_STATE_SUSPENDED);
		chSysUnlock();

		if ( !ed_serialPort )
			continue;

		const kbb_current_msg_t * msg = kbw_getCurrentMsg();
		memcpy(&kbb_display.ltc_frame, 		&msg->ltc_frame, 	sizeof(kbb_display.ltc_frame));
		memcpy(&kbb_display.smpte_time, 	&msg->smpte_time, 	sizeof(kbb_display.smpte_time));
		kbb_display.ecef[0] = msg->nav_sol.ecefX;
		kbb_display.ecef[1] = msg->nav_sol.ecefY;
		kbb_display.ecef[2] = msg->nav_sol.ecefZ;
		memcpy(&kbb_display.vnav, 			&msg->vnav, 		sizeof(kbb_display.vnav));
		kbb_display.temperature = msg->temperature;

		uint8_t * buf = (uint8_t*) &kbb_display;
		kbb_display.header.checksum = calc_checksum_16(buf+KBB_CHECKSUM_START,
				KBB_DISPLAY_SIZE-KBB_CHECKSUM_START);

		// display!!!
		sdWrite(ed_serialPort, buf, KBB_DISPLAY_SIZE);
	}
	return 0;
}

//-----------------------------------------------------------------------------
void
kbed_dataReadyI(void)
{
	if (eDisplayThreadForSleep)
	{
		eDisplayThreadForSleep->p_u.rdymsg = (msg_t)1;
		chSchReadyI(eDisplayThreadForSleep);
		eDisplayThreadForSleep = NULL;
	}
}

//-----------------------------------------------------------------------------
int kuroBoxExternalDisplayInit(void)
{
	eDisplayThread = chThdCreateStatic(waEDisplay, sizeof(waEDisplay), NORMALPRIO, thEDisplay, NULL);
	return KB_OK;
}

//-----------------------------------------------------------------------------
int kuroBoxExternalDisplayStop(void)
{
	chThdTerminate(eDisplayThread);

	chSysLock();
	if (eDisplayThreadForSleep)
	{
		eDisplayThreadForSleep->p_u.rdymsg = (msg_t)1; // just a random non-0
		chSchReadyI(eDisplayThreadForSleep);
		eDisplayThreadForSleep = NULL;
	}
	chSysUnlock();

	chThdWait(eDisplayThread);
	eDisplayThread = NULL;
	return KB_OK;
}

//-----------------------------------------------------------------------------
int kbed_getInterval(void)
{
	return ed_interval;
}

//-----------------------------------------------------------------------------
void kbfa_setInterval(int interval)
{
	if ( interval >= EXTERNAL_DISPLAY_INTERVAL_LTC &&
		 interval <= EXTERNAL_DISPLAY_INTERVAL_VN)
		ed_interval = interval;
}

//-----------------------------------------------------------------------------
int kbfa_changeInterval(void)
{
	if ( ed_interval == EXTERNAL_DISPLAY_INTERVAL_VN )
		ed_interval = EXTERNAL_DISPLAY_INTERVAL_LTC;
	else
		ed_interval++;
	return 0;
}

//-----------------------------------------------------------------------------
int kbed_getSerialPort(void)
{
	if ( ed_serialPort == &Serial1 ) 	return 1;
	if ( ed_serialPort == &Serial2 )	return 2;
	return 0;
}

//-----------------------------------------------------------------------------
void kbfa_setSerialPort(int serialPort)
{
	if      ( serialPort == 1 ) ed_serialPort = &Serial1;
	else if ( serialPort == 2 ) ed_serialPort = &Serial2;
	else if ( serialPort == 0 ) ed_serialPort = NULL;
}

//-----------------------------------------------------------------------------
int kbfa_changeSerialPort(void)
{
	if      ( ed_serialPort == &Serial1 ) 	ed_serialPort = &Serial2;
	else if ( ed_serialPort == &Serial2 ) 	ed_serialPort = NULL;
	else if ( ed_serialPort == NULL		)	ed_serialPort = &Serial1;
	return 0;
}
