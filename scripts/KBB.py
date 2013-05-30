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

import sys
import os
import struct

DBG = 3

def calc_checksum(buf):
	xor = 0
	for c in buf:
		xor ^= ord(c)
		xor = xor % 256
	return xor

class KBB_V11(object):
	
	class KBB_V11_header():
		def __init__(self,msg):
			self.preamble,self.version,self.checksum,self.msg_size,self.msg_num,self.write_errors = \
				struct.unpack("<IBBHII",msg[0:16])

	class KBB_V11_ltc():
		"""
		struct SMPTETimecode {
			char timezone[6];   ///< the timezone 6bytes: "+HHMM" textual representation
			unsigned char years; ///< LTC-date uses 2-digit year 00.99
			unsigned char months; ///< valid months are 1..12
			unsigned char days; ///< day of month 1..31

			unsigned char hours; ///< hour 0..23
			unsigned char mins; ///< minute 0..60
			unsigned char secs; ///< second 0..60
			unsigned char frame; ///< sub-second frame 0..(FPS - 1)
		};
		"""
		def __init__(self,msg):
			self.timezone = msg[16:22]
			self.years,self.months,self.days,self.hours,self.mins,self.secs,self.frame = \
				struct.unpack("<BBBBBBB",msg[22:29])

	class KBB_V11_rtc():
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
				struct.unpack("<IIIIIIIII",msg[29:29+4*9])


	def __init__(self, arg):
		super(KBB_V11, self).__init__()
		KBB_V11.SIZE = 512
		self.fname = arg
		self.errors = {}
		self.fin = file(self.fname,"rb")
		self.msg_count = 0
		self.msg_num_prev = None
		self.set_index(0)

	def set_index(self,idx):
		self.fin.seek(idx*KBB_V11.SIZE)
		self.idx = idx

	def get_index(self):
		return self.idx-1 # since we already increment on read

	def inc_err(self,which,count):
		if not self.errors.has_key(which):
			self.errors[which] = 0
		self.errors[which] = self.errors[which] + count

	def get_err(self,which):
		if not self.errors.has_key(which):
			return 0
		return self.errors[which]

	def parse_header(self):
		self.header = self.KBB_V11_header(self.msg)
		return self.header
		
	def check_header(self):
		# check to see if we are missing packets
		if DBG>3:print self.header.msg_num
		if self.msg_num_prev is None:
			self.msg_num_prev = self.header.msg_num
		else:
			if self.msg_num_prev + 1 != self.header.msg_num:
				if DBG>2:print "Skipped a beat:", self.msg_num_prev, "->", self.header.msg_num, "@", self.msg_count, self.get_err("msg_skipped")
				self.inc_err("msg_skipped", self.header.msg_num-self.msg_num_prev)
			self.msg_num_prev = self.header.msg_num
		self.msg_count+=1

		# check the checksum
		cs = calc_checksum(self.msg[16:])
		if cs != self.header.checksum:
			self.inc_err("checksum mismatch",1)
		
	def parse_ltc(self):
		self.ltc = self.KBB_V11_ltc(self.msg)
		return self.ltc

	def parse_rtc(self):
		self.rtc = self.KBB_V11_rtc(self.msg)
		return self.rtc

	def parse_all(self):
		self.parse_header()
		self.parse_ltc()
		self.parse_rtc()

	def read_next(self):
		self.msg = self.fin.read(512)
		self.idx += 1
		if len(self.msg)==0:
			if DBG>1:print self.msg
			return False

		self.parse_all()
		return True

	def print_errors(self):
		if DBG>1: 
			print "Total msgs:", self.msg_count
			for k in self.errors.keys():
				print k,self.errors[k]
		

def KBB_factory(fname):
	fin = file(fname,"rb")
	msg = fin.read(5)
	if len(msg) != 5:
		return None
	preamble,version = struct.unpack("<IB",msg[0:5])
	return KBB_V11(fname)
