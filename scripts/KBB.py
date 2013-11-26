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

def KBB_factory(fname):
	fin = file(fname,"rb")
	msg = fin.read(18)
	fin.close()
	if len(msg) != 18:
		return None
	preamble,checksum,version,msg_size,msg_num,write_errors = struct.unpack("<IBBHII",msg[0:16])
	if version == 11:
		import KBB_V11
		return KBB_V11.KBB_V11(fname)
	preamble,checksum,version,msg_type,msg_size,msg_num,write_errors = struct.unpack("<IHBBHII",msg[0:18])
	if version == 12:
		import KBB_V12
		return KBB_V12.KBB_V12(fname)
	return None

