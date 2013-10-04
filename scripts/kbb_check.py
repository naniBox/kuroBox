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
import KBB
import glob
    
def main():
    for fname in glob.glob(sys.argv[1]):
        print "Checking '%s'"%fname
        kbb = KBB.KBB_factory(fname)
        if kbb is None:
            print "\tCan't parse!"
        while kbb.read_next():
            kbb.check_all()
        kbb.print_errors()

if __name__ == '__main__':
    main()
