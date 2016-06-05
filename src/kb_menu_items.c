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
#include "kb_menu_items.h"
#include "kb_serial.h"
#include "kb_externalDisplay.h"
#include "kb_gpio.h"
#include "kb_gps.h"
#include "kb_time.h"
#include "kb_screen.h"
#include <time.h>

//-----------------------------------------------------------------------------
extern int16_t current_menu_item;
bool_t kuroBox_request_standby;

//-----------------------------------------------------------------------------
void 
mi_exit(void * data)
{
	(void)data;
	current_menu_item = NO_MENU_ITEM;
}

//-----------------------------------------------------------------------------
void 
mi_eDisplay_changeInterval(void * data)
{
	(void)data;
	kbfa_changeInterval();
}

//-----------------------------------------------------------------------------
int 
mi_eDisplay_getInterval(void)
{
	return kbed_getInterval();
}

//-----------------------------------------------------------------------------
void 
mi_eDisplay_changeSerialPort(void * data)
{
	(void)data;
	kbfa_changeSerialPort();
}

//-----------------------------------------------------------------------------
int
mi_eDisplay_getSerialPort(void)
{
	return kbed_getSerialPort();
}

//-----------------------------------------------------------------------------
void
mi_serial1_pwr(void * data)
{
	(void)data;
	kbse_setPowerSerial1(!kbse_getPowerSerial1());
}

//-----------------------------------------------------------------------------
void 
mi_serial1_baud(void * data)
{
	(void)data;
	kbse_changeBaudSerial1();
}

//-----------------------------------------------------------------------------
int 
mi_serial1_getbaud(void)
{
	return kbse_getBaudSerial1();
}

//-----------------------------------------------------------------------------
void 
mi_serial2_pwr(void * data)
{
	(void)data;
	kbse_setPowerSerial2(!kbse_getPowerSerial2());
}

//-----------------------------------------------------------------------------
void 
mi_serial2_baud(void * data)
{
	(void)data;
	kbse_changeBaudSerial2();
}

//-----------------------------------------------------------------------------
int 
mi_serial2_getbaud(void)
{
	return kbse_getBaudSerial2();
}

//-----------------------------------------------------------------------------
void mi_metric_units(void * data)
{
	(void)data;
	kbs_changeMetricUnits();
}

//-----------------------------------------------------------------------------
int mi_get_metric_units(void)
{
	return kbs_getMetricUnits();
}

//-----------------------------------------------------------------------------
void 
mi_led3(void * data)
{
	(void)data;
	kbg_setLED3(!kbg_getLED3());
}

//-----------------------------------------------------------------------------
void 
mi_lcd_backlight(void * data)
{
	(void)data;
	kbg_setLCDBacklight(!kbg_getLCDBacklight());
}

//-----------------------------------------------------------------------------
void 
mi_time_from_gps(void * data)
{
	(void)data;
	const struct ubx_nav_sol_t * nav_sol = kbg_getUbxNavSol();
	struct tm timp;
	rtcGetTimeTm(&RTCD1, &timp);
	uint32_t secs_today = (nav_sol->itow/1000) % (60*60*24);
	timp.tm_hour = secs_today/3600;
	timp.tm_min = (secs_today%3600)/60;
	timp.tm_sec = secs_today%60;
	rtcSetTimeTm(&RTCD1, &timp);
	mi_exit(NULL);
}

//-----------------------------------------------------------------------------
void 
mi_time_from_ltc(void * data)
{
	(void)data;
	const struct smpte_timecode_t * ltc = kbt_getLTC();
	struct tm timp;
	rtcGetTimeTm(&RTCD1, &timp);
	timp.tm_hour = ltc->hours;
	timp.tm_min = ltc->minutes;
	timp.tm_sec = ltc->seconds;
	rtcSetTimeTm(&RTCD1, &timp);
	mi_exit(NULL);
}

//-----------------------------------------------------------------------------
void 
mi_standby(void * data)
{
	(void)data;
	kuroBox_request_standby = 1;
	mi_exit(NULL);
}
