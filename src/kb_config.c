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

//-----------------------------------------------------------------------------
// #define BKPSRAM_BASE				0x40024000
#define BKPSRAM_END					0x40024FFF
#define BKPSRAM_SIZE				(BKPSRAM_END-BKPSRAM_BASE)
#define BKPREG_MAGIC				0x426b426e	// nBkB ;)

//-----------------------------------------------------------------------------
static int
check_offset(uint16_t offset, uint32_t length)
{
	if ( (offset+length) > BKPSRAM_SIZE )
		return KB_ERR_VALUE;
	return KB_OK;
}

//-----------------------------------------------------------------------------
static uint8_t
calc_cs(void)
{
	return calc_checksum_8((uint8_t*)(BKPSRAM_BASE+4), BKPSRAM_SIZE-4);
}

//-----------------------------------------------------------------------------
int kbc_read8(uint32_t offset, uint8_t * data)
{
	return kbc_read(offset, (uint8_t*)data, sizeof(*data));
}

//-----------------------------------------------------------------------------
int kbc_read16(uint32_t offset, uint16_t * data)
{
	return kbc_read(offset, (uint8_t*)data, sizeof(*data));
}

//-----------------------------------------------------------------------------
int kbc_read32(uint32_t offset, uint32_t * data)
{
	return kbc_read(offset, (uint8_t*)data, sizeof(*data));
}

//-----------------------------------------------------------------------------
int kbc_read(uint32_t offset, uint8_t * data, uint32_t length)
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
int kbc_write(uint32_t offset, uint8_t * data, uint32_t length)
{
	check_offset(offset,length);
	memcpy((uint8_t*)(BKPSRAM_BASE+offset+4), data, length);
	uint8_t cs = calc_cs();
	memcpy((uint8_t*)(BKPSRAM_BASE), &cs, 1);
	return KB_OK;
}

//-----------------------------------------------------------------------------
int kuroBoxConfigInit(void)
{
	if ( ( RTC->BKP0R != BKPREG_MAGIC ) ||
		 ( *(uint8_t*)(BKPSRAM_BASE) != calc_cs() ) )
	{
		// clear it all
		memset((uint8_t*)BKPSRAM_BASE, 0, BKPSRAM_SIZE);
		RTC->BKP0R = BKPREG_MAGIC;
		// the checksum of all 0's is 0, so nothing needed
	}

	return KB_OK;
}

//-----------------------------------------------------------------------------
int kuroBoxConfigStop(void)
{
	return KB_OK;
}

