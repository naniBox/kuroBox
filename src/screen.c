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

#include "screen.h"
#include "nanibox_util.h"
#include "ST7565.h"
#include "ltc.h"
#include <memstreams.h>
#include <chprintf.h>
#include <string.h>

//-----------------------------------------------------------------------------
typedef struct kuroBoxScreen kuroBoxScreen;
struct kuroBoxScreen
{
	SMPTETimecode ltc;
	uint16_t voltage;
	uint32_t sdc_free;
	char sdc_fname[23]; // we can fit 128/26=21.3 chars per line, i think
};

//-----------------------------------------------------------------------------
//static Thread * screenThread;
static kuroBoxScreen screen;
static char charbuf[128];
static MemoryStream msb;

//-----------------------------------------------------------------------------
static WORKING_AREA(waScreen, 128);
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
		chprintf(bss,"%4dMB", screen.sdc_free>1000?1000:screen.sdc_free);
		st7565_drawstring(&ST7565D1, C2P(10), 0, charbuf);

		// input voltage
		INIT_CBUF();
		chprintf(bss,"%2dV", screen.voltage/10);
		st7565_drawstring(&ST7565D1, -C2P(3), 0, charbuf);

		INIT_CBUF();
		chprintf(bss,"T: %.2d:%.2d:%.2d.%.2d",
				screen.ltc.hours,
				screen.ltc.mins,
				screen.ltc.secs,
				screen.ltc.frame);
		st7565_drawstring(&ST7565D1, 0, 1, charbuf);



		st7565_display(&ST7565D1);
		chThdSleepMilliseconds(500);
	}
	#undef INIT_CBUF
	return 0;
}

//-----------------------------------------------------------------------------
int kuroBoxScreenInit()
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