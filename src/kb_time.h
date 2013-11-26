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
#ifndef _naniBox_kuroBox_time
#define _naniBox_kuroBox_time

//-----------------------------------------------------------------------------
#include <ch.h>
#include <hal.h>
#include "kb_util.h"
#include "kbb_types.h"

//-----------------------------------------------------------------------------
const smpte_timecode_t * kbt_getLTC(void);

//-----------------------------------------------------------------------------
int kuroBoxTimeInit(void);
int kuroBoxTimeStop(void);

//-----------------------------------------------------------------------------
void kbt_pulseExtiCB(EXTDriver *extp, expchannel_t channel);

//-----------------------------------------------------------------------------
void kbt_startOneSec(int32_t drift_factor);
void kbt_getRTC(rtc_t * rtc);

#endif // _naniBox_kuroBox_time
