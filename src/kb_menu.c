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
#include "kb_menu_items.h"
#include "kb_config.h"
#include "ST7565.h"
#include <memstreams.h>
#include <chprintf.h>
#include <string.h>

//-----------------------------------------------------------------------------
#define REFRESH_SLEEP 				50
#define MENU_ITEMS_COUNT 			11

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
typedef struct menu_item_t menu_item_t;
struct menu_item_t
{
	// what to display
	char title[32]; 				

	// should we skip it or display it?
	bool_t active;					

	// is it a config item, eg, exit & power off aren't
	bool_t is_config;				

	// what to call when activated
	menucallback_cb callback;		

	// if we can display extra data specific to this menu item, this will return
	// it. Note, for the moment, only INT's are supported
	menuextra_data_cb extra_data;	

	// any extra data to pass onto the callback
	void * data;
};

//-----------------------------------------------------------------------------
int16_t current_menu_item = NO_MENU_ITEM;

//-----------------------------------------------------------------------------
menu_item_t menu_items [MENU_ITEMS_COUNT] =
{
	// menu item callback functions are in kb_menu_items.c
		{ "Feature", 			1,		1, 		mi_featureA_change, mi_featureA_getFeature, NULL },
		{ "Serial1 Pwr", 		1,		1, 		mi_serial1_pwr, 	NULL, NULL },
		{ "Serial1 Baud", 		1,		1, 		mi_serial1_baud, 	mi_serial1_getbaud, NULL },
		{ "Serial2 Pwr", 		1,		1, 		mi_serial2_pwr, 	NULL, NULL },
		{ "Serial2 Baud", 		1,		1, 		mi_serial2_baud, 	mi_serial2_getbaud, NULL },
		{ "LED 3", 				1,		1, 		mi_led3, 			NULL, NULL },
		{ "LCD Backlight", 		1,		1, 		mi_lcd_backlight, 	NULL, NULL },
		{ "Set time from LTC", 	1,		1, 		mi_time_from_ltc, 	NULL, NULL },
		{ "Set time from GPS", 	1,		1, 		mi_time_from_gps, 	NULL, NULL },
		{ "Power OFF", 			1,		0, 		mi_standby, 		NULL, NULL },
		{ "Exit Menu", 			1,		0, 		mi_exit, 			NULL, NULL },
};

//-----------------------------------------------------------------------------
// display state stuff
static char charbuf[128];
static MemoryStream msb;

//-----------------------------------------------------------------------------
// This function gets called by the screen's main thread. If it returns 0, then
// nothing got drawn and the screen should do its thing.
int kbm_display(void)
{
	// Messages are handled in mailboxes, so as to free up the ISR. In the ISR
	// a message is mailed, and then returns. We buffer up to 10 
	// keypresses/releases, which, I would hope, is more than enough
	while(chMBGetUsedCountI(&btn_mailbox) > 0)
	{
		msg_t btn_state = 0;
		msg_t mbox_stat = chMBFetch(&btn_mailbox, &btn_state, TIME_IMMEDIATE);
		if ( mbox_stat == RDY_OK )
		{
			// in the message of the mailbox, which is just an int, we encode
			// which button it is, and whether it's a press or release
			switch(BTN_DECODE_BUTTON(btn_state))
			{
			case 0: kbm_btn0_dispatch(BTN_DECODE_PRESSED(btn_state)); break;
			case 1: kbm_btn1_dispatch(BTN_DECODE_PRESSED(btn_state)); break;
			}
		}
	}

	// if, after processing all key events, we're back at ZERO, then exit
	if ( current_menu_item == NO_MENU_ITEM )
		return 0;

	#define INIT_CBUF() \
		memset(charbuf,0,sizeof(charbuf));\
		msObjectInit(&msb, (uint8_t*)charbuf, 128, 0);
	BaseSequentialStream * bss = (BaseSequentialStream *)&msb;

	st7565_clear(&ST7565D1);

	INIT_CBUF();
	// +1 because non-programmers aren't used to 0-based counts... :p
	chprintf(bss,"kuroBox menu: %d/%d", current_menu_item+1, MENU_ITEMS_COUNT);
	st7565_drawstring(&ST7565D1, 0, 0, charbuf);

	INIT_CBUF();
	chprintf(bss,"> %s", menu_items[current_menu_item].title);
	st7565_drawstring(&ST7565D1, 0, 1, charbuf);

	if ( menu_items[current_menu_item].extra_data )
	{
		INIT_CBUF();
		chprintf(bss,"> %d", menu_items[current_menu_item].extra_data());
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
// Button 0 is the "choose" button, just goes through all the items, 
// in sequence
static void
kbm_btn0_dispatch(bool_t pressed)
{
	if ( !pressed )
		return;

	if ( current_menu_item == -1 )
	{
		current_menu_item = 0;
	}
	else
	{
		current_menu_item++;
		if ( current_menu_item == MENU_ITEMS_COUNT )
			current_menu_item = -1;
	}
}

//-----------------------------------------------------------------------------
// Button 1 is the "action" button, whatever is the current item, activate it,
// and if it's a config item, just make sure that the current config is saved
static void
kbm_btn1_dispatch(bool_t pressed)
{
	if ( !pressed )
		return;

	if ( current_menu_item == -1 )
		return;

	if ( menu_items[current_menu_item].callback )
	{
		menu_items[current_menu_item].callback(menu_items[current_menu_item].data);
		if (menu_items[current_menu_item].is_config)
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
