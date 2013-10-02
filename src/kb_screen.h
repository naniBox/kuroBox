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
#ifndef _naniBox_kuroBox_screen
#define _naniBox_kuroBox_screen

//-----------------------------------------------------------------------------
#include <ch.h>
#include "kb_time.h"

//-----------------------------------------------------------------------------
int kuroBoxScreenInit(void);
int kuroBoxScreenStop(void);

//-----------------------------------------------------------------------------
void kbs_setVoltage(uint16_t volts);	// in volts*10
void kbs_setTemperature(int16_t temperature);	// in C
void kbs_setLTC(smpte_timecode_t * ltc);

void kbs_setWriteCount(uint32_t write_count);
void kbs_setWriteErrors(uint32_t write_errors);
void kbs_setSDCFree(int32_t sdc_free);	// if negative, then write protected
void kbs_setFName(const char * fname);

void kbs_btn0(uint8_t on);
void kbs_btn1(uint8_t on);

void kbs_PPS(void);
void kbs_setGpsEcef(int32_t x, int32_t y, int32_t z);

void kbs_setYPR(float yaw, float pitch, float roll);

//-----------------------------------------------------------------------------
#endif // _naniBox_kuroBox_screen
