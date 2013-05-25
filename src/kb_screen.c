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

#include "kb_screen.h"
#include "nanibox_util.h"
#include "ST7565.h"
#include <memstreams.h>
#include <chrtclib.h>
#include <chprintf.h>
#include <string.h>
#include <time.h>

//-----------------------------------------------------------------------------
#define SCREEN_REFRESH_SLEEP			40

//-----------------------------------------------------------------------------
typedef struct kuroBoxScreen kuroBoxScreen;
struct kuroBoxScreen
{
	SMPTETimecode ltc;
	uint16_t voltage;
	int16_t temperature;
	int32_t sdc_free;
	char sdc_fname[23]; // we can fit 128/26=21.3 chars per line, i think
	uint32_t counter;
	uint8_t btn0;
	uint8_t btn1;
};

//-----------------------------------------------------------------------------
//static Thread * screenThread;
static kuroBoxScreen screen;
static char charbuf[128];
static MemoryStream msb;

//-----------------------------------------------------------------------------
static WORKING_AREA(waScreen, 1024*4);
static msg_t 
thScreen(void *arg) 
{
	(void)arg;
	chRegSetThreadName("Screen");
	

	// let the splash screen get some glory, but not 
	// in debug, gotta get work DONE!
#ifdef NDEBUG
	chThdSleepMilliseconds(2000);
#else
	chThdSleepMilliseconds(500);
#endif
	st7565_clear(&ST7565D1);
	st7565_display(&ST7565D1);
	
	// @TODO: is memset optional?
	#define INIT_CBUF() \
		memset(charbuf,0,sizeof(charbuf));\
		msObjectInit(&msb, (uint8_t*)charbuf, 128, 0);
	BaseSequentialStream * bss = (BaseSequentialStream *)&msb;
	while( !chThdShouldTerminate() )
	{
		st7565_clear(&ST7565D1);
		
		// 1st line contains title, voltage, MB free...		
		st7565_drawstring(&ST7565D1, 0, 0, "kuroBox");

		// free bytes
		INIT_CBUF();
		int32_t sdc_free = screen.sdc_free<0?-screen.sdc_free:screen.sdc_free;
		chprintf(bss,"%4dMB", sdc_free);
		st7565_drawstring(&ST7565D1, C2P(10), 0, charbuf);
		if ( screen.sdc_free<0 )
			st7565_drawline(&ST7565D1, C2P(10), CHAR_HEIGHT, C2P(16), 0, COLOUR_BLACK);

		// input voltage
		INIT_CBUF();
		chprintf(bss,"%2dV", screen.voltage/10);
		st7565_drawstring(&ST7565D1, -C2P(3), 0, charbuf);

		// core temperature
		//INIT_CBUF();
		//chprintf(bss,"%2d", screen.temperature);
		//st7565_drawstring(&ST7565D1, -C2P(2), 0, charbuf);

		// LTC
		INIT_CBUF();
		chprintf(bss,"T: %.2d:%.2d:%.2d.%.2d",
				screen.ltc.hours,
				screen.ltc.mins,
				screen.ltc.secs,
				screen.ltc.frame);
		st7565_drawstring(&ST7565D1, 0, 1, charbuf);

		// file name & count
		INIT_CBUF();
		chprintf(bss,"F: %s /%d",
				screen.sdc_fname,
				screen.counter);
		st7565_drawstring(&ST7565D1, 0, 2, charbuf);

		//-----------------------------------------------------------------------------
		//-----------------------------------------------------------------------------
		// @OTODO: remove these, they are just for show!

		INIT_CBUF();
		struct tm timp;
		rtcGetTimeTm(&RTCD1, &timp);
		chprintf(bss, "%.4d%.2d%.2d %.2d%.2d%.2d",timp.tm_year+1900,timp.tm_mon+1,timp.tm_mday,
		   timp.tm_hour,timp.tm_min,timp.tm_sec);
		st7565_drawstring(&ST7565D1, 0, 3, charbuf);

		INIT_CBUF();
		chprintf(bss,"%d%d", screen.btn0, screen.btn1);
		st7565_drawstring(&ST7565D1, C2P(7), 0, charbuf);

		INIT_CBUF();
		uint8_t idle_time = 100*chThdGetTicks(chSysGetIdleThread()) / chTimeNow();
		chprintf(bss,"%d", idle_time);
		st7565_drawstring(&ST7565D1, C2P(-2), 3, charbuf);


		st7565_display(&ST7565D1);
		chThdSleepMilliseconds(SCREEN_REFRESH_SLEEP);
	}
	#undef INIT_CBUF
	return 0;
}

//-----------------------------------------------------------------------------
int kuroBoxScreenInit(void)
{
	memset(&screen, 0, sizeof(screen));
	/*screenThread = */chThdCreateStatic(waScreen, sizeof(waScreen), NORMALPRIO, thScreen, NULL);
	return KB_OK;
}

//-----------------------------------------------------------------------------
void kbs_setVoltage(uint16_t volts)
{
	screen.voltage = volts;
}

//-----------------------------------------------------------------------------
void kbs_setTemperature(int16_t temperature)
{
	screen.temperature = temperature;
}

//-----------------------------------------------------------------------------
void kbs_setLTC(SMPTETimecode * ltc)
{
	memcpy(&screen.ltc, ltc, sizeof(screen.ltc));
}

//-----------------------------------------------------------------------------
void kbs_setCounter(uint32_t count)
{
	screen.counter = count;
}

//-----------------------------------------------------------------------------
void kbs_setSDCFree(int32_t sdc_free)
{
	screen.sdc_free = sdc_free;
}

//-----------------------------------------------------------------------------
void kbs_setFName(const char * fname)
{
	strcpy(screen.sdc_fname, fname);
}

//-----------------------------------------------------------------------------
void kbs_setBtn0(uint8_t on)
{
	screen.btn0 = on;
}

//-----------------------------------------------------------------------------
void kbs_setBtn1(uint8_t on)
{
	screen.btn1 = on;
}
