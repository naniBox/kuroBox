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

//-----------------------------------------------------------------------------
void btn_0_exti_cb(EXTDriver *extp, expchannel_t channel)
{
	(void)extp;
	(void)channel;
	palTogglePad(GPIOB, GPIOB_LED2);
}

//-----------------------------------------------------------------------------
void btn_1_exti_cb(EXTDriver *extp, expchannel_t channel)
{
	(void)extp;
	(void)channel;
	palTogglePad(GPIOA, GPIOA_LED3);
}
