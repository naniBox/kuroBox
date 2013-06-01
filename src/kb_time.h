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

#ifndef _naniBox_kuroBox_time
#define _naniBox_kuroBox_time

#include <ch.h>
#include <hal.h>
#include "nanibox_util.h"

//-----------------------------------------------------------------------------
struct __PACKED__ LTCFrame
{
	unsigned int frame_units:4; ///< SMPTE framenumber BCD unit 0..9
	unsigned int user1:4;

	unsigned int frame_tens:2; ///< SMPTE framenumber BCD tens 0..3
	unsigned int dfbit:1; ///< indicated drop-frame timecode
	unsigned int col_frame:1; ///< colour-frame: timecode intentionally synchronized to a colour TV field sequence
	unsigned int user2:4;

	unsigned int secs_units:4; ///< SMPTE seconds BCD unit 0..9
	unsigned int user3:4;

	unsigned int secs_tens:3; ///< SMPTE seconds BCD tens 0..6
	unsigned int biphase_mark_phase_correction:1; ///< see note on Bit 27 in description and \ref ltc_frame_set_parity .
	unsigned int user4:4;

	unsigned int mins_units:4; ///< SMPTE minutes BCD unit 0..9
	unsigned int user5:4;

	unsigned int mins_tens:3; ///< SMPTE minutes BCD tens 0..6
	unsigned int binary_group_flag_bit0:1; ///< indicate user-data char encoding, see table above - bit 43
	unsigned int user6:4;

	unsigned int hours_units:4; ///< SMPTE hours BCD unit 0..9
	unsigned int user7:4;

	unsigned int hours_tens:2; ///< SMPTE hours BCD tens 0..2
	unsigned int binary_group_flag_bit1:1; ///< indicate timecode is local time wall-clock, see table above - bit 58
	unsigned int binary_group_flag_bit2:1; ///< indicate user-data char encoding (or parity with 25fps), see table above - bit 59
	unsigned int user8:4;

	unsigned int sync_word:16;
};

struct __PACKED__ SMPTETimecode {
	char timezone[6];   ///< the timezone 6bytes: "+HHMM" textual representation
	unsigned char years; ///< LTC-date uses 2-digit year 00.99
	unsigned char months; ///< valid months are 1..12
	unsigned char days; ///< day of month 1..31

	unsigned char hours; ///< hour 0..23
	unsigned char mins; ///< minute 0..60
	unsigned char secs; ///< second 0..60
	unsigned char frame; ///< sub-second frame 0..(FPS - 1)
};


//-----------------------------------------------------------------------------
void ltc_exti_cb(EXTDriver *extp, expchannel_t channel);
void ltc_icu_period_cb(ICUDriver *icup);
int kuroBoxTimeInit(void);

#endif // _naniBox_kuroBox_time
