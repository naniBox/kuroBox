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
#include "kb_vectornav.h"
#include "kb_util.h"
#include "kb_screen.h"
#include <string.h>

static const VectorNavConfig default_vectornav_config =
{
	{	// SPIConfig struct
		NULL, //  we are going to need a callback, mkay?
		GPIOD,
		GPIOD_L1_VN_NSS,
		SPI_CR1_CPOL | SPI_CR1_CPHA
	},
	{
		1000000, // 1MHz should be fine enough to get a 50us timeout
		NULL
	}
};

//-----------------------------------------------------------------------------
VectorNavDriver VND1;

//-----------------------------------------------------------------------------
static Thread * vnThread;
static WORKING_AREA(waVN, 128);
static msg_t
thVN(void *arg)
{
	(void)arg;
	chRegSetThreadName("VN");

	float ypr[4]; // the 4th is a uint32_t, deal with it
	memset(&ypr, 0, sizeof(ypr));
	while( !chThdShouldTerminate() )
	{
		chSysLock();
		vnThread = chThdSelf();
		chSchGoSleepS(THD_STATE_SUSPENDED);
		chSysUnlock();
		kbv_readRegister(&VND1, VN100_REG_YPR, VN100_REG_YPR_SIZE+4, (uint8_t*)ypr);
		kbs_setYPR(ypr[0],ypr[1],ypr[2]);
	}
	return 0;
}

//-----------------------------------------------------------------------------
void vn_dr_int_exti_cb(EXTDriver *extp, expchannel_t channel)
{
	(void)extp;
	(void)channel;

	chSysLockFromIsr();
	if (vnThread != NULL)
	{
		vnThread->p_u.rdymsg = (msg_t)1;
		chSchReadyI(vnThread);
		vnThread = NULL;
	}
	chSysUnlockFromIsr();
}

//-----------------------------------------------------------------------------
int kuroBoxVectorNavInit(VectorNavDriver * nvp, const VectorNavConfig * cfg)
{
	if ( cfg )
		nvp->cfgp = cfg;
	else
		nvp->cfgp = &default_vectornav_config;

	spiStart(nvp->spip, &nvp->cfgp->spicfg);
	gptStart(nvp->gpdp, &nvp->cfgp->gptcfg);

	/* vnThread = */chThdCreateStatic(waVN, sizeof(waVN), NORMALPRIO, thVN, NULL);
	return KB_OK;
}

//-----------------------------------------------------------------------------
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
