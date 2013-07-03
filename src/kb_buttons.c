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

#include "kb_buttons.h"
#include "kb_screen.h"
#include "kb_menu.h"

#define BUTTON_DOWN 	0
#define BUTTON_UP		1

static bool_t btn_0_pressed;
static bool_t btn_1_pressed;

//-----------------------------------------------------------------------------
static void on_btn0(bool_t pressed)
{
	kbs_btn0(pressed);
	kbm_btn0(pressed);
}

//-----------------------------------------------------------------------------
static void on_btn1(bool_t pressed)
{
	kbs_btn1(pressed);
	kbm_btn1(pressed);
}

//-----------------------------------------------------------------------------
int kuroBoxButtonsInit(void)
{
	btn_0_pressed=palReadPad(GPIOA, GPIOA_BTN0) == BUTTON_DOWN;
	btn_1_pressed=palReadPad(GPIOA, GPIOA_BTN1) == BUTTON_DOWN;
	on_btn0(btn_0_pressed);
	on_btn1(btn_1_pressed);
	return KB_OK;
}

//-----------------------------------------------------------------------------
void btn_0_exti_cb(EXTDriver *extp, expchannel_t channel)
{
	(void)extp;
	(void)channel;
	btn_0_pressed=palReadPad(GPIOA, GPIOA_BTN0) == BUTTON_DOWN;
	on_btn0(btn_0_pressed);
}

//-----------------------------------------------------------------------------
void btn_1_exti_cb(EXTDriver *extp, expchannel_t channel)
{
	(void)extp;
	(void)channel;
	btn_1_pressed=palReadPad(GPIOA, GPIOA_BTN1) == BUTTON_DOWN;
	on_btn1(btn_1_pressed);
}

//-----------------------------------------------------------------------------
bool_t is_btn_0_pressed(void)
{
	return btn_0_pressed == 0;
}

//-----------------------------------------------------------------------------
bool_t is_btn_1_pressed(void)
{
	return btn_1_pressed == 0;
}
