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
#include "kb_menu.h"
#include "ST7565.h"
#include <memstreams.h>
#include <chprintf.h>
#include <string.h>

//-----------------------------------------------------------------------------
#define REFRESH_SLEEP 				50
#define MENU_ITEMS_COUNT 			6
bool_t kuroBox_request_standby;

//-----------------------------------------------------------------------------
typedef void (*menucallback_t)(void * data);
typedef struct menu_item_t menu_item_t;
struct menu_item_t
{
	char title[32];
	bool_t active;
	menucallback_t callback;
	void * data;
};

//-----------------------------------------------------------------------------
int16_t current_item = -1;

//-----------------------------------------------------------------------------
static void mi_exit(void * data)
{
	(void)data;
	current_item = -1;
}

//-----------------------------------------------------------------------------
static void mi_serial1_pwr(void * data)
{
	(void)data;
	palTogglePad(GPIOD, GPIOD_SERIAL1_PWR);
}

//-----------------------------------------------------------------------------
static void mi_serial2_pwr(void * data)
{
	(void)data;
	palTogglePad(GPIOD, GPIOD_SERIAL2_PWR);
}

//-----------------------------------------------------------------------------
static void mi_led3(void * data)
{
	(void)data;
	palTogglePad(GPIOA, GPIOA_LED3);
}

//-----------------------------------------------------------------------------
static void mi_lcd_backlight(void * data)
{
	(void)data;
	palTogglePad(GPIOD, GPIOD_LCD_LED_DRIVE);
}

//-----------------------------------------------------------------------------
static void mi_standby(void * data)
{
	(void)data;
	kuroBox_request_standby = 1;
}

//-----------------------------------------------------------------------------
menu_item_t menu_items [MENU_ITEMS_COUNT] =
{
		{ "Serial 1 Pwr", 1, 	mi_serial1_pwr, 	NULL },
		{ "Serial 2 Pwr", 1, 	mi_serial2_pwr, 	NULL },
		{ "LED 3", 1, 			mi_led3, 			NULL },
		{ "LCD Backlight", 1, 	mi_lcd_backlight, 	NULL },
		{ "Power OFF", 1, 		mi_standby, 		NULL },
		{ "Exit Menu", 1, 		mi_exit, 			NULL },
};

//-----------------------------------------------------------------------------
// display state stuff
static char charbuf[128];
static MemoryStream msb;

//-----------------------------------------------------------------------------
int kbm_display(void)
{

	if ( current_item == -1 )
		return 0;

	#define INIT_CBUF() \
		memset(charbuf,0,sizeof(charbuf));\
		msObjectInit(&msb, (uint8_t*)charbuf, 128, 0);
	BaseSequentialStream * bss = (BaseSequentialStream *)&msb;

	st7565_clear(&ST7565D1);

	INIT_CBUF();
	// +1 because non-programmers aren't used to 0-based counts... :p
	chprintf(bss,"kuroBox menu: %d/%d", current_item+1, MENU_ITEMS_COUNT);
	st7565_drawstring(&ST7565D1, 0, 0, charbuf);

	INIT_CBUF();
	chprintf(bss,"> %s", menu_items[current_item].title);
	st7565_drawstring(&ST7565D1, 0, 1, charbuf);

	st7565_display(&ST7565D1);

	#undef INIT_CBUF
	return 1;
}

//-----------------------------------------------------------------------------
int kbm_btn0(bool_t pressed)
{
	if ( !pressed )
		return 0;

	if ( current_item == -1 )
	{
		current_item = 0;
	}
	else
	{
		current_item++;
		if ( current_item == MENU_ITEMS_COUNT )
			current_item = 0;
	}

	return 0;
}

//-----------------------------------------------------------------------------
int kbm_btn1(bool_t pressed)
{
	if ( !pressed )
		return 0;

	if ( current_item == -1 )
		return 0;

	if ( menu_items[current_item].callback )
		menu_items[current_item].callback(menu_items[current_item].data);

	return 0;
}

//-----------------------------------------------------------------------------
int kuroBoxMenuInit(void)
{
	return KB_OK;
}

//-----------------------------------------------------------------------------
int kuroBoxMenuStop(void)
{
	return KB_OK;
}
