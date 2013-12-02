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

//-----------------------------------------------------------------------------
#ifndef KBB_H
#define KBB_H

//-----------------------------------------------------------------------------
#include <string>
#include "utils.h"
#include "KBB_types.h"
KBB_TYPES_VERSION_CHECK(0x0002)

//-----------------------------------------------------------------------------
class KBB_Packet
{
public:
	KBB_Packet() {}
	virtual ~KBB_Packet() {}

	virtual const std::string name() const = 0;
	virtual const kbb_header_t & header() const = 0;
protected:
};

//-----------------------------------------------------------------------------
class KBB_01_01 : public KBB_Packet
{
public:
	explicit KBB_01_01(const kbb_01_01_t & packet) : m_packet(packet) {}
	const kbb_01_01_t & packet() const { return m_packet; }
	virtual const std::string name() const { return "KBB_01_01"; }
	virtual const kbb_header_t & header() const { return m_packet.header; }
protected:
	kbb_01_01_t m_packet;
};

//-----------------------------------------------------------------------------
class KBB_02_01 : public KBB_Packet
{
public:
	explicit KBB_02_01(const kbb_02_01_t & packet) : m_packet(packet) {}
	const kbb_02_01_t & packet() const { return m_packet; }
	virtual const std::string name() const { return "KBB_02_01"; }
	virtual const kbb_header_t & header() const { return m_packet.header; }
protected:
	kbb_02_01_t m_packet;
};

//-----------------------------------------------------------------------------
class KBB_02_02 : public KBB_Packet
{
public:
	explicit KBB_02_02(const kbb_02_02_t & packet) : m_packet(packet) {}
	const kbb_02_02_t & packet() const { return m_packet; }
	virtual const std::string name() const { return "KBB_02_02"; }
	virtual const kbb_header_t & header() const { return m_packet.header; }
protected:
	kbb_02_02_t m_packet;
};

//-----------------------------------------------------------------------------
class KBB
{
public:
	explicit KBB();
	~KBB();

	bool open(const std::string & fname);
	bool close();

	uint32_t size() const;
	const KBB_Packet * current() const { return m_current_packet; }
	const KBB_Packet * next();
	bool seek(uint32_t idx);

	static const KBB_Packet * packetMaker(const uint8_t * data);
	static uint16_t calcCS(const uint8_t * data);	// always a MSG_SIZE packet, ignoring header
protected:
	std::string m_fname;
	FILE * m_fp;
	mutable uint32_t m_size;
	const KBB_Packet * m_current_packet;
};

//-----------------------------------------------------------------------------
void ltc_frame_to_smpte_timecode(smpte_timecode_t * smpte_timecode, const ltc_frame_t * ltc_frame);
uint32_t ltc_frame_count(const smpte_timecode_t * smpte_timecode, uint32_t fps, bool drop_frame_flag);
void ecef_to_lla(float x, float y, float z, float & olat, float & olon, float & oalt);

#endif // KBB_H
