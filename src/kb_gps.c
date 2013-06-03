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
#include "kb_gps.h"
#include "kb_screen.h"
#include "kb_util.h"
#include "kb_logger.h"
#include <string.h>

//-----------------------------------------------------------------------------
#define UBX_HEADER								0x62b5 //  little endian
#define UBX_NAV_SOL_ID 							0x0601 //  little endian
uint8_t ubx_nav_sol_buffer[UBX_NAV_SOL_SIZE];
uint8_t ubx_nav_sol_idx;

//-----------------------------------------------------------------------------
static SerialConfig gps_cfg = {
	9600,
	0, 						// cr1;
	USART_CR2_STOP1_BITS,	// cr2;
	0 						// cr3;
};

//-----------------------------------------------------------------------------
void parse_and_store_nav_sol(void)
{
	struct ubx_nav_sol_t * nav_sol = (struct ubx_nav_sol_t*) ubx_nav_sol_buffer;

	if ( nav_sol->header == UBX_HEADER )
	{
		if ( nav_sol->id == UBX_NAV_SOL_ID )
		{
			uint16_t cs = calc_checksum_16( ubx_nav_sol_buffer+2, UBX_NAV_SOL_SIZE-4 );
			if ( nav_sol->cs == cs )
			{
				// valid packet!
				chSysLock();
					kbl_setGpsNavSol(nav_sol);
				chSysUnlock();
			}
		}
	}
	memset(ubx_nav_sol_buffer, 0, sizeof(ubx_nav_sol_buffer));
	ubx_nav_sol_idx = 0;
}

//-----------------------------------------------------------------------------
/*static Thread * gpsThread;*/
//-----------------------------------------------------------------------------
static WORKING_AREA(waGps, 128);
static msg_t
thGps(void *arg)
{
	(void)arg;
	chRegSetThreadName("Gps");

	memset(ubx_nav_sol_buffer, 0, sizeof(ubx_nav_sol_buffer));
	while( !chThdShouldTerminate() )
	{
		uint8_t c = 0;
		if ( sdRead(&SD6, &c, 1) != 1 )
			continue;

		if ( ( ubx_nav_sol_idx == 0 && c == (UBX_HEADER&0xff) ) ||
			 ( ubx_nav_sol_idx == 1 && c == ((UBX_HEADER>>8)&0xff) ) ||
			 ( ( ubx_nav_sol_idx > 1 ) &&
			   ( ubx_nav_sol_idx < UBX_NAV_SOL_SIZE ) ) )
			ubx_nav_sol_buffer[ubx_nav_sol_idx++] = c;
		else
			ubx_nav_sol_idx = 0;

		if ( ubx_nav_sol_idx == UBX_NAV_SOL_SIZE )
			parse_and_store_nav_sol();
	}
	return 0;
}


//-----------------------------------------------------------------------------
void gps_timepulse_exti_cb(EXTDriver *extp, expchannel_t channel)
{
	(void)extp;
	(void)channel;

	chSysLockFromIsr();
		kbl_incPPS();
	chSysUnlockFromIsr();
}

//-----------------------------------------------------------------------------
int kuroBoxGPSInit(void)
{
	sdStart(&SD6, &gps_cfg);
	/*gpsThread = */chThdCreateStatic(waGps, sizeof(waGps), NORMALPRIO, thGps, NULL);
	return KB_OK;
}

