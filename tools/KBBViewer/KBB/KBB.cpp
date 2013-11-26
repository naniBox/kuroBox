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

#include <sys/stat.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "KBB.h"

//-----------------------------------------------------------------------------
KBB::KBB()
	: m_fp(NULL)
	, m_size(0)
	, m_current_packet(NULL)
{
}

//-----------------------------------------------------------------------------
KBB::~KBB()
{
	close();
}

//-----------------------------------------------------------------------------
bool KBB::open(const std::string & fname)
{
	close();
	if ( fname == "" )
		return false;

	if ( fname == m_fname )
		return false;

	m_fp = fopen(fname.c_str(), "rb");

	if ( m_fp )
		m_fname = fname;

	seek(0);

	return m_fp != NULL;
}

//-----------------------------------------------------------------------------
bool KBB::close()
{
	if ( m_fp )
		fclose(m_fp);

	m_fp = NULL;

	if ( m_current_packet )
		delete m_current_packet;
	m_current_packet = NULL;

	return true;
}

//-----------------------------------------------------------------------------
const KBB_Packet * KBB::next()
{
	uint8_t buf[512];
	if ( fread(buf, KBB_MSG_SIZE, 1, m_fp) != 1 )
		return NULL;

	if ( m_current_packet )
		delete m_current_packet;
	m_current_packet = packetMaker(buf);
	return m_current_packet;
}

//-----------------------------------------------------------------------------
uint32_t KBB::size() const
{
	ASSERT(!m_fname.empty());

	if ( m_size )
		return m_size;

	struct stat st;
	stat(m_fname.c_str(), &st);
	return m_size = (st.st_size / KBB_MSG_SIZE);
}

//-----------------------------------------------------------------------------
bool KBB::seek(uint32_t idx)
{
	ASSERT(m_fp);

	int ret = fseek(m_fp, idx*KBB_MSG_SIZE, SEEK_SET);

	return ret == 0;
}

//-----------------------------------------------------------------------------
const KBB_Packet *
KBB::packetMaker(const uint8_t * data)
{
	ASSERT(data);

	kbb_header_t header;
	memcpy(&header, data, sizeof(header));

	uint16_t cs = KBB::calcCS(data);

	if ( cs != header.checksum )
	{
		// @TODO: bad thing here...
		return NULL;
	}
	
	kbb_02_01_t kbb_02_01;
	memset(&kbb_02_01, 0, sizeof(kbb_02_01));
	KBB_Packet * packet = NULL;
	switch ( header.msg_class )
	{
	case KBB_CLASS_HEADER:
		switch ( header.msg_subclass )
		{
		case KBB_SUBCLASS_HEADER_01:
			{
				const kbb_01_01_t * kbb_01_01  = reinterpret_cast<const kbb_01_01_t *>(data);
				packet = new KBB_01_01(*kbb_01_01);
			} break;
		default:
			break;
		}
		break;
	case KBB_CLASS_DATA:
		switch ( header.msg_subclass )
		{
		case KBB_SUBCLASS_DATA_01:
			{
				const kbb_02_01_t * kbb_02_01  = reinterpret_cast<const kbb_02_01_t *>(data);
				packet = new KBB_02_01(*kbb_02_01);
			} break;
		default:
			break;
		}
		break;

	default:
		break;
	}

	return packet;
}

//-----------------------------------------------------------------------------
uint16_t
KBB::calcCS(const uint8_t * data)
{
	ASSERT(data);

	uint16_t cs = calc_checksum_16(data+KBB_CHECKSUM_START, KBB_MSG_SIZE-KBB_CHECKSUM_START);

	return cs;
}

//-----------------------------------------------------------------------------
void
ltc_frame_to_smpte_timecode(smpte_timecode_t * smpte_timecode, const ltc_frame_t * ltc_frame)
{
	smpte_timecode->hours = ltc_frame->hours_units + ltc_frame->hours_tens*10;
	smpte_timecode->minutes  = ltc_frame->minutes_units  + ltc_frame->minutes_tens*10;
	smpte_timecode->seconds  = ltc_frame->seconds_units  + ltc_frame->seconds_tens*10;
	smpte_timecode->frames = ltc_frame->frame_units + ltc_frame->frame_tens*10;
}

//-----------------------------------------------------------------------------
uint32_t
ltc_frame_count(const smpte_timecode_t * smpte_timecode, uint32_t fps, bool drop_frame_flag)
{
	uint32_t total_seconds = smpte_timecode->hours*3600 + smpte_timecode->minutes*60 + smpte_timecode->seconds;
	uint32_t total_frames = total_seconds * fps + smpte_timecode->frames;
	if (drop_frame_flag)
	{
		const uint32_t df_per_hour = 2*60 - 2*6;
		total_frames -= df_per_hour * smpte_timecode->hours;
		total_frames -= ( smpte_timecode->minutes - ( smpte_timecode->minutes / 10 ) ) * 2;
	}
	return total_frames;
}

//-----------------------------------------------------------------------------
void ecef_to_lla(int32_t x, int32_t y, int32_t z, float & lat, float & lon, float & alt)
{
	float a = 6378137.0f;
	float e = 8.1819190842622e-2f;
	float b = 6356752.3142451793f; // sqrtf(powf(a,2) * (1-powf(e,2)));
	float ep = 0.082094437949695676f; // sqrtf((powf(a,2)-powf(b,2))/powf(b,2));
	float p = sqrtf(powf(x,2)+powf(y,2));
	float th = atan2f(a*z, b*p);
	lon = atan2f(y, x);
	lat = atan2f((z+ep*ep*b*powf(sinf(th),3)), (p-e*e*a*powf(cosf(th),3)));
	float n = a/sqrt(1-e*e*powf(sinf(lat),2));
	alt = p/cosf(lat)-n;
	lat = (lat*180.0f)/M_PI;
	lon = (lon*180.0f)/M_PI;
}

