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
#include "kb_serial.h"
#include "kb_config.h"
#include "kb_gpio.h"
#include "ST7565.h"
#include <memstreams.h>
#include <chprintf.h>
#include <string.h>

//-----------------------------------------------------------------------------
#define REFRESH_SLEEP 				50
#define MENU_ITEMS_COUNT 			7
bool_t kuroBox_request_standby;

//-----------------------------------------------------------------------------
#define BTN_NUM_MSGS				10
static Semaphore btn_semaphore;
static msg_t btn_msgs[BTN_NUM_MSGS];
static Mailbox btn_mailbox;

#define BTN_ENCODE(b,p)				((b<<1)|p)
#define BTN_DECODE_BUTTON(s)		((s>>1)&0x01)
#define BTN_DECODE_PRESSED(s)		(s&0x01)

static void kbm_btn0_dispatch(bool_t pressed);
static void kbm_btn1_dispatch(bool_t pressed);

//-----------------------------------------------------------------------------
typedef void (*menucallback_cb)(void * data);
typedef int (*menuextra_data_cb)(void);
typedef struct menu_item_t menu_item_t;
struct menu_item_t
{
	char title[32];
	bool_t active;
	bool_t is_config;
	menucallback_cb callback;
	menuextra_data_cb extra_data;
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
	kbse_setPowerSerial1(!kbse_getPowerSerial1());
}

//-----------------------------------------------------------------------------
static void mi_serial1_baud(void * data)
{
	(void)data;
	kbse_changeSerial1Baud();
}

//-----------------------------------------------------------------------------
static int mi_serial1_getbaud(void)
{
	return kbse_getSerial1Baud();
}

//-----------------------------------------------------------------------------
static void mi_serial2_pwr(void * data)
{
	(void)data;
	kbse_setPowerSerial2(!kbse_getPowerSerial2());
}

//-----------------------------------------------------------------------------
static void mi_led3(void * data)
{
	(void)data;
	kbg_setLED3(kbg_getLED3());
}

//-----------------------------------------------------------------------------
static void mi_lcd_backlight(void * data)
{
	(void)data;
	kbg_setLCDBacklight(!kbg_getLCDBacklight());
}

//-----------------------------------------------------------------------------
static void mi_standby(void * data)
{
	(void)data;
//	kuroBox_request_standby = 1;
}

//-----------------------------------------------------------------------------
menu_item_t menu_items [MENU_ITEMS_COUNT] =
{
		{ "Serial1 Pwr", 		1,		1, 		mi_serial1_pwr, 	NULL, NULL },
		{ "Serial1 Baud", 		1,		1, 		mi_serial1_baud, 	mi_serial1_getbaud, NULL },
		{ "Serial2 Pwr", 		1,		1, 		mi_serial2_pwr, 	NULL, NULL },
		{ "LED 3", 				1,		1, 		mi_led3, 			NULL, NULL },
		{ "LCD Backlight", 		1,		1, 		mi_lcd_backlight, 	NULL, NULL },
		{ "Power OFF", 			1,		0, 		mi_standby, 		NULL, NULL },
		{ "Exit Menu", 			1,		0, 		mi_exit, 			NULL, NULL },
};

//-----------------------------------------------------------------------------
// display state stuff
static char charbuf[128];
static MemoryStream msb;

//-----------------------------------------------------------------------------
int kbm_display(void)
{
	while(chMBGetUsedCountI(&btn_mailbox)>0)
	{
		msg_t btn_state = 0;
		msg_t mbox_stat = chMBFetch(&btn_mailbox, &btn_state, TIME_IMMEDIATE);
		if ( mbox_stat == RDY_OK )
		{
			switch(BTN_DECODE_BUTTON(btn_state))
			{
			case 0: kbm_btn0_dispatch(BTN_DECODE_PRESSED(btn_state)); break;
			case 1: kbm_btn1_dispatch(BTN_DECODE_PRESSED(btn_state)); break;
			}
		}
	}

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

	if ( menu_items[current_item].extra_data )
	{
		INIT_CBUF();
		chprintf(bss,"> %d", menu_items[current_item].extra_data());
		st7565_drawstring(&ST7565D1, 0, 2, charbuf);
	}

	st7565_display(&ST7565D1);

	#undef INIT_CBUF
	return 1;
}

//-----------------------------------------------------------------------------
int kbm_btn0(bool_t pressed)
{
	chMBPostI(&btn_mailbox, BTN_ENCODE(0,pressed));
	return 0;
}

//-----------------------------------------------------------------------------
int kbm_btn1(bool_t pressed)
{
	chMBPostI(&btn_mailbox, BTN_ENCODE(1,pressed));
	return 0;
}

//-----------------------------------------------------------------------------
static void
kbm_btn0_dispatch(bool_t pressed)
{
	if ( !pressed )
		return;

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
}

//-----------------------------------------------------------------------------
static void
kbm_btn1_dispatch(bool_t pressed)
{
	if ( !pressed )
		return;

	if ( current_item == -1 )
		return;

	if ( menu_items[current_item].callback )
	{
		menu_items[current_item].callback(menu_items[current_item].data);
		if (menu_items[current_item].is_config)
			kbc_save();
	}
}

//-----------------------------------------------------------------------------
int kuroBoxMenuInit(void)
{
	chSemInit(&btn_semaphore, 1);
	chMBInit(&btn_mailbox, btn_msgs, BTN_NUM_MSGS);
	return KB_OK;
}

//-----------------------------------------------------------------------------
int kuroBoxMenuStop(void)
{
	return KB_OK;
}
