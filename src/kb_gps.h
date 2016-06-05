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
#ifndef _naniBox_kuroBox_gps
#define _naniBox_kuroBox_gps

//-----------------------------------------------------------------------------
#include <hal.h>
#include "kb_util.h"

//-----------------------------------------------------------------------------
#include "kbb_types.h"
KBB_TYPES_VERSION_CHECK(0x0002)

//-----------------------------------------------------------------------------
int kuroBoxGPSInit(SerialDriver * sd);
int kuroBoxGPSStop(void);

//-----------------------------------------------------------------------------
void kbg_timepulseExtiCB(EXTDriver *extp, expchannel_t channel);
void kbg_ecef2lla(int32_t x, int32_t y, int32_t z, float * lat, float * lon, float * alt);
const ubx_nav_sol_t * kbg_getUbxNavSol(void);

#endif // _naniBox_kuroBox_gps
