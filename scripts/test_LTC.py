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

import KBB
import KBB_util

def test_LTC(t, fps=30):
    l1 = KBB_util.LTC(t)
    l2 = KBB_util.LTC(t)
    l1.drop_frame_flag = True
    l2.drop_frame_flag = False
    print t
    print l1, l1.frame_number(fps), l1.drop_frame_flag 
    print l2, l2.frame_number(fps), l2.drop_frame_flag 
    print

def main():
    test_LTC("00:05:32:14")
    test_LTC("00:00:32:14")

    test_LTC("00:05:32:14", 24)
    test_LTC("00:00:32:14", 24)

if __name__ == '__main__':
    main()
