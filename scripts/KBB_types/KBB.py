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
import sys
import os
import struct
import KBB_util 
import KBB_types

#---------------------------------------------------------------
class KBB(object):  

#---------------------------------------------------------------
	def __init__(self, arg):
		super(KBB, self).__init__()
		self.fname = arg
		self.idx = 0
		self.fin = file(self.fname,"rb")

#---------------------------------------------------------------
	def read_next(self):
		self.msg = self.fin.read(512)
		self.idx += 1
		if len(self.msg)==0:
			return False
		self.packet = None
		preamble,checksum,msg_class,msg_subclass,msg_size = struct.unpack("<IHBBH",self.msg[0:10])
		if msg_class == KBB_types.CLASS_HEADER:
			#print "Header", 
			if msg_subclass == KBB_types.SUBCLASS_HEADER_01:
				self.packet = KBB_types.KBB_01_01(self.msg)

		elif msg_class == KBB_types.CLASS_DATA:
			#print "Data", 
			if msg_subclass == KBB_types.SUBCLASS_DATA_01:
				self.packet = KBB_types.KBB_02_01(self.msg)
			elif msg_subclass == KBB_types.SUBCLASS_DATA_02:
				self.packet = KBB_types.KBB_02_02(self.msg)
				
		elif msg_class == KBB_types.CLASS_EXTERNAL:
			pass
			#print "External", 
		
		#print self.packet
		return True

#---------------------------------------------------------------
	def check_all(self):
		pass

#---------------------------------------------------------------
	def print_errors(self):
		pass
