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

def calc_checksum_8(buf):
	xor = 0
	for c in buf:
		xor ^= ord(c)
		xor = xor % 256
	return xor

def calc_checksum_16(buf):
	a,b = 0,0
	for c in buf:
		a += ord(c)
		a = a%256
		b += a
		b = b%256
	return b*256+a

class LTC:
    """
        struct __PACKED__ ltc_frame_t
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
    def __init__(self, from_str=None):
        if from_str is not None:
            parts = from_str.split(":")
            self.hours = int(parts[0])
            self.minutes = int(parts[1])
            self.seconds = int(parts[2])
            self.frames = int(parts[3])
        else:
            self.hours = 0
            self.minutes = 0
            self.seconds = 0
            self.frames = 0

    def parse(self, bytes):
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

        self.drop_frame_flag = bytes[1]&0x04 == 0x04

    def frame_number(self,fps):
        total_seconds = self.hours*3600 + self.minutes*60 + self.seconds
        total_frames = total_seconds * fps + self.frames
        if self.drop_frame_flag:
            df_per_hour = 2*60 - 2*6
            total_frames -= df_per_hour * self.hours
            total_frames -= ( self.minutes - ( self.minutes / 10 ) ) * 2
        return total_frames

    def __str__(self):
        return "%02d:%02d:%02d.%02d"%(self.hours,self.minutes,self.seconds,self.frames)
    def __repr__(self):
        return self.__str__()

def LTC_LT(a, b):
	if a.hours < b.hours: return True
	if a.hours == b.hours:
		if a.minutes < b.minutes: return True
		if a.minutes == b.minutes:
			if a.seconds < b.seconds: return True
			if a.seconds == b.seconds:
				if a.frames < b.frames: return True
	return False
