/*
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

*/

#ifndef _naniBox_kuroBox_serial
#define _naniBox_kuroBox_serial

//----------------------------------------------------------------------------
#include <ch.h>
#include <hal.h>

//----------------------------------------------------------------------------
// this is so we can address serial ports consistently even if they change
// the actual port.
#define Serial1 SD1
#define Serial2 SD2

//----------------------------------------------------------------------------
#define DEBG ((BaseSequentialStream *)&Serial2)

//-----------------------------------------------------------------------------
int kuroBoxSerialInit(SerialConfig  * serial1_cfg, SerialConfig * serial2_cfg);
int kuroBoxSerialStop(void);

//-----------------------------------------------------------------------------
int kbse_setPower(int which, int on);
int kbse_getPower(int which);

//----------------------------------------------------------------------------
#define kbse_setPowerSerial1(on) 		kbse_setPower(1,on)
#define kbse_setPowerSerial2(on) 		kbse_setPower(2,on)

#define kbse_setPowerSerial1On() 		kbse_setPowerSerial1(1)
#define kbse_setPowerSerial1Off() 		kbse_setPowerSerial1(0)

#define kbse_setPowerSerial2On() 		kbse_setPowerSerial2(1)
#define kbse_setPowerSerial2Off() 		kbse_setPowerSerial2(0)

#define kbse_getPowerSerial1() 			kbse_getPower(1)
#define kbse_getPowerSerial2() 			kbse_getPower(2)

//-----------------------------------------------------------------------------
int32_t kbse_getBaud(int which);
int kbse_setBaud(int which, int32_t baud);
int kbse_changeBaud(int which);

#define kbse_getBaudSerial1()			kbse_getBaud(1)
#define kbse_getBaudSerial2()			kbse_getBaud(2)

#define kbse_setBaudSerial1(baud)		kbse_setBaud(1, baud)
#define kbse_setBaudSerial2(baud)		kbse_setBaud(2, baud)

#define kbse_changeBaudSerial1()		kbse_changeBaud(1)
#define kbse_changeBaudSerial2()		kbse_changeBaud(2)

#endif // _naniBox_kuroBox_serial
