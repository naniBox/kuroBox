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

//-----------------------------------------------------------------------------
#include "kb_serial.h"
#include "kb_util.h"
#include "kb_gpio.h"

//-----------------------------------------------------------------------------
static SerialConfig serial1_default_cfg = {
	38400,
	0,
	USART_CR2_STOP1_BITS | USART_CR2_LINEN,
	0
};

//-----------------------------------------------------------------------------
static SerialConfig serial2_default_cfg = {
	38400,
	0,
	USART_CR2_STOP1_BITS | USART_CR2_LINEN,
	0
};

//-----------------------------------------------------------------------------
static SerialConfig * serial1_cfg;
static SerialConfig * serial2_cfg;

//-----------------------------------------------------------------------------
int kuroBoxSerialInit(SerialConfig  * s1_cfg, SerialConfig * s2_cfg)
{
	if ( !s1_cfg ) serial1_cfg = &serial1_default_cfg;
	if ( !s2_cfg ) serial2_cfg = &serial2_default_cfg;

	sdStart(&Serial1, serial1_cfg);
	sdStart(&Serial2, serial2_cfg);

	return KB_OK;
}

//-----------------------------------------------------------------------------
int kuroBoxSerialStop(void)
{
	sdStop(&Serial2);
	sdStop(&Serial1);
	kbg_setSerial1Pwr(0);
	kbg_setSerial2Pwr(0);

	return KB_OK;
}

//-----------------------------------------------------------------------------
int kbse_setPower(int which, int on)
{
	switch ( which )
	{
	case 1:
		{
			kbg_setSerial1Pwr(on);
		} return KB_OK;
	case 2:
		{
			kbg_setSerial2Pwr(on);
		} return KB_OK;
	}
	return KB_ERR_VALUE;
}

//-----------------------------------------------------------------------------
int kbse_getPower(int which)
{
	switch ( which )
	{
	case 1:	return kbg_getSerial1Pwr();
	case 2:	return kbg_getSerial2Pwr();
	}
	return -1;
}

//-----------------------------------------------------------------------------
int32_t kbse_getBaud(int which)
{
	switch ( which )
	{
	case 1:	return serial1_cfg->speed;
	case 2:	return serial2_cfg->speed;
	}
	return -1;
}

//-----------------------------------------------------------------------------
int kbse_changeBaud(int which)
{
	SerialConfig * cfg = NULL;
	SerialDriver * drv = NULL;
	switch ( which )
	{
	case 1:
		{
			cfg = serial1_cfg;
			drv = &Serial1;
		} break;
	case 2:
		{
			cfg = serial2_cfg;
			drv = &Serial2;
		} break;
	default: return KB_ERR_VALUE;
	}

	switch( cfg->speed )
	{
	case 300: 		cfg->speed = 	1200; 	break;
	case 1200: 		cfg->speed = 	9600; 	break;
	case 9600: 		cfg->speed = 	19200; 	break;
	case 19200: 	cfg->speed = 	38400; 	break;
	case 38400: 	cfg->speed = 	57600; 	break;
	case 57600: 	cfg->speed = 	115200; break;
	case 115200: 	cfg->speed = 	230400; break;
	case 230400: 	cfg->speed = 	460800; break;
	case 460800: 	cfg->speed = 	921600; break;
	case 921600: 	cfg->speed = 	300; 	break;
	default:  return KB_ERR_VALUE;
	}

	sdStop(drv);
	sdStart(drv, cfg);

	return KB_OK;
}

//-----------------------------------------------------------------------------
int kbse_setBaud(int which, int32_t baud)
{
	SerialConfig * cfg = NULL;
	SerialDriver * drv = NULL;
	switch ( which )
	{
	case 1:
		{
			cfg = serial1_cfg;
			drv = &Serial1;
		} break;
	case 2:
		{
			cfg = serial2_cfg;
			drv = &Serial2;
		} break;
	default: return KB_ERR_VALUE;
	}

	switch( baud )
	{
	case 300:
	case 1200:
	case 9600:
	case 19200:
	case 38400:
	case 57600:
	case 115200:
	case 230400:
	case 460800:
	case 921600:
		cfg->speed = baud;
		break;
	default:
		cfg->speed = 9600;
	}

	sdStop(drv);
	sdStart(drv, cfg);

	return KB_OK;
}
