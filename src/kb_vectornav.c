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
    /*

	We do a couple of strange things in this file, lets explain them.

	We communicate with the VectorNav in 2 different ways, synchronously and
	asynchronously. 

	We need to wait 50us between issuing a command and reading its answer, so 
	the sync calls ("kbv_readRegister()"), will busy wait for responses. This 
	is great for reading a one-off register, like at startup, we read the 
	model number, serial number, etc.

	The periodic data comes in every 5ms (200Hz), when the EXTi callback is
	triggered on the failling edge ("vn_dr_int_exti_cb()"). At that point 
	we start the async communication. That callback fills in the async_vn_msg_t
	with what registers need to be read and how big they are.

	Calls go like this:

	Fill async_vn_msg_t
	Call 1st spiStartSend with Read Register Request
	On 1st spiStartSend callback, unselect the SPI
	1st sleep for 50us, asychronously
	On 1st sleep callback, call 2nd spiStartSend with Read Register
	On 2nd spiStartSend callback, unselect the SPI
	Dispatch register
	2nd sleep for 50us, asynchronously
	On 2nd sleep callback, clear async_vn_msg_t to indicate we're done and
		a new async call can start

    */
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
#include "kb_vectornav.h"
#include "kb_util.h"
#include "kb_screen.h"
#include "kb_writer.h"
#include <string.h>

//-----------------------------------------------------------------------------
static void vectornav_spi_end_cb(SPIDriver * spip);
static void vectornav_gpt_end_cb(GPTDriver * spip);

//-----------------------------------------------------------------------------
static const VectorNavConfig default_vectornav_config =
{
	{	// SPIConfig struct
		vectornav_spi_end_cb,
		GPIOD,
		GPIOD_L1_VN_NSS,

		// lets put it down to 10.5MHz
		SPI_CR1_BR_0 | SPI_CR1_CPOL | SPI_CR1_CPHA
	},
	{	// GPT config
		1000000, // 1MHz should be fine enough to get a 50us timeout
		vectornav_gpt_end_cb,
		0, // CR2 register
		0  // DIER register
	}
};

//-----------------------------------------------------------------------------
#define VN_ASYNC_INACTIVE				0
#define VN_ASYNC_1ST_SPI_CB				1
#define VN_ASYNC_1ST_SLEEP				2
#define VN_ASYNC_2ND_SPI_CB				3
#define VN_ASYNC_2ND_SLEEP				4
#define VN_SLEEPTIME					50

//-----------------------------------------------------------------------------
typedef struct async_vn_msg_t async_vn_msg_t;
struct async_vn_msg_t
{
	uint8_t state;
	uint8_t reg;
	uint16_t wait;
	uint8_t buf[VN100_MAX_MSG_SIZE];
	uint16_t buf_size;
};

//-----------------------------------------------------------------------------
VectorNavDriver VND1;
static async_vn_msg_t async_vn_msg;
static vnav_data_t vnav_data;

//-----------------------------------------------------------------------------
static void 
vectornav_dispatch_register(uint8_t reg, uint16_t buf_size, uint8_t * buf)
{
	switch( reg )
	{
	case VN100_REG_YPR:
		if ( buf[0] == 0x00 && buf[1] == 0x01 &&
			 buf[2] == reg && buf[3] == 0x00 && 
			 buf_size == VN100_REG_YPR_SIZE + VN100_HEADER_SIZE)
		{
			// data return is good, proceed
			float * fbuf = (float*)(&buf[4]); //  header
			memcpy(&vnav_data.ypr, fbuf, sizeof(vnav_data.ypr));

			// screen is interested in YPR, the writer in the entire packet	
			kbs_setYPR(fbuf[0], fbuf[1], fbuf[2]);
			kbw_setVNav(&vnav_data);
		}
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
static void 
vectornav_spi_end_cb(SPIDriver * spip)
{
	(void)spip;

	switch( async_vn_msg.state )
	{
	case VN_ASYNC_1ST_SPI_CB:
		chSysLockFromISR();
		spiUnselectI(VND1.spip);
		async_vn_msg.state = VN_ASYNC_1ST_SLEEP;
		gptStartOneShotI(VND1.gpdp, VN_SLEEPTIME);
		chSysUnlockFromISR();
		break;

	case VN_ASYNC_2ND_SPI_CB:
		chSysLockFromISR();
		spiUnselectI(VND1.spip);
		// here we have the register
		vectornav_dispatch_register(async_vn_msg.reg, async_vn_msg.buf_size, async_vn_msg.buf);
		async_vn_msg.state = VN_ASYNC_2ND_SLEEP;
		gptStartOneShotI(VND1.gpdp, VN_SLEEPTIME);
		chSysUnlockFromISR();
		break;

	case VN_ASYNC_INACTIVE:
	case VN_ASYNC_1ST_SLEEP:
	case VN_ASYNC_2ND_SLEEP:
	default:
		// @TODO: assert?
		break;
	}
}

//-----------------------------------------------------------------------------
static void 
vectornav_gpt_end_cb(GPTDriver * gptp)
{
	(void)gptp;

	switch( async_vn_msg.state )
	{
	case VN_ASYNC_1ST_SLEEP:
		chSysLockFromISR();
		async_vn_msg.state = VN_ASYNC_2ND_SPI_CB;
		memset(&async_vn_msg.buf, 0, sizeof(async_vn_msg.buf));
		spiSelectI(VND1.spip);
		spiStartReceiveI(VND1.spip, async_vn_msg.buf_size, async_vn_msg.buf);
		chSysUnlockFromISR();
		break;

	case VN_ASYNC_2ND_SLEEP:
		// finished, clear it all
		memset(&async_vn_msg, 0, sizeof(async_vn_msg));
		break;

	case VN_ASYNC_INACTIVE:
	case VN_ASYNC_1ST_SPI_CB:
	case VN_ASYNC_2ND_SPI_CB:
	default:
		// @TODO: assert?
		break;
	}
}

//-----------------------------------------------------------------------------
const vnav_data_t * 
kbv_getYPR(void)
{
	return &vnav_data;
}

//-----------------------------------------------------------------------------
void
kbv_drIntExtiCB(EXTDriver *extp, expchannel_t channel)
{
	(void)extp;
	(void)channel;

	chSysLockFromISR();
	memset(&async_vn_msg, 0, sizeof(async_vn_msg));
	async_vn_msg.reg = VN100_REG_YPR;
	// ReadReg: [0x01, REG, 0x00, 0x00]
	async_vn_msg.buf[0] = 1;
	async_vn_msg.buf[1] = async_vn_msg.reg;
	async_vn_msg.buf_size = VN100_REG_YPR_SIZE + VN100_HEADER_SIZE;
	async_vn_msg.state = VN_ASYNC_1ST_SPI_CB;
	spiSelectI(VND1.spip);
	spiStartSendI(VND1.spip, 4, async_vn_msg.buf);
	///**/kbed_dataReadyI();
	chSysUnlockFromISR();
}

//-----------------------------------------------------------------------------
int
kuroBoxVectorNavInit(VectorNavDriver * nvp, const VectorNavConfig * cfg)
{
	if ( cfg )
		nvp->cfgp = cfg;
	else
		nvp->cfgp = &default_vectornav_config;

	spiStart(nvp->spip, &nvp->cfgp->spicfg);
	gptStart(nvp->gpdp, &nvp->cfgp->gptcfg);

	uint8_t header_data[VN100_REG_USER_TAG_SIZE+
	                    VN100_REG_MODEL_SIZE+
	                    VN100_REG_HWREV_SIZE+
	                    VN100_REG_SN_SIZE+
	                    VN100_REG_FWVER_SIZE];

	uint8_t * header_data_ptr = header_data;

	kbv_readRegister(nvp, VN100_REG_USER_TAG, VN100_REG_USER_TAG_SIZE, header_data_ptr);
	header_data_ptr += VN100_REG_USER_TAG_SIZE;

	kbv_readRegister(nvp, VN100_REG_MODEL, VN100_REG_MODEL_SIZE, header_data_ptr);
	header_data_ptr += VN100_REG_MODEL_SIZE;

	kbv_readRegister(nvp, VN100_REG_HWREV, VN100_REG_HWREV_SIZE, header_data_ptr);
	header_data_ptr += VN100_REG_HWREV_SIZE;

	kbv_readRegister(nvp, VN100_REG_SN, VN100_REG_SN_SIZE, header_data_ptr);
	header_data_ptr += VN100_REG_SN_SIZE;

	kbv_readRegister(nvp, VN100_REG_FWVER, VN100_REG_FWVER_SIZE, header_data_ptr);
	header_data_ptr += VN100_REG_FWVER_SIZE;

	kbwh_setVNav(header_data, header_data_ptr-header_data);

	return KB_OK;
}


//-----------------------------------------------------------------------------
int
kuroBoxVectorNavStop(VectorNavDriver * nvp)
{
	spiStop(nvp->spip);
	gptStop(nvp->gpdp);
	return KB_OK;
}

//-----------------------------------------------------------------------------
// polled read of a register
uint8_t
kbv_readRegister(VectorNavDriver * nvp, uint8_t reg, uint8_t size, uint8_t * buf)
{
	uint8_t r[] = { 0x01, reg, 0x00, 0x00 };

	chSysLock();

	spiSelectI(nvp->spip);
	// we ignore the first exchange's return
	for ( int i = 0 ; i < 4 ; ++i )
		spiPolledExchange(nvp->spip, r[i]);
	spiUnselectI(nvp->spip);

	gptPolledDelay(nvp->gpdp, 50);

	spiSelectI(nvp->spip);
	for ( int i = 0 ; i < 4 ; ++i )
		r[i] = spiPolledExchange(nvp->spip, 0);

	// was there an error?
	if ( r[0] == 0x00 && r[1] == 0x01 && r[2] == reg && r[3] == 0x00 )
	{
		// all good!
		for ( int i = 0 ; i < size ; ++i )
			buf[i] = spiPolledExchange(nvp->spip, 0);
	}
	spiUnselectI(nvp->spip);
	gptPolledDelay(nvp->gpdp, 50);
	chSysUnlock();
	return r[3];
}
