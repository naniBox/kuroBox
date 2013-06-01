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

#ifndef _naniBox_kuroBox_screen
#define _naniBox_kuroBox_screen

#include <ch.h>
#include <ltc.h>

//-----------------------------------------------------------------------------
int kuroBoxScreenInit(void);

void kbs_setVoltage(uint16_t volts);	// in volts*10
void kbs_setTemperature(int16_t temperature);	// in C
void kbs_setLTC(SMPTETimecode * ltc);
void kbs_setCounter(uint32_t count);
void kbs_setSDCFree(int32_t sdc_free);	// if negative, then write protected
void kbs_setFName(const char * fname);
void kbs_setBtn0(uint8_t on);
void kbs_setBtn1(uint8_t on);

void kbs_setLTCS(uint32_t a, uint32_t b, uint32_t c, uint32_t d);

#endif // _naniBox_kuroBox_screen
