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
#include "kb_screen.h"
#include "kb_util.h"
#include "kb_gps.h"
#include "kb_menu.h"
#include "kb_serial.h"
#include "kb_gpio.h"
#include "ST7565.h"
#include <memstreams.h>
#include <chrtclib.h>
#include <chprintf.h>
#include <string.h>
#include <time.h>

//-----------------------------------------------------------------------------
// I usually keep this at 1000/24 ~= 40, HOWEVER, the LCD's refresh rate
// is about 100ms, so, set it to that instead.
#define SCREEN_REFRESH_SLEEP			100
#define SCREEN_OK						0x00000000
#define SCREEN_ERR_NO_SD				0x00000001
#define SCREEN_ERR_NO_LTC				0x00000002

//-----------------------------------------------------------------------------
// @TODO: is memset optional?
#define INIT_CBUF() \
	memset(charbuf,0,sizeof(charbuf));\
	msObjectInit(&msb, (uint8_t*)charbuf, 128, 0);
#define bss ((BaseSequentialStream *)&msb)

//-----------------------------------------------------------------------------
typedef struct kuroBoxScreen kuroBoxScreen;
struct __PACKED__ kuroBoxScreen
{
	smpte_timecode_t ltc;
	uint16_t voltage;
	int16_t temperature;
	int32_t sdc_free;
	char sdc_fname[23]; // we can fit 128/6=21.3 chars per line
	uint32_t write_count;
	uint32_t write_errors;
	uint8_t btn0;
	uint8_t btn1;
	uint8_t pps;
	int32_t ecef[3];
	float ypr[3];
	int32_t error;
};

//-----------------------------------------------------------------------------
static Thread * screenThread;
static kuroBoxScreen screen;
static char charbuf[128];
static MemoryStream msb;

//-----------------------------------------------------------------------------
static void
draw(void)
{
	struct tm timp;
	rtcGetTimeTm(&RTCD1, &timp);

	st7565_clear(&ST7565D1);

//----------------------------------------------------------------------------
	INIT_CBUF();
	chprintf(bss,"%s",
			screen.sdc_fname);
	st7565_drawstring(&ST7565D1, 0, 0, "F");
	st7565_drawstring(&ST7565D1, C2P(1)+2, 0, charbuf);

	// free bytes
	INIT_CBUF();
	// only display between 0-9999; <0 indicates read only, >9999 is because our
	// screen realestate is limited.
	int32_t sdc_free = screen.sdc_free<0?0:screen.sdc_free>9999?9999:screen.sdc_free;
	chprintf(bss,"%4dMB", sdc_free);
	st7565_drawstring(&ST7565D1, C2P(-10), 0, charbuf);
	if ( screen.sdc_free<0 )
		st7565_drawline(&ST7565D1, C2P(-8), CHAR_HEIGHT, C2P(-4), 0, COLOUR_BLACK);

	// input voltage
	INIT_CBUF();
	chprintf(bss,"%2dV", screen.voltage/10);
	st7565_drawstring(&ST7565D1, -C2P(3), 0, charbuf);

	// core temperature
	//INIT_CBUF();
	//chprintf(bss,"%2d", screen.temperature);
	//st7565_drawstring(&ST7565D1, -C2P(2), 0, charbuf);

//----------------------------------------------------------------------------
	// LTC

	INIT_CBUF();
	chprintf(bss,"%.2d:%.2d:%.2d.%.2d  %.2d%.2d%.2d",
			screen.ltc.hours,
			screen.ltc.minutes,
			screen.ltc.seconds,
			screen.ltc.frames,
			timp.tm_hour,timp.tm_min,timp.tm_sec);
	st7565_drawstring(&ST7565D1, 0, 1, "T");
	st7565_drawstring(&ST7565D1, C2P(1)+2, 1, charbuf);

//----------------------------------------------------------------------------
	INIT_CBUF();
	chprintf(bss,"%4i/%4i/%4i",
			(int)screen.ypr[0], (int)screen.ypr[1], (int)screen.ypr[2]);
	st7565_drawstring(&ST7565D1, 0, 2, "A");
	st7565_drawstring(&ST7565D1, C2P(1)+2, 2, charbuf);

	INIT_CBUF();
	chprintf(bss,"%iC", (int)screen.temperature);
	st7565_drawstring(&ST7565D1, C2P(-3), 2, charbuf);

//----------------------------------------------------------------------------
	float lat,lon,alt;
	lat=lon=alt=0.0f;
	kbg_ecef2lla(screen.ecef[0], screen.ecef[1], screen.ecef[2],
			&lat, &lon, &alt);

	INIT_CBUF();
	chprintf(bss,"%3f/%3f/%3f",
			lat, lon, alt);
	st7565_drawstring(&ST7565D1, 0, 3, "G");
	st7565_drawstring(&ST7565D1, C2P(1)+2, 3, charbuf);

//----------------------------------------------------------------------------
	// @OTODO: remove these, they are just for show!
	//
	// v--  from here  --v
	//
	//
#ifdef _DEBUG

	/*
	INIT_CBUF();
	chprintf(bss,"C/E %d/%d",
			screen.write_count, screen.write_errors);
	st7565_drawstring(&ST7565D1, 0, 1, "T");
	st7565_drawstring(&ST7565D1, C2P(1)+2, 1, charbuf);
	 */
	if ( screen.btn0 )
		st7565_drawline(&ST7565D1, C2P(-4), 0, C2P(-4), CHAR_HEIGHT-1, COLOUR_BLACK);
	if ( screen.btn1 )
		st7565_drawline(&ST7565D1, C2P(-4)+2, 0, C2P(-4)+2, CHAR_HEIGHT-1, COLOUR_BLACK);

	INIT_CBUF();
	uint8_t idle_time = 100*chThdGetTicks(chSysGetIdleThread()) / chTimeNow();
	chprintf(bss,"%d", idle_time);
	st7565_drawstring(&ST7565D1, C2P(-6), 0, charbuf);
#endif
	//
	//
	// ^-- to here  --^
	//
//----------------------------------------------------------------------------

	// we want to draw this over the top, so put it last.
	st7565_drawline(&ST7565D1, C2P(1), 0, C2P(1), 31, COLOUR_BLACK);
	if ( screen.pps )
	{
		screen.pps--;
		st7565_fillrect(&ST7565D1, C2P(-1), CHAR_HEIGHT+1, CHAR_WIDTH-1, CHAR_HEIGHT, COLOUR_BLACK);
	}

	st7565_display(&ST7565D1);
}

//-----------------------------------------------------------------------------
static int
draw_error(void)
{
	if ( screen.error == SCREEN_OK )
		return 0;
	if ( ( chTimeNow() >> 11 ) & 0x01 ) // yeah, not chosen scientifically
		return 0;

	st7565_clear(&ST7565D1);
	INIT_CBUF();
	chprintf(bss, "ERRORS (0x%.4X)", screen.error);
	st7565_drawstring(&ST7565D1, 0, 0, charbuf);
	int line = 1;
	if ( screen.error & SCREEN_ERR_NO_SD )
		st7565_drawstring(&ST7565D1, 0, line++, "No SD Card");
	if ( screen.error & SCREEN_ERR_NO_LTC )
		st7565_drawstring(&ST7565D1, 0, line++, "No Initial TimeCode");

	st7565_display(&ST7565D1);

	return 1;
}

//-----------------------------------------------------------------------------
static WORKING_AREA(waScreen, 512);
static msg_t 
thScreen(void *arg) 
{
	(void)arg;
	chRegSetThreadName("Screen");
	
	// let the splash screen get some glory, but not 
	// in debug, gotta get work DONE!
#ifdef _DEBUG
	chThdSleepMilliseconds(500);
#else
	chThdSleepMilliseconds(4000);
#endif
	st7565_clear(&ST7565D1);
	st7565_display(&ST7565D1);
	
	while( !chThdShouldTerminate() )
	{
		// are we displaying the menu?
		if ( kbm_display() )
		{
			chThdSleepMilliseconds(SCREEN_REFRESH_SLEEP);
		}
		else if ( draw_error() )
		{
			chThdSleepMilliseconds(SCREEN_REFRESH_SLEEP);
		}
		else
		{
			draw();
			chThdSleepMilliseconds(SCREEN_REFRESH_SLEEP);
		}

	}
	#undef INIT_CBUF

	st7565_clear(&ST7565D1);
	st7565_display(&ST7565D1);

	return 0;
}

//-----------------------------------------------------------------------------
static Thread * blinkerThread;
static WORKING_AREA(waBlinker, 64);
static msg_t
thBlinker(void *arg)
{
	(void)arg;
	chRegSetThreadName("Blinker");
	while( !chThdShouldTerminate() )
	{
		if ( screen.error & SCREEN_ERR_NO_SD )
		{
			for ( int i = 0 ; i < 2 ; i++ )
			{
				kbg_setLED3(1);
				chThdSleepMilliseconds(30);
				kbg_setLED3(0);
				chThdSleepMilliseconds(100);
			}
			chThdSleepMilliseconds(500);
		}

		if ( screen.error & SCREEN_ERR_NO_LTC )
		{
			for ( int i = 0 ; i < 4 ; i++ )
			{
				kbg_setLED3(1);
				chThdSleepMilliseconds(20);
				kbg_setLED3(0);
				chThdSleepMilliseconds(50);
			}
			chThdSleepMilliseconds(500);
		}
		// 10 short sleeps so that shutting down is quicker
		for ( int i = 0 ; i < 10 && !chThdShouldTerminate() ; i++ )
			chThdSleepMilliseconds(100);
	}
	return 0;
}


//-----------------------------------------------------------------------------
int 
kuroBoxScreenInit(void)
{
	memset(&screen, 0, sizeof(screen));
	screenThread = chThdCreateStatic(waScreen, sizeof(waScreen), NORMALPRIO, thScreen, NULL);
	blinkerThread = chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, thBlinker, NULL);
	return KB_OK;
}

//-----------------------------------------------------------------------------
int 
kuroBoxScreenStop(void)
{
	chThdTerminate(blinkerThread);
	chThdTerminate(screenThread);
	chThdWait(blinkerThread);
	chThdWait(screenThread);
	kbg_setLCDBacklight(0);

	return KB_OK;
}

//-----------------------------------------------------------------------------
void 
kbs_setVoltage(uint16_t volts)
{
	screen.voltage = volts;
}

//-----------------------------------------------------------------------------
void 
kbs_setTemperature(int16_t temperature)
{
	screen.temperature = temperature;
}

//-----------------------------------------------------------------------------
void 
kbs_setLTC(smpte_timecode_t * ltc)
{
	memcpy(&screen.ltc, ltc, sizeof(screen.ltc));
}

//-----------------------------------------------------------------------------
void 
kbs_setWriteCount(uint32_t write_count)
{
	screen.write_count = write_count;
}

//-----------------------------------------------------------------------------
void 
kbs_setWriteErrors(uint32_t write_errors)
{
	screen.write_errors = write_errors;
}

//-----------------------------------------------------------------------------
void 
kbs_setSDCFree(int32_t sdc_free)
{
	screen.sdc_free = sdc_free;
}

//-----------------------------------------------------------------------------
void 
kbs_setFName(const char * fname)
{
	strcpy(screen.sdc_fname, fname);
}

//-----------------------------------------------------------------------------
void 
kbs_btn0(uint8_t on)
{
	screen.btn0 = on;
}

//-----------------------------------------------------------------------------
void 
kbs_btn1(uint8_t on)
{
	screen.btn1 = on;
}

//-----------------------------------------------------------------------------
void 
kbs_PPS(void)
{
	screen.pps = 1000 / (SCREEN_REFRESH_SLEEP*10);
}

//-----------------------------------------------------------------------------
void 
kbs_setGpsEcef(int32_t x, int32_t y, int32_t z)
{
	screen.ecef[0] = x;
	screen.ecef[1] = y;
	screen.ecef[2] = z;
}

//-----------------------------------------------------------------------------
void 
kbs_setYPR(float yaw, float pitch, float roll)
{
	screen.ypr[0] = yaw;
	screen.ypr[1] = pitch;
	screen.ypr[2] = roll;
}

//-----------------------------------------------------------------------------
void
kbs_err_setSD(bool_t has_sd)
{
	if ( has_sd )
		screen.error &= ~SCREEN_ERR_NO_SD;
	else
		screen.error |= SCREEN_ERR_NO_SD;
}

//-----------------------------------------------------------------------------
void
kbs_err_setLTC(bool_t has_ltc)
{
	if ( has_ltc )
		screen.error &= ~SCREEN_ERR_NO_LTC;
	else
		screen.error |= SCREEN_ERR_NO_LTC;
}
