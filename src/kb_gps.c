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
#include "nanibox_util.h"
#include <string.h>

//-----------------------------------------------------------------------------
#define UBX_NAV_SOL_SIZE						60
#define UBX_NAV_SOL_COUNT_PER_PULSE				5
uint8_t gps_ubx_nav_sol[UBX_NAV_SOL_SIZE];
uint8_t gps_ubx_nav_sol_count;


//-----------------------------------------------------------------------------
static void gps_rx_end_cb(UARTDriver * uartp)
{
	(void)uartp;
	palTogglePad(GPIOB, GPIOB_LED2);

	const uint16_t UBX_HEADER_PREAMBLE = 0x62b5;
	uint16_t * ubx_header = (uint16_t *)&gps_ubx_nav_sol[0];
	if( *ubx_header == UBX_HEADER_PREAMBLE )
		kbs_gpsCount(1);
	else
		kbs_gpsCount(-1);

	gps_ubx_nav_sol_count--;
	if ( gps_ubx_nav_sol_count )
	{
		chSysLockFromIsr();
		uartStopReceiveI(&UARTD6);
		uartStartReceiveI(&UARTD6, UBX_NAV_SOL_SIZE, gps_ubx_nav_sol);
		chSysUnlockFromIsr();
	}
}

//-----------------------------------------------------------------------------
static void gps_rx_char_cb(UARTDriver *uartp, uint16_t c)
{
	(void)uartp;
	(void)c;
}

//-----------------------------------------------------------------------------
static const UARTConfig gps_cfg = {
	NULL, 				// txend1_cb;
	NULL, 				// txend2_cb;
	gps_rx_end_cb, 		// rxend_cb;
	gps_rx_char_cb,		// rxchar_cb;
	NULL, 				// rxerr_cb;
	9600, 				// speed;
	0, 					// cr1;
	0, 					// cr2;
	0 					// cr3;
};

//-----------------------------------------------------------------------------
/*static Thread * gpsThread;*/
//-----------------------------------------------------------------------------
static WORKING_AREA(waGps, 128);
static msg_t
thGps(void *arg)
{
	(void)arg;
	chRegSetThreadName("Gps");
	while( !chThdShouldTerminate() )
	{
		palTogglePad(GPIOA, GPIOA_LED3);
		chThdSleepMilliseconds(1000);
	}
	return 0;
}


//-----------------------------------------------------------------------------
void gps_timepulse_exti_cb(EXTDriver *extp, expchannel_t channel)
{
	(void)extp;
	(void)channel;

	gps_ubx_nav_sol_count = UBX_NAV_SOL_COUNT_PER_PULSE;
	chSysLockFromIsr();
	uartStopReceiveI(&UARTD6);
	uartStartReceiveI(&UARTD6, UBX_NAV_SOL_SIZE, gps_ubx_nav_sol);
	chSysUnlockFromIsr();

	/*
	if (palReadPad(GPIOC, GPIOC_L1_TIMEPULSE))
		palSetPad(GPIOB, GPIOB_LED2);
	else
		palClearPad(GPIOB, GPIOB_LED2);
	*/
}

//-----------------------------------------------------------------------------
int kuroBoxGPSInit(void)
{
	memset(gps_ubx_nav_sol, 0, sizeof(gps_ubx_nav_sol));

	uartStart(&UARTD6, &gps_cfg);
	/*gpsThread = */chThdCreateStatic(waGps, sizeof(waGps), NORMALPRIO, thGps, NULL);
	return KB_OK;
}

