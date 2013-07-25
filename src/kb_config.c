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
#include "kb_config.h"
#include "kb_util.h"
#include <string.h>
#include <chprintf.h>

// config sources
#include "kb_serial.h"
#include "kb_featureA.h"
#include "kb_gpio.h"

//-----------------------------------------------------------------------------
// #define BKPSRAM_BASE				0x40024000
#define BKPSRAM_END					0x40024FFF
#define BKPSRAM_SIZE				(BKPSRAM_END-BKPSRAM_BASE)
#define BKPREG_MAGIC				0x426b426e	// nBkB ;)
#define CONFIG_VERSION				0x01010101

//-----------------------------------------------------------------------------
typedef struct config_t config_t;
struct config_t
{
	uint8_t 		serial1_pwr;
	int32_t 		serial1_baud;
	uint8_t 		serial2_pwr;
	int32_t 		serial2_baud;
	uint8_t 		lcd_backlight;
	int32_t			featureA;
};
config_t config;

//-----------------------------------------------------------------------------
// actual save/load
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int kbc_save(void)
{
	config.serial1_pwr = kbse_getPowerSerial1();
	config.serial1_baud = kbse_getBaudSerial1();
	config.serial2_pwr = kbse_getPowerSerial2();
	config.serial2_baud = kbse_getBaudSerial2();
	config.lcd_backlight = kbg_getLCDBacklight();
	config.featureA = kbfa_getFeature();

	return kbc_write(0, &config, sizeof(config));
}

//-----------------------------------------------------------------------------
int kbc_load(void)
{
	int ret = kbc_read(0, &config, sizeof(config));
	if (ret != KB_OK)
		return ret;

	kbse_setPowerSerial1(config.serial1_pwr);
	kbse_setBaudSerial1(config.serial1_baud);
	kbse_setPowerSerial2(config.serial2_pwr);
	kbse_setBaudSerial2(config.serial2_baud);
	kbg_setLCDBacklight(config.lcd_backlight);
	kbfa_setFeature(config.featureA);
	return KB_OK;
}

//-----------------------------------------------------------------------------
// housekeeping stuff below
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static int
check_offset(uint16_t offset, uint32_t length)
{
	if ( (offset+length) > BKPSRAM_SIZE )
		return KB_ERR_VALUE;
	return KB_OK;
}

//-----------------------------------------------------------------------------
static uint16_t
calc_cs(void)
{
	return calc_checksum_16((uint8_t*)(BKPSRAM_BASE), BKPSRAM_SIZE);
}

//-----------------------------------------------------------------------------
uint8_t kbc_read8(uint32_t offset)
{
	uint8_t data = 0;
	kbc_read(offset, (uint8_t*)&data, sizeof(data));
	return data;
}

//-----------------------------------------------------------------------------
uint16_t kbc_read16(uint32_t offset)
{
	uint16_t data = 0;
	kbc_read(offset, (uint8_t*)&data, sizeof(data));
	return data;
}

//-----------------------------------------------------------------------------
uint32_t kbc_read32(uint32_t offset)
{
	uint32_t data = 0;
	kbc_read(offset, (uint8_t*)&data, sizeof(data));
	return data;
}

//-----------------------------------------------------------------------------
int kbc_read(uint32_t offset, void * data, uint32_t length)
{
	check_offset(offset,length);
	memcpy(data, (uint8_t*)(BKPSRAM_BASE+offset), length);
	return KB_OK;
}

//-----------------------------------------------------------------------------
int kbc_write8(uint32_t offset, uint8_t data)
{
	return kbc_write(offset, (uint8_t*)&data, sizeof(data));
}

//-----------------------------------------------------------------------------
int kbc_write16(uint32_t offset, uint16_t data)
{
	return kbc_write(offset, (uint8_t*)&data, sizeof(data));
}

//-----------------------------------------------------------------------------
int kbc_write32(uint32_t offset, uint32_t data)
{
	return kbc_write(offset, (uint8_t*)&data, sizeof(data));
}

//-----------------------------------------------------------------------------
int kbc_write(uint32_t offset, void * data, uint32_t length)
{
	check_offset(offset,length);
	memcpy((uint8_t*)(BKPSRAM_BASE+offset), data, length);
	RTC->BKP2R = calc_cs();
	return KB_OK;
}

//-----------------------------------------------------------------------------
int kuroBoxConfigInit(void)
{
	if ( ( RTC->BKP0R != BKPREG_MAGIC ) ||
		 ( RTC->BKP1R != CONFIG_VERSION ) ||
		 ( RTC->BKP2R != calc_cs() ) )
	{
		// clear it all
		memset((uint8_t*)BKPSRAM_BASE, 0, BKPSRAM_SIZE);
		RTC->BKP0R = BKPREG_MAGIC;
		RTC->BKP1R = CONFIG_VERSION;
		RTC->BKP2R = calc_cs();
	}
	else
	{
		// if we wiped memory, don't load it, maybe do a save instead so
		// it gets put in memory, right?
		kbc_load();
	}

	return KB_OK;
}

//-----------------------------------------------------------------------------
int kuroBoxConfigStop(void)
{
	//kbc_save();
	return KB_OK;
}

