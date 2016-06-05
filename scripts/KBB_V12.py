#!/usr/bin/env python2
"""
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

"""

#---------------------------------------------------------------
import sys
import os
import struct
import KBB_util 

#---------------------------------------------------------------
DBG = 2

#---------------------------------------------------------------
#---------------------------------------------------------------
class KBB_V12_HEADER():
	def __init__(self,msg):
		assert len(msg) == 512
		self.msg = msg
		self.preamble,self.checksum,self.version,self.msg_type,self.msg_size = \
			struct.unpack("<IHBBH",msg[0:10])

		self.vnav_user_tag,self.vnav_model,self.vnav_hwrev,\
			self.vnav_sn_a,self.vnav_sn_b,self.vnav_sn_c,\
			self.vnav_fwver_major,self.vnav_fwver_minor,self.vnav_fwver_build,self.vnav_fwver_rev = \
			struct.unpack("<20s24sI3I4B",msg[10:10+64])


#---------------------------------------------------------------
#---------------------------------------------------------------
class KBB_V12_MSG_header():
	START_BYTE = 0
	LENGTH = 18
	def __init__(self,msg):
		self.preamble,self.checksum,self.version,self.msg_type,self.msg_size,self.msg_num,self.write_errors = \
			struct.unpack("<IHBBHII",msg[KBB_V12_MSG_header.START_BYTE:KBB_V12_MSG_header.START_BYTE+KBB_V12_MSG_header.LENGTH])

#---------------------------------------------------------------
#---------------------------------------------------------------
class KBB_V12_MSG_ltc(KBB_util.LTC):
	START_BYTE = 18
	LENGTH = 10

	def __init__(self,msg):
		bytes = struct.unpack("<BBBBBBBBBB",msg[KBB_V12_MSG_ltc.START_BYTE:KBB_V12_MSG_ltc.START_BYTE+KBB_V12_MSG_ltc.LENGTH])
		KBB_util.LTC.parse(self, bytes)		

	def __str__(self):
		return "%02d:%02d:%02d.%02d"%(self.hours,self.minutes,self.seconds,self.frames)
	def __repr__(self):
		return self.__str__()

#---------------------------------------------------------------
#---------------------------------------------------------------
class KBB_V12_MSG_rtc():
	START_BYTE = 28
	LENGTH = 36
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
			struct.unpack("<IIIIIIIII",msg[KBB_V12_MSG_rtc.START_BYTE:KBB_V12_MSG_rtc.START_BYTE+KBB_V12_MSG_rtc.LENGTH])

	def __str__(self):
		return "%04d-%02d-%02d %02d:%02d:%02d"%(self.year+1900, self.mon+1, self.mday, self.hour,self.min,self.sec)

	def __repr__(self):
		return self.__str__()

#---------------------------------------------------------------
#---------------------------------------------------------------
class KBB_V12_MSG_nav_sol():
	START_BYTE = 64
	LENGTH = 64
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
		self.pps,self.header,self.msg_id,self.msg_len,self.itow,self.ftow,self.week,self.gpsfix,self.flags, \
			self.ecefX,self.ecefY,self.ecefZ,self.pAcc,self.ecefVX,self.ecefVY,self.ecefVZ, \
			self.sAcc,self.pdop,self.reserved1,self.numSV,self.reserved2,self.cs = \
			struct.unpack("<IHHHIihBBiiiIiiiIHBBIH", msg[KBB_V12_MSG_nav_sol.START_BYTE:KBB_V12_MSG_nav_sol.START_BYTE+KBB_V12_MSG_nav_sol.LENGTH])
		self.calc_cs = KBB_util.calc_checksum_16(msg[KBB_V12_MSG_nav_sol.START_BYTE+6:KBB_V12_MSG_nav_sol.START_BYTE+KBB_V12_MSG_nav_sol.LENGTH-2])
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
		return "%3.7f, %3.7f, %3.7f"%(self.lat, self.lon, self.alt)

	def __repr__(self):
		return self.__str__()

#---------------------------------------------------------------
#---------------------------------------------------------------
class KBB_V12_MSG_vnav():
	START_BYTE = 128
	LENGTH = 16
	"""
	struct tm
	{
	  float 	ypr[3];
	  uint32_t	ypr_ts;
	};
	"""
	def __init__(self,msg):
		self.yaw,self.pitch,self.roll,self.ypr_ts = struct.unpack("<fffI",msg[KBB_V12_MSG_vnav.START_BYTE:KBB_V12_MSG_vnav.START_BYTE+KBB_V12_MSG_vnav.LENGTH])

	def __str__(self):
		self.calculateLLA()
		return "%4.1f, %4.1f, %4.1f"%(self.yaw,self.pitch,self.roll)

	def __repr__(self):
		return self.__str__()

#---------------------------------------------------------------
#---------------------------------------------------------------
class KBB_V12_MSG_altimeter():
	START_BYTE = 144
	LENGTH = 8
	"""
	struct tm
	{
	  float 	altitude
	  float		temperature;
	};
	"""
	def __init__(self,msg):
		self.altitude,self.temperature = struct.unpack("<ff",msg[KBB_V12_MSG_altimeter.START_BYTE:KBB_V12_MSG_altimeter.START_BYTE+KBB_V12_MSG_altimeter.LENGTH])

	def __str__(self):
		self.calculateLLA()
		return "%4.1f, %4.1f"%(self.altitude,self.temperature)

	def __repr__(self):
		return self.__str__()

#---------------------------------------------------------------
#---------------------------------------------------------------
class KBB_V12_MSG_global_counter():
	START_BYTE = 152
	LENGTH = 4
	"""
	  uint32_t	global_counter;
	"""
	def __init__(self,msg):
		self.global_counter = struct.unpack("<I",msg[KBB_V12_MSG_global_counter.START_BYTE:KBB_V12_MSG_global_counter.START_BYTE+KBB_V12_MSG_global_counter.LENGTH])

	def __str__(self):
		return "%d"%self.global_counter

	def __repr__(self):
		return self.__str__()


#---------------------------------------------------------------
#---------------------------------------------------------------
class KBB_V12(object):	

#---------------------------------------------------------------
	def __init__(self, arg):
		super(KBB_V12, self).__init__()
		KBB_V12.SIZE = 512
		self.fname = arg
		self.errors = {}
		self.fin = file(self.fname,"rb")
		self.msg_count = 0
		self.msg_num_prev = None
		self.total_msg_count = os.stat(self.fname).st_size / 512 - 1 # because of header!
		self.HEADER = KBB_V12_HEADER(self.fin.read(512))
		self.set_index(0)

#---------------------------------------------------------------
	def set_index(self,idx):
		self.fin.seek((idx+1)*KBB_V12.SIZE)
		self.idx = idx

#---------------------------------------------------------------
	def get_index(self):
		return self.idx

#---------------------------------------------------------------
	def inc_err(self,which,count):
		if not self.errors.has_key(which):
			self.errors[which] = 0
		self.errors[which] = self.errors[which] + count

#---------------------------------------------------------------
	def get_err(self,which):
		if not self.errors.has_key(which):
			return 0
		return self.errors[which]

#---------------------------------------------------------------
	def parse_header(self):
		self.header = KBB_V12_MSG_header(self.msg)
		return self.header
		
#---------------------------------------------------------------
	def check_header(self):
		# check to see if we are missing packets
		if DBG>3:print self.header.msg_num
		if self.msg_num_prev is None:
			self.msg_num_prev = self.header.msg_num
		else:
			if self.msg_num_prev + 1 != self.header.msg_num:
				if DBG>2:print "Skipped a beat:", self.msg_num_prev, "->", self.header.msg_num, "@", self.msg_count, self.get_err("msg_skipped"), self.header.msg_num-self.msg_num_prev
				self.inc_err("msg_skipped", self.header.msg_num-self.msg_num_prev)
			self.msg_num_prev = self.header.msg_num
		self.msg_count+=1

		# check the checksum
		self.header.calculated_checksum = KBB_util.calc_checksum_16(self.msg[6:])
		if self.header.calculated_checksum != self.header.checksum:
			self.valid_packet = False
			self.inc_err("checksum mismatch",1)
		else:
			self.valid_packet = True
		
#---------------------------------------------------------------
	def parse_ltc(self):
		self.ltc = KBB_V12_MSG_ltc(self.msg)
		return self.ltc

#---------------------------------------------------------------
	def parse_rtc(self):
		self.rtc = KBB_V12_MSG_rtc(self.msg)
		return self.rtc

#---------------------------------------------------------------
	def parse_nav_sol(self):
		self.nav_sol = KBB_V12_MSG_nav_sol(self.msg)
		return self.nav_sol

#---------------------------------------------------------------
	def check_nav_sol(self):
		if self.nav_sol.cs != self.nav_sol.calc_cs:
			self.inc_err("nav_sol_cs",1)
			if DBG>2:print "NAV_SOL cs mismatch:",self.nav_sol.cs,self.nav_sol.calc_cs

#---------------------------------------------------------------
	def parse_vnav(self):
		self.vnav = KBB_V12_MSG_vnav(self.msg)
		return self.vnav

#---------------------------------------------------------------
	def parse_global_counter(self):
		self.global_counter = KBB_V12_MSG_global_counter(self.msg)
		return self.global_counter

#---------------------------------------------------------------
	def parse_altitude(self):
		self.altimeter = KBB_V12_MSG_altimeter(self.msg)
		return self.altimeter

#---------------------------------------------------------------
	def parse_all(self):
		self.parse_header()
		self.parse_ltc()
		self.parse_rtc()
		self.parse_nav_sol()
		self.parse_vnav()
		self.parse_altitude()
		self.parse_global_counter()

#---------------------------------------------------------------
	def check_all(self):
		self.check_header()
		self.check_nav_sol()

#---------------------------------------------------------------
	def read_next(self):
		self.msg = self.fin.read(512)
		self.idx += 1
		if len(self.msg)==0:
			if DBG>1:print self.msg
			return False

		self.parse_all()
		return True

#---------------------------------------------------------------
	def print_errors(self):
		if DBG>1: 
			print "Total msgs:", self.msg_count
			for k in self.errors.keys():
				print k,self.errors[k], "%4.2f%%"%(float(self.errors[k]*100)/float(self.msg_count))
