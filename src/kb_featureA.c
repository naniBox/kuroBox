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
#include "kb_featureA.h"
#include "kb_time.h"
#include "kb_serial.h"
#include "kb_vectornav.h"
#include <chprintf.h>

//-----------------------------------------------------------------------------
#define FEATURE_A_REFRESH_SLEEP		10
static int current_feature;

//-----------------------------------------------------------------------------
#define LTC_PREFIX "\x79\x00\x77\x10"
#define YPR_PREFIX "\x79\x00\x77\x00"
#define BLANK_PREFIX "\x76\x77\x04"

typedef struct ambient_status_t ambient_status_t;
struct __PACKED__ ambient_status_t
{
	/*
	 * 2 byte sequence to identify start of packet: 0x6E, 0x42
	 * */
	uint16_t preamble;

	/*
	 * Instantaneous YAW at time when the LTC SYNC_WORD comes in.
	 * This is a ieee 754 32-bit floating number
	 */
	float yaw;

	/*
	 * Instantaneous PITCH at time when the LTC SYNC_WORD comes in.
	 * This is a ieee 754 32-bit floating number
	 */
	float pitch;

	/*
	 * Instantaneous ROLL at time when the LTC SYNC_WORD comes in.
	 * This is a ieee 754 32-bit floating number
	 */
	float roll;

	/*
	 * Packet Id sent since powerup.
	 */
	uint32_t packet_id;

	/*
	 * 16-bit checksum calculated based on entire packet except the checksum
	 * field, that is, the first 18bytes of the packet. It's calculated based
	 * on the fletcher algorithm (from uBlox datasheet):
	 * 		http://www.ietf.org/rfc/rfc1145.txt
	 *		http://en.wikipedia.org/wiki/Fletcher's_checksum
	 * Pseudocode:

		CK_A = 0, CK_B = 0
		For(I=0;I<N;I++)
		{
			CK_A = CK_A + Buffer[I]
			CK_B = CK_B + CK_A
		}
		RES = ( CK_B << 8 ) | CK_A

	 */
	uint16_t checksum;
};
STATIC_ASSERT(sizeof(ambient_status_t)==20, AMBIENT_STATUS_T_NOT_20_BYTES);


//-----------------------------------------------------------------------------
static Thread * featureAThread;
static WORKING_AREA(waFeatureA, 256);
static msg_t
thFeatureA(void *arg)
{
	(void)arg;
	chRegSetThreadName("FeatureA");

	while( !chThdShouldTerminate() )
	{
		switch( current_feature )
		{
		case 0:
			{
				const struct smpte_timecode_t * ltc = kbt_getLTC();
				sdWrite(&Serial1, (uint8_t*)LTC_PREFIX, 4);
				chprintf(((BaseSequentialStream *)&Serial1), "%.2d%.2d", ltc->seconds, ltc->frames);
			}
			break;
		case 1:
			{
				const struct smpte_timecode_t * ltc = kbt_getLTC();
				sdWrite(&Serial1, (uint8_t*)LTC_PREFIX, 4);
				if ( ltc->seconds&0x01 )
					chprintf(((BaseSequentialStream *)&Serial1), "%.2d%.2d", ltc->seconds, ltc->frames);
				else
					chprintf(((BaseSequentialStream *)&Serial1), "%.2d%.2d", ltc->hours, ltc->minutes);
			}
			break;
		case 2:
			{
				const vnav_data_t * ypr = kbv_getYPR();
				sdWrite(&Serial1, (uint8_t*)YPR_PREFIX, 4);
				chprintf(((BaseSequentialStream *)&Serial1), "%.4d", (int)ypr->ypr[0]);
			} break;
		case 3:
			{
				const vnav_data_t * ypr = kbv_getYPR();
				sdWrite(&Serial1, (uint8_t*)YPR_PREFIX, 4);
				chprintf(((BaseSequentialStream *)&Serial1), "%.4d", (int)ypr->ypr[1]);
			} break;
		case 4:
			{
				const vnav_data_t * ypr = kbv_getYPR();
				sdWrite(&Serial1, (uint8_t*)YPR_PREFIX, 4);
				chprintf(((BaseSequentialStream *)&Serial1),  "%.4d", (int)ypr->ypr[2]);
			} break;
		case 5:
		default:
			{
				sdWrite(&Serial1, (uint8_t*)BLANK_PREFIX, 3);
			} break;
		}
		chThdSleepMilliseconds(FEATURE_A_REFRESH_SLEEP);
	}
	return 0;
}

//-----------------------------------------------------------------------------
int 
kbfa_getFeature()
{
	return current_feature;
}

//-----------------------------------------------------------------------------
void 
kbfa_setFeature(int feature)
{
	if ( feature >= 0 && feature <= 5 )
		current_feature = feature;
}

//-----------------------------------------------------------------------------
int 
kbfa_changeFeature()
{
	switch( current_feature )
	{
	case 0: current_feature = 1; break;
	case 1: current_feature = 2; break;
	case 2: current_feature = 3; break;
	case 3: current_feature = 4; break;
	case 4: current_feature = 5; break;
	case 5: current_feature = 0; break;
	default: current_feature = 0; break;
	}

	return 0;
}

//-----------------------------------------------------------------------------
int 
kuroBoxFeatureAInit(void)
{
	featureAThread = chThdCreateStatic(waFeatureA, sizeof(waFeatureA), NORMALPRIO, thFeatureA, NULL);
	return KB_OK;
}

//-----------------------------------------------------------------------------
int 
kuroBoxFeatureAStop(void)
{
	chThdTerminate(featureAThread);
	chThdWait(featureAThread);
	return KB_OK;
}
