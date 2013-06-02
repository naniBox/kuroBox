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
			struct __PACKED__ LTCFrame
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
		"""
		def __init__(self,msg):
			bytes = struct.unpack("<BBBBBBBBBB",msg[16:26])
			
			frame_units = bytes[0]&0x0f
			frame_tens = bytes[1]&0x03
			
			second_units = bytes[2]&0xf
			second_tens = bytes[3]&0x07

			minute_units = bytes[4]&0xf
			minute_tens = bytes[5]&0x07

			hour_units = bytes[6]&0xf
			hour_tens = bytes[7]&0x03

			self.frames = frame_units+frame_tens*10
			self.seconds = second_units+second_tens*10
			self.minutes = minute_units+minute_tens*10
			self.hours = hour_units+hour_tens*10

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
				struct.unpack("<IIIIIIIII",msg[26:62])


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
		self.header.calculated_checksum = calc_checksum(self.msg[16:])
		if self.header.calculated_checksum != self.header.checksum:
			self.valid_packet = False
			self.inc_err("checksum mismatch",1)
		else:
			self.valid_packet = True
		
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

	def check_all(self):
		self.check_header()

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
