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
#include "kb_writer.h"
#include <string.h>
#include <math.h>

//-----------------------------------------------------------------------------
#define UBX_HEADER				0x62b5 //  little endian, so byte swapped
#define UBX_NAV_SOL_ID 			0x0601 //  little endian
uint8_t ubx_nav_sol_buffer[UBX_NAV_SOL_SIZE];
uint8_t ubx_nav_sol_idx;
ubx_nav_sol_t ubx_nav_sol_valid;

//-----------------------------------------------------------------------------
static SerialConfig gps_cfg = {
	9600,
	0, 						// cr1;
	USART_CR2_STOP1_BITS,	// cr2;
	0 						// cr3;
};

//-----------------------------------------------------------------------------
// Implemented from
// http://www.nalresearch.com/files/Standard%20Modems/A3LA-XG/A3LA-XG%20SW%20Version%201.0.0/GPS%20Technical%20Documents/GPS.G1-X-00006%20(Datum%20Transformations).pdf
// input is in SI (meters)
#define A		6378137.000f
#define B		6356752.31424518f
#define EE		0.0066943799901411239f
#define E1E1	0.0067394967422762389f
void
kbg_ecef2lla(int32_t xx, int32_t yy, int32_t zz, float * olat, float * olon, float * oalt)
{
	float x = xx/100.0f;
	float y = yy/100.0f;
	float z = zz/100.0f;

	// const float F = 1.0/298.257223563f;
	// const float A = 6378137.000f;
	// const float B = 6356752.31424518f;			// A*(1.0-F)

	// const float E = 0.081819190842620321f;	// sqrt((A*A-B*B)/(A*A));
	// const float EE = 0.0066943799901411239f;	// E*E
	// const float E1 = 0.082094437949694496f;	// sqrt((A*A-B*B)/(B*B));
	// const float E1E1 = 0.0067394967422762389f;	// E1*E1

	float lambda = atan2f(y, x);
	float p = sqrtf(x*x + y*y);
	float theta = atan2f((z*A), (p*B));
	float phi = atan2f(z + E1E1*B*powf(sinf(theta), 3), p - EE*A*powf(cosf(theta), 3));
	float N = A / sqrtf(1.0f - EE*powf(sinf(phi), 2));
	float h = p / cosf(phi) - N;

	*olat = phi*180.0f/(float)M_PI;
	*olon = lambda*180.0f/(float)M_PI;
	*oalt = h;
}

/*
//-----------------------------------------------------------------------------
void
kbg_ecef2lla(int32_t x, int32_t y, int32_t z, float * lat, float * lon, float * alt)
{
    float a = 6378137.0f;
    float e = 8.1819190842622e-2f;
    float b = 6356752.3142451793f; // sqrtf(powf(a,2) * (1-powf(e,2)));
    float ep = 0.082094437949695676f; // sqrtf((powf(a,2)-powf(b,2))/powf(b,2));
    float p = sqrt(pow(x,2)+pow(y,2));
    float th = atan2(a*z, b*p);
    *lon = atan2(y, x);
    *lat = atan2((z+ep*ep*b*pow(sin(th),3)), (p-e*e*a*pow(cos(th),3)));
    float n = a/sqrt(1-e*e*pow(sin(*lat),2));
    *alt = p/cos(*lat)-n;
    *lat = (*lat*180.0f)/M_PI;
    *lon = (*lon*180.0f)/M_PI;
}
*/

//-----------------------------------------------------------------------------
static void
parse_and_store_nav_sol(void)
{
	ubx_nav_sol_t * nav_sol = (ubx_nav_sol_t*) ubx_nav_sol_buffer;

	if ( nav_sol->header == UBX_HEADER )
	{
		if ( nav_sol->id == UBX_NAV_SOL_ID )
		{
			uint16_t cs = calc_checksum_16( ubx_nav_sol_buffer+2, UBX_NAV_SOL_SIZE-4 );
			if ( nav_sol->cs == cs )
			{
				// everything checks out, valid packet!
				chSysLock();
					memcpy(&ubx_nav_sol_valid, nav_sol, sizeof(ubx_nav_sol_valid));
					kbs_setGpsEcef(nav_sol->ecefX, nav_sol->ecefY, nav_sol->ecefZ);
					kbw_setGpsNavSol(nav_sol);
				chSysUnlock();
			}
		}
	}
	memset(ubx_nav_sol_buffer, 0, sizeof(ubx_nav_sol_buffer));
	ubx_nav_sol_idx = 0;
}

//-----------------------------------------------------------------------------
static thread_t * gpsThread;
//-----------------------------------------------------------------------------
static THD_WORKING_AREA(waGps, 128);
static THD_FUNCTION(thGps, arg)
{
	SerialDriver * sd = (SerialDriver *)arg;
	chRegSetThreadName("Gps");

	memset(ubx_nav_sol_buffer, 0, sizeof(ubx_nav_sol_buffer));
	while( !chThdShouldTerminate() )
	{
		uint8_t c = 0;
		// read one byte at a time, this function blocks until there's
		// something to read
		if ( sdRead(sd, &c, 1) != 1 )
			// but just in case, if nothing was read, try again
			continue;

		/*
			This works this way:
			- if at idx == 0 and the first byte is the same as the first byte of the header
			 -or-
			- if at idx == 1 and the second byte is the same as the second byte of the header
			 -or-
			- if at idx >= 2 and idx < size of packet

			THEN:
				store it, advance idx
			If not, reset idx and start from the begining.
			

			@TODO: handle different messages
		 */
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

	return;
}

//-----------------------------------------------------------------------------
void
kbg_timepulseExtiCB(EXTDriver *extp, expchannel_t channel)
{
	(void)extp;
	(void)channel;

	chSysLockFromIsr();
		// @TODO: inform VectorNav that this happened
		kbw_incPPS();
		kbs_PPS();
	chSysUnlockFromIsr();
}

//-----------------------------------------------------------------------------
int 
kuroBoxGPSInit(SerialDriver * sd)
{
	ASSERT(sd != NULL, "kuroBoxGPSInit 1", "SerialDriver is NULL");
	sdStart(sd, &gps_cfg);
	gpsThread = chThdCreateStatic(waGps, sizeof(waGps), NORMALPRIO, thGps, &SD6);
	return KB_OK;
}

//-----------------------------------------------------------------------------
int 
kuroBoxGPSStop(void)
{
	chThdTerminate(gpsThread);
	chThdWait(gpsThread);

	gpsThread = NULL;

	return KB_OK;
}

//-----------------------------------------------------------------------------
const ubx_nav_sol_t * kbg_getUbxNavSol(void)
{
	return &ubx_nav_sol_valid;
}
