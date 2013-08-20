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

#ifndef _naniBox_kuroBox_logger
#define _naniBox_kuroBox_logger

#include <ch.h>
#include "kb_time.h"
#include "kb_gps.h"
#include "kb_vectornav.h"


//-----------------------------------------------------------------------------
int kuroBoxWriterInit(void);
int kuroBoxWriterStop(void);

//-----------------------------------------------------------------------------
void kbw_setLTC(ltc_frame_t * ltc_frame);
void kbw_incPPS(void);
void kbw_setGpsNavSol(ubx_nav_sol_t * nav_sol);
void kbw_setVNav(vnav_data_t * vnav);

void kbw_header_vnav(uint8_t * data);

#endif // _naniBox_kuroBox_logger
