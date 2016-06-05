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
#include "spiEEPROM.h"
#include "kb_time.h"

// config sources
#include "kb_serial.h"
#include "kb_externalDisplay.h"
#include "kb_gpio.h"
#include "kb_screen.h"

//-----------------------------------------------------------------------------
#include "kbb_types.h"
KBB_TYPES_VERSION_CHECK(0x0002)

//-----------------------------------------------------------------------------
// runtime configuration - stored in Backup Domain
// #define BKPSRAM_BASE				0x40024000
#define BKPSRAM_END					0x40024FFF
#define BKPSRAM_SIZE				(BKPSRAM_END-BKPSRAM_BASE)
#define BKPREG_MAGIC				0x426b426e	// nBkB ;)
#define BKPREG_VERSION				0x00000002

//-----------------------------------------------------------------------------
// factory configuration - stored in EEPROM
factory_config_t factory_config;

bool_t factory_config_good;

//-----------------------------------------------------------------------------
typedef struct config_t config_t;
struct config_t
{
	uint8_t 		serial1_pwr;
	int32_t 		serial1_baud;
	uint8_t 		serial2_pwr;
	int32_t 		serial2_baud;
	uint8_t 		lcd_backlight;
	int32_t			metric_units;
	int32_t			eDisplayInterval;
	int32_t			eDisplaySerialPort;
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
	config.metric_units = kbs_getMetricUnits();
	config.eDisplayInterval = kbed_getInterval();
	config.eDisplaySerialPort = kbed_getSerialPort();

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
	kbs_setMetricUnits(config.metric_units);
	kbfa_setInterval(config.eDisplayInterval);
	kbfa_setSerialPort(config.eDisplaySerialPort);

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
	uint16_t cs = calc_checksum_16((uint8_t*)(BKPSRAM_BASE), BKPSRAM_SIZE);
	return cs;
}

//-----------------------------------------------------------------------------
uint8_t 
kbc_read8(uint32_t offset)
{
	uint8_t data = 0;
	kbc_read(offset, (uint8_t*)&data, sizeof(data));
	return data;
}

//-----------------------------------------------------------------------------
uint16_t 
kbc_read16(uint32_t offset)
{
	uint16_t data = 0;
	kbc_read(offset, (uint8_t*)&data, sizeof(data));
	return data;
}

//-----------------------------------------------------------------------------
uint32_t 
kbc_read32(uint32_t offset)
{
	uint32_t data = 0;
	kbc_read(offset, (uint8_t*)&data, sizeof(data));
	return data;
}

//-----------------------------------------------------------------------------
int 
kbc_read(uint32_t offset, void * data, uint32_t length)
{
	check_offset(offset,length);
	memcpy(data, (uint8_t*)(BKPSRAM_BASE+offset), length);
	return KB_OK;
}

//-----------------------------------------------------------------------------
int 
kbc_write8(uint32_t offset, uint8_t data)
{
	return kbc_write(offset, (uint8_t*)&data, sizeof(data));
}

//-----------------------------------------------------------------------------
int 
kbc_write16(uint32_t offset, uint16_t data)
{
	return kbc_write(offset, (uint8_t*)&data, sizeof(data));
}

//-----------------------------------------------------------------------------
int 
kbc_write32(uint32_t offset, uint32_t data)
{
	return kbc_write(offset, (uint8_t*)&data, sizeof(data));
}

//-----------------------------------------------------------------------------
int 
kbc_write(uint32_t offset, void * data, uint32_t length)
{
	check_offset(offset,length);
	memcpy((uint8_t*)(BKPSRAM_BASE+offset), data, length);
	RTC->BKP2R = calc_cs();
	return KB_OK;
}

//-----------------------------------------------------------------------------
int
kuroBoxConfigInit(void)
{
	if ( ( RTC->BKP0R == BKPREG_MAGIC ) &&
		 ( RTC->BKP1R == BKPREG_VERSION ) &&
		 ( RTC->BKP2R == calc_cs() ) )
	{
		// if we wiped memory, don't load it, maybe do a save instead so
		// it gets put in memory, right?
		// @TODO: check up on this
		kbc_load();
	}
	else
	{
		// clear it all
		memset((uint8_t*)BKPSRAM_BASE, 0, BKPSRAM_SIZE);
		RTC->BKP0R = BKPREG_MAGIC;
		RTC->BKP1R = BKPREG_VERSION;
		RTC->BKP2R = calc_cs();
	}

	factory_config_good = 0;
	memset(&factory_config, 0, sizeof(factory_config));

	uint8_t * eeprombuf = (uint8_t*) &factory_config;
	spiEepromReadBytes(&spiEepromD1, FACTORY_CONFIG_ADDRESS, eeprombuf, FACTORY_CONFIG_SIZE);

	if ( factory_config.preamble == FACTORY_CONFIG_MAGIC &&
		 factory_config.version == FACTORY_CONFIG_VERSION )
	{
		uint16_t checksum = calc_checksum_16((uint8_t*)&factory_config,
				sizeof(factory_config)-sizeof(factory_config.checksum));
		if ( checksum == factory_config.checksum )
		{
			// all good!
			factory_config_good = 1;
			kbt_startOneSec(factory_config.tcxo_compensation);
		}
		else
		{
			// error?
		}
	}
	else
	{
		// @TODO: do something here if we don't have config
	}

	return KB_OK;
}

//-----------------------------------------------------------------------------
int
kuroBoxConfigStop(void)
{
	// NOTE: no need to save this out on shutdown
	//kbc_save();
	return KB_OK;
}

