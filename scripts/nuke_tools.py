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
import KBB

def mkNuke(fname):
    basedir,basename = os.path.split(fname)
    nukefname = os.path.join(basedir, os.path.splitext(basename)[0]+".nk")
    print fname
    print nukefname

    kbb = KBB.KBB_factory(fname)
    if kbb is None:
        print "Can't parse '%s'"%fname
        return False

    nukef = file(nukefname,"wb")

    kbb.set_index(0)
    first_tc_idx = 0
    while kbb.read_next():
        if kbb.ltc.hours != 0 or kbb.ltc.minutes != 0 or kbb.ltc.seconds != 0 or kbb.ltc.frames != 0:
            break
        first_tc_idx += 1
    print "First TC index:", first_tc_idx, kbb.ltc
    kbb.set_index(first_tc_idx)
    skip_item = None
    frame = 1
    data = [[] for i in range(3)]
    while kbb.read_next():
        if kbb.ltc.frames != skip_item:
            if frame%100==0:
                print kbb.header.msg_num, kbb.ltc, frame
            skip_item = kbb.ltc.frames
            data[0].append([frame, kbb.vnav.yaw])
            data[1].append([frame, kbb.vnav.pitch])
            data[2].append([frame, 180.0-kbb.vnav.roll])
            frame += 1
    for d in data:
        nukef.write("{curve ")
        for f,v in d:
            nukef.write("x%i %f "%(f,v))
        nukef.write("} ")

    nukef.close()

def main():
    for fname in sys.argv[1:]:
        mkNuke(fname)


if __name__ == '__main__':
    main()
