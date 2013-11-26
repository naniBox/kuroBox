/*
	kuroBox / naniBox
	Copyright (c) 2013
	david morris-oliveros // dmo@nanibox.com // naniBox.com

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

#ifndef KBB_types_H
#define KBB_types_H

#include <stdint.h>

#pragma pack(push,1)

//-----------------------------------------------------------------------------
#define UBX_NAV_SOL_SIZE						60
typedef struct ubx_nav_sol_t ubx_nav_sol_t;
struct __PACKED__ ubx_nav_sol_t
{
	uint16_t header;
	uint16_t id;
	uint16_t len;
	uint32_t itow;
	int32_t ftow;
	int16_t week;
	uint8_t gpsfix;
	uint8_t flags;
	int32_t ecefX;
	int32_t ecefY;
	int32_t ecefZ;
	uint32_t pAcc;

	int32_t ecefVX;
	int32_t ecefVY;
	int32_t ecefVZ;
	uint32_t sAcc;
	uint16_t pdop;
	uint8_t reserved1;
	uint8_t numSV;
	uint32_t reserved2;

	uint16_t cs;
};
STATIC_ASSERT(sizeof(ubx_nav_sol_t)==UBX_NAV_SOL_SIZE, UBX_NAV_SOL_SIZE);

//-----------------------------------------------------------------------------
#define FACTORY_CONFIG_USER_MAX_LENGTH		32
#define FACTORY_CONFIG_SIZE					128
typedef struct factory_config_t factory_config_t;
struct __PACKED__ factory_config_t
{
	uint32_t preamble;
	uint32_t version;
	uint32_t serial_number;
	uint32_t hardware_revision;
	int32_t tcxo_compensation;
	int32_t rtc_compensation;
	int32_t vin_compensation;
	char user_string[FACTORY_CONFIG_USER_MAX_LENGTH];
	uint8_t __pad[FACTORY_CONFIG_SIZE - (4+4+2+4+4+4+4+4+FACTORY_CONFIG_USER_MAX_LENGTH)];

	// checksum always at the end, not included in checksum calculations!
	uint16_t checksum;
};
STATIC_ASSERT(sizeof(factory_config_t)==FACTORY_CONFIG_SIZE, FACTORY_CONFIG_SIZE);

//-----------------------------------------------------------------------------
// this is the raw 80bit LTC frame as defined:
// http://en.wikipedia.org/wiki/Linear_timecode
#define LTC_FRAME_SIZE 10
typedef struct ltc_frame_t ltc_frame_t;
struct __PACKED__ ltc_frame_t
{
	uint8_t frame_units:4;
	uint8_t user_bits_1:4;
	uint8_t frame_tens:2;
	uint8_t drop_frame_flag:1;
	uint8_t color_frame_flag:1;
	uint8_t user_bits_2:4;

	uint8_t seconds_units:4;
	uint8_t user_bits_3:4;
	uint8_t seconds_tens:3;
	uint8_t even_parity_bit:1;
	uint8_t user_bits_4:4;

	uint8_t minutes_units:4;
	uint8_t user_bits_5:4;
	uint8_t minutes_tens:3;
	uint8_t binary_group_flag_1:1;
	uint8_t user_bits_6:4;

	uint8_t hours_units:4;
	uint8_t user_bits_7:4;
	uint8_t hours_tens:2;
	uint8_t reserved:1;
	uint8_t binary_group_flag_2:1;
	uint8_t user_bits_8:4;

	uint16_t sync_word:16;
};
STATIC_ASSERT(sizeof(ltc_frame_t)==LTC_FRAME_SIZE, LTC_FRAME_SIZE); // 80bits

//-----------------------------------------------------------------------------
// interpreted timecode with just HH:MM:SS:FF, none of the userbit intepreted
#define SMPTE_TIMECODE_SIZE 4
typedef struct smpte_timecode_t smpte_timecode_t;
struct __PACKED__ smpte_timecode_t
{
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	uint8_t frames;
};
STATIC_ASSERT(sizeof(smpte_timecode_t)==SMPTE_TIMECODE_SIZE, SMPTE_TIMECODE_SIZE);

//-----------------------------------------------------------------------------
// This struct represents as close as possible the standard C struct tm
// I've duplicated it here since on OSX it's different and this allows me
// not to be dependant on the whims of any external modifications
#define RTC_SIZE 36
typedef struct rtc_t rtc_t;
struct __PACKED__ rtc_t
{
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
};
STATIC_ASSERT(sizeof(rtc_t)==RTC_SIZE, RTC_SIZE);

//-----------------------------------------------------------------------------
#define VNAV_DATA_SIZE	16
typedef struct vnav_data_t vnav_data_t;
struct __PACKED__ vnav_data_t
{
	float ypr[3];
	uint32_t ypr_ts;
};
STATIC_ASSERT(sizeof(vnav_data_t)==VNAV_DATA_SIZE, VNAV_DATA_SIZE);

//-----------------------------------------------------------------------------
#define KBB_MSG_SIZE					512
#define KBB_HEADER_SIZE					10
// nBkB  backwards, since this architecture is little-endian, we
// pre-swizzle the bytes
#define KBB_PREAMBLE					0x426b426e
#define KBB_CHECKSUM_START				6

#define KBB_CLASS_HEADER				0x01
#define KBB_CLASS_DATA					0x10

#define KBB_SUBCLASS_HEADER_01			0x01
#define KBB_SUBCLASS_DATA_01			0x01


//-----------------------------------------------------------------------------
//
typedef struct kbb_header_t kbb_header_t;
struct __PACKED__ kbb_header_t
{
	uint32_t preamble;					// 4
	uint16_t checksum;					// 2
	uint8_t msg_class;					// 1
	uint8_t msg_subclass;				// 1
	uint16_t msg_size;					// 2
	// = 10 for HEADER block
};
STATIC_ASSERT(sizeof(kbb_header_t)==KBB_HEADER_SIZE, KBB_HEADER_SIZE);


//-----------------------------------------------------------------------------
// this msg is written at the start of every file
typedef struct kbb_01_01_t kbb_01_01_t; // the header packet
struct __PACKED__ kbb_01_01_t
{
	kbb_header_t header;				// 10 bytes

	uint8_t vnav_header[64];			// vnav stuff, dumped in here

	uint8_t __pad[512 - (10 + 64)];		// 438 left
};

STATIC_ASSERT(sizeof(kbb_01_01_t)==KBB_MSG_SIZE, KBB_MSG_SIZE);

//-----------------------------------------------------------------------------
typedef struct kbb_02_01_t kbb_02_01_t;	// the data packet
struct __PACKED__ kbb_02_01_t
{
	kbb_header_t header;				// 10 bytes

	uint32_t msg_num;					// 4
	uint32_t write_errors;				// 4
	// = 18 for HEADER block

	ltc_frame_t ltc_frame;				// 10
	rtc_t rtc;							// 36
	uint32_t one_sec_pps;				// 4
	// = 50 for TIME (LTC+RTC) block

	uint32_t pps;						// 4
	ubx_nav_sol_t nav_sol;				// 60
	// = 64 for GPS block

	vnav_data_t vnav;					// 4*3+4 = 16
	// 16 for the VNAV block

	float altitude;						// 4
	float temperature;					// 4
	// 8 for the altimeter block

	uint32_t global_count;

	uint8_t __pad[512 - (18 + 50 + 64 + 16 + 8 + 4)];
};
typedef kbb_02_01_t kbb_current_msg_t;
STATIC_ASSERT(sizeof(kbb_02_01_t)==KBB_MSG_SIZE, KBB_MSG_SIZE);
STATIC_ASSERT(sizeof(kbb_current_msg_t)==KBB_MSG_SIZE, KBB_MSG_SIZE);

#pragma pack(pop)

#endif // KBB_types_H
