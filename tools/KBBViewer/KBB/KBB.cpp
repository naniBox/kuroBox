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
		case KBB_SUBCLASS_DATA_02:
			{
				const kbb_02_02_t * kbb_02_02  = reinterpret_cast<const kbb_02_02_t *>(data);
				packet = new KBB_02_02(*kbb_02_02);
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
// Implemented from
// http://www.nalresearch.com/files/Standard%20Modems/A3LA-XG/A3LA-XG%20SW%20Version%201.0.0/GPS%20Technical%20Documents/GPS.G1-X-00006%20(Datum%20Transformations).pdf
// input is in SI (meters)
#define A		6378137.000f
#define B		6356752.31424518f
#define EE		0.0066943799901411239f
#define E1E1	0.0067394967422762389f
void ecef_to_lla(float xx, float yy, float zz, float & olat, float & olon, float & oalt)
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

	olat = phi*180.0f/(float)M_PI;
	olon = lambda*180.0f/(float)M_PI;
	oalt = h;
}
