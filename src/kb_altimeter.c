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
#include "kb_altimeter.h"
#include "kb_util.h"
#include "kb_writer.h"
#include "kb_screen.h"

//-----------------------------------------------------------------------------
#define BAROMETER_I2C_ADDRESS 0x60
static uint8_t txbuf[2];	// i only ever write out 2 regs
static uint8_t rxbuf[5];	// i only ever read the 5 status regs
static i2cflags_t errors = 0;

//-----------------------------------------------------------------------------
static msg_t
write_reg(uint8_t reg, uint8_t dat)
{
	systime_t timeout = MS2ST(4);
	txbuf[0] = reg;
	txbuf[1] = dat;
	i2cAcquireBus(&I2CD2);
	msg_t status = i2cMasterTransmitTimeout(&I2CD2, BAROMETER_I2C_ADDRESS, txbuf, 2, rxbuf, 0, timeout);
	i2cReleaseBus(&I2CD2);
	if (status != RDY_OK)
	{
		errors = i2cGetErrors(&I2CD2);
	}
	return status;
}

//-----------------------------------------------------------------------------
static msg_t
read_reg(uint8_t reg, uint8_t * dat, uint8_t dat_len)
{
	systime_t timeout = MS2ST(4);
	txbuf[0] = reg;
	i2cAcquireBus(&I2CD2);
	msg_t status = i2cMasterTransmitTimeout(&I2CD2, BAROMETER_I2C_ADDRESS, txbuf, 1, dat, dat_len, timeout);
	i2cReleaseBus(&I2CD2);
	if (status != RDY_OK)
	{
		errors = i2cGetErrors(&I2CD2);
	}
	return status;
}

//-----------------------------------------------------------------------------
static Thread * altimeterThread;
//-----------------------------------------------------------------------------
static WORKING_AREA(waAltimeter, 256);
static msg_t
thAltimeter(void *arg)
{
	(void)arg;
	chRegSetThreadName("Altimeter");

	msg_t status = RDY_OK;

	status = write_reg(0x26, 0xb8);
	status = write_reg(0x13, 0x07);
	status = write_reg(0x26, 0xb9);

	while( !chThdShouldTerminate() )
	{
		status = read_reg(0x01, rxbuf, 5);
		if ( status == RDY_OK )
		{

			int m_a,m_t,c_a;
			float l_a,l_t;

			m_a = rxbuf[0];
			c_a = rxbuf[1];
			l_a = (float)(rxbuf[2]>>4)/16.0f;
			m_t = rxbuf[3];
			l_t = (float)(rxbuf[4]>>4)/16.0f;
			float altitude = (float)((m_a << 8)|c_a)+l_a;
			float temperature = (float)(m_t + l_t);

			kbw_setAltitude(altitude, temperature);
			kbs_setTemperature((int)temperature);
		}
		chThdSleepMilliseconds(200);
	}
	return 0;
}

//-----------------------------------------------------------------------------
int kuroBoxAltimeterInit(void)
{
	altimeterThread = chThdCreateStatic(waAltimeter, sizeof(waAltimeter), NORMALPRIO, thAltimeter, NULL);
	return KB_OK;
}

//-----------------------------------------------------------------------------
int kuroBoxAltimeterStop(void)
{
	chThdTerminate(altimeterThread);
	chThdWait(altimeterThread);
	altimeterThread = NULL;
	return KB_OK;

}


