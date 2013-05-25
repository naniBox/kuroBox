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

def parse_header(msg):
	preamble,version,checksum,msg_size,msg_num,write_errors = struct.unpack("<IBBHII",msg)
	return preamble,version,checksum,msg_size,msg_num,write_errors

def check_kbb(fname):
	print fname
	fin = file(fname,"rb")
	msg_count = 0
	msg_num_prev = None
	msg_skipped = 0
	while True:
		msg = fin.read(512)
		if len(msg)==0:
			print msg
			break
		preamble,version,checksum,msg_size,msg_num,write_errors = parse_header(msg[0:16])
		if msg_num_prev is None:
			msg_num_prev = msg_num
		else:
			if msg_num_prev + 1 != msg_num:
				print "Skipped a beat:", msg_num_prev, "->", msg_num, "@", msg_count
				msg_skipped += msg_num-msg_num_prev
			msg_num_prev = msg_num
		#print msg_num
		msg_count+=1

	print "Total msgs:", msg_count, "skipped:", msg_skipped

	
def main():
	for fname in sys.argv[1:]:
		check_kbb(fname)

if __name__ == '__main__':
	main()
