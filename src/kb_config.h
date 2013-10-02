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
#ifndef _naniBox_kuroBox_config
#define _naniBox_kuroBox_config

//-----------------------------------------------------------------------------
#include <ch.h>
#include <hal.h>

//-----------------------------------------------------------------------------
int kbc_save(void);
int kbc_load(void);

//-----------------------------------------------------------------------------
uint8_t kbc_read8(uint32_t offset);
uint16_t kbc_read16(uint32_t offset);
uint32_t kbc_read32(uint32_t offset);
int kbc_read(uint32_t offset, void * data, uint32_t length);

//-----------------------------------------------------------------------------
int kbc_write8(uint32_t offset, uint8_t data);
int kbc_write16(uint32_t offset, uint16_t data);
int kbc_write32(uint32_t offset, uint32_t data);
int kbc_write(uint32_t offset, void * data, uint32_t length);

//-----------------------------------------------------------------------------
int kuroBoxConfigInit(void);
int kuroBoxConfigStop(void);

//-----------------------------------------------------------------------------
#endif // _naniBox_kuroBox_config
