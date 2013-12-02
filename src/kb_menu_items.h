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
#ifndef _naniBox_kuroBox_menu_items
#define _naniBox_kuroBox_menu_items

//-----------------------------------------------------------------------------
#include "kb_util.h"

//-----------------------------------------------------------------------------
#define NO_MENU_ITEM				-1

//-----------------------------------------------------------------------------
typedef void (*menucallback_cb)(void * data);
typedef int (*menuextra_data_cb)(void);

//-----------------------------------------------------------------------------
void mi_exit(void * data);

void mi_eDisplay_changeInterval(void * data);
int mi_eDisplay_getInterval(void);

void mi_eDisplay_changeSerialPort(void * data);
int mi_eDisplay_getSerialPort(void);

void mi_serial1_pwr(void * data);
void mi_serial1_baud(void * data);
int mi_serial1_getbaud(void);

void mi_serial2_pwr(void * data);
void mi_serial2_baud(void * data);
int mi_serial2_getbaud(void);

void mi_metric_units(void * data);
int mi_get_metric_units(void);

void mi_led3(void * data);
void mi_lcd_backlight(void * data);
void mi_time_from_gps(void * data);
void mi_time_from_ltc(void * data);
void mi_standby(void * data);

#endif // _naniBox_kuroBox_menu
