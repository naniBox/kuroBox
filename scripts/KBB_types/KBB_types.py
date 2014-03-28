"""
	kuroBox / naniBox
	Copyright (c) 2014
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

"""

#---------------------------------------------------------------
import struct
import KBB_util

#---------------------------------------------------------------
CLASS_HEADER 			= 0x01
CLASS_DATA 				= 0x10
CLASS_EXTERNAL 			= 0x80

#---------------------------------------------------------------
SUBCLASS_HEADER_01 		= 0x01
SUBCLASS_DATA_01 		= 0x01
SUBCLASS_DATA_02 		= 0x02
SUBCLASS_EXTERNAL_01 	= 0x01


#---------------------------------------------------------------
class ltc_frame_t(KBB_util.LTC):
	_unpack_str = "<BBBBBBBBBB"
	_unpack_size = struct.calcsize(_unpack_str)

	def __init__(self,msg):
		bytes = struct.unpack(ltc_frame_t._unpack_str,msg[0:ltc_frame_t._unpack_size])
		KBB_util.LTC.parse(self, bytes)		

	def __str__(self):
		return "%02d:%02d:%02d.%02d"%(self.hours,self.minutes,self.seconds,self.frames)
	def __repr__(self):
		return self.__str__()

#---------------------------------------------------------------
class rtc_t():
	_unpack_str = "<IIIIIIIII"
	_unpack_size = struct.calcsize(_unpack_str)
	"""
	struct tm
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
	"""
	def __init__(self,msg):
		self.sec,self.min,self.hour,self.mday,self.mon,self.year,self.wday,self.yday,self.isdst = \
			struct.unpack(rtc_t._unpack_str,msg[0:rtc_t._unpack_size])

	def __str__(self):
		return "%04d-%02d-%02d %02d:%02d:%02d"%(self.year+1900, self.mon+1, self.mday, self.hour,self.min,self.sec)

	def __repr__(self):
		return self.__str__()

#---------------------------------------------------------------
class ubx_nav_sol_t():
	_unpack_str = "<HHHIihBBiiiIiiiIHBBIH"
	_unpack_size = struct.calcsize(_unpack_str)
	"""
	struct ubx_nav_sol_t
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

	"""
	def __init__(self,msg):
		self.header,self.msg_id,self.msg_len,self.itow,self.ftow,self.week,self.gpsfix,self.flags, \
			self.ecefX,self.ecefY,self.ecefZ,self.pAcc,self.ecefVX,self.ecefVY,self.ecefVZ, \
			self.sAcc,self.pdop,self.reserved1,self.numSV,self.reserved2,self.cs = \
			struct.unpack(ubx_nav_sol_t._unpack_str,msg[0:ubx_nav_sol_t._unpack_size])
		self.calc_cs = KBB_util.calc_checksum_16(msg[2:ubx_nav_sol_t._unpack_size-2])
		self.calculatedLLA = False

	def calculateLLA(self):
		if self.calculatedLLA:
			return
		import math
		# Constants (WGS ellipsoid)
		a = 6378137.0
		e = 8.1819190842622e-2
		b = 6356752.3142451793 # math.sqrt(pow(a,2) * (1-pow(e,2)))
		ep = 0.082094437949695676 #math.sqrt((pow(a,2)-pow(b,2))/pow(b,2))
		# Calculation
		ecefX = float(self.ecefX)
		ecefY = float(self.ecefY)
		ecefZ = float(self.ecefZ)
		p = math.sqrt(pow(ecefX,2)+pow(ecefY,2))
		th = math.atan2(a*ecefZ, b*p)
		self.lon = math.atan2(ecefY, ecefX)
		self.lat = math.atan2((ecefZ+ep*ep*b*pow(math.sin(th),3)), (p-e*e*a*pow(math.cos(th),3)))
		n = a/math.sqrt(1-e*e*pow(math.sin(self.lat),2))
		self.alt = p/math.cos(self.lat)-n
		self.lat = (self.lat*180.0)/math.pi
		self.lon = (self.lon*180.0)/math.pi
		self.calculatedLLA = True

	def __str__(self):
		self.calculateLLA()
		return "%3.7f, %3.7f, %3.7f"%(self.lat, self.lon, self.alt)+("" if self.calc_cs == self.cs else " WRONG_CS")

	def __repr__(self):
		return self.__str__()

#---------------------------------------------------------------
class vnav_data_t():
	_unpack_str = "<fffI"
	_unpack_size = struct.calcsize(_unpack_str)
	"""
	struct tm
	{
	  float 	ypr[3];
	  uint32_t	ypr_ts;
	};
	"""
	def __init__(self,msg):
		self.yaw,self.pitch,self.roll,self.ypr_ts = struct.unpack(vnav_data_t._unpack_str,msg[0:vnav_data_t._unpack_size])

	def __str__(self):
		return "%4.1f, %4.1f, %4.1f"%(self.yaw,self.pitch,self.roll)

	def __repr__(self):
		return self.__str__()




#---------------------------------------------------------------
class HEADER:
	_unpack_str = "<IHBBH"
	_unpack_size = struct.calcsize(_unpack_str)
	def __init__(self, msg):
		self.msg = msg
		self.preamble,self.checksum,self.msg_class,self.msg_subclass,self.msg_size = \
			struct.unpack(HEADER._unpack_str,self.msg[0:KBB_01_01._unpack_size])

#---------------------------------------------------------------
class KBB_01_01:
	_unpack_str = "<IHBBH"
	_unpack_size = struct.calcsize(_unpack_str)
	def __init__(self, msg):
		self.msg = msg
		self.header = HEADER(self.msg[0:HEADER._unpack_size])

#---------------------------------------------------------------
class KBB_02_01:
	_unpack_str = "<IHBBH"
	_unpack_size = struct.calcsize(_unpack_str)
	def __init__(self, msg):
		self.msg = msg
		self.header = HEADER(self.msg[0:HEADER._unpack_size])

#---------------------------------------------------------------
class KBB_02_02:
	"""
//-----------------------------------------------------------------------------
typedef struct kbb_02_02_t kbb_02_02_t;	// the data packet
struct __PACKED__ kbb_02_02_t
{
										// @ 0
	kbb_header_t header;				// 10 bytes


										// @ 10
	uint32_t msg_num;					// 4
	uint32_t write_errors;				// 4
										// = 18 for HEADER block

										// @ 18
	ltc_frame_t ltc_frame;				// 10
	rtc_t rtc;							// 36
	uint32_t one_sec_pps;				// 4
	smpte_timecode_t smpte_time;		// 4
										// = 54 for TIME (LTC+RTC+FPS) block

										// @ 72
	uint32_t pps;						// 4
	ubx_nav_sol_t nav_sol;				// 60
										// = 64 for GPS block

										// @ 136
	vnav_data_t vnav;					// 4*3+4 = 16
										// 16 for the VNAV block

										// @ 152
	float altitude;						// 4
	float temperature;					// 4
										// 8 for the altimeter block

	uint32_t global_count;

	uint8_t __pad[512 - (18 + 54 + 64 + 16 + 8 + 4)];
};	"""
	def __init__(self, msg):
		self.msg = msg
		self.header = HEADER(self.msg[0:HEADER._unpack_size])
		self.msg_num,self.write_errors = struct.unpack("<II",self.msg[10:10+8])

		self.ltc_frame = ltc_frame_t(self.msg[18:18+ltc_frame_t._unpack_size])
		self.rtc = rtc_t(self.msg[28:28+rtc_t._unpack_size])
		self.one_sec_pps = struct.unpack("<I",self.msg[64:64+4])
		#self.smpte_time

		self.pps = struct.unpack("<I",self.msg[72:72+4])
		self.nav_sol = ubx_nav_sol_t(self.msg[76:76+ubx_nav_sol_t._unpack_size])

		self.vnav = vnav_data_t(self.msg[136:136+vnav_data_t._unpack_size])

		#print self

	def __str__(self):
		return ", ".join([str(i) for i in ["0x%08X"%self.header.preamble, "0x%04X"%self.header.checksum, self.ltc_frame, \
			self.rtc, self.nav_sol, self.vnav, self.msg_num, self.write_errors]])
	def __repr__(self):
		return self.__str__()
