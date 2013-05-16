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

#include <string.h>
#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <memstreams.h>
#include "spiEEPROM.h"
#include "ST7565.h"
#include "fatfsWrapper.h"
#include "screen.h"
#include "callbacks.h"

//-----------------------------------------------------------------------------
// types and stuff
#define GS_INIT 		0
#define GS_WAIT_ON_SD	1
#define GS_RUNNING		2
#define GS_ERROR		3
typedef uint32_t kuroBoxState_t;

//-----------------------------------------------------------------------------
// forward declarations

//-----------------------------------------------------------------------------
// static data
static uint8_t lcd_buffer[ST7565_BUFFER_SIZE];
/*static Thread * blinkerThread;*/
static kuroBoxState_t global_state;
static VirtualTimer adc_trigger;

//-----------------------------------------------------------------------------
static ICUConfig ltc_icucfg = {
	ICU_INPUT_ACTIVE_HIGH,
	150000,
	NULL,						// width
	ltc_icu_period_cb,			// period
	NULL,						// overflow
	ICU_CHANNEL_2
};

//-----------------------------------------------------------------------------
static SerialConfig serial1_cfg = {
	SERIAL_DEFAULT_BITRATE,
	0,
	USART_CR2_STOP1_BITS | USART_CR2_LINEN,
	0
};

//-----------------------------------------------------------------------------
static SerialConfig serial2_cfg = {
	SERIAL_DEFAULT_BITRATE,
	0,
	USART_CR2_STOP1_BITS | USART_CR2_LINEN,
	0
};

//-----------------------------------------------------------------------------
static ADCConfig adc_cfg = {
	0
};

//-----------------------------------------------------------------------------
static const SDCConfig sdio_cfg = {
	0
};

//-----------------------------------------------------------------------------
static const spiEepromConfig eeprom_cfg =
{
	{	// SPIConfig struct
		NULL,			// callback
		GPIOE,
		GPIOE_EEPROM_NSS,
		SPI_CR1_BR_2 | SPI_CR1_CPOL | SPI_CR1_CPHA
	}
};

//-----------------------------------------------------------------------------
static const ST7565Config lcd_cfg = 
{
	{	// SPIConfig struct
		NULL,
		GPIOE,
		GPIOE_LCD_NSS,
		SPI_CR1_CPOL | SPI_CR1_CPHA
	},
	{ GPIOE, GPIOE_LCD_A0 },		// A0 pin
	{ GPIOE, GPIOE_LCD_RST }		// RST pin
};

//-----------------------------------------------------------------------------
static WORKING_AREA(waBlinker, 128);
static msg_t 
thBlinker(void *arg) 
{
	(void)arg;
	chRegSetThreadName("Blinker");
	while( !chThdShouldTerminate() )
	{
		palTogglePad(GPIOB, GPIOB_LED1);
		chThdSleepMilliseconds(1000);
	}
	return 0;
}

//-----------------------------------------------------------------------------
int kuroBox_panic(panic_msg_t msg)
{
	switch( msg )
	{
	case unknown_panic:
	default:
		{
			while(1)
			{
				palTogglePad(GPIOA, GPIOA_LED3);
				chThdSleepMilliseconds(50);
			}			
		}
	case no_panic:
		{
			while(1)
			{
				palTogglePad(GPIOA, GPIOA_LED3);
				chThdSleepMilliseconds(1000);
			}			
		}
	}
}

//-----------------------------------------------------------------------------
int
kuroBoxInit()
{
	chDbgAssert(GS_INIT == global_state, "kuroBoxInit, 1", "global_state is not GS_INIT");

	// Serial
	sdStart(&SD1, &serial1_cfg);
	sdStart(&SD2, &serial2_cfg);

	BaseSequentialStream * prnt = (BaseSequentialStream *)&SD1;
	chprintf(prnt, "%s\n\r\n\r", BOARD_NAME);

	// ACD to monitor voltage levels.
	adcStart(&ADCD1, &adc_cfg);
	adcSTM32EnableTSVREFE();
	adcSTM32EnableVBATE();

	// start the SPI bus, just use the LCD's SPI config since
	// it's shared with the eeprom
	spiStart(&SPID1, &lcd_cfg.spicfg);
	memset(lcd_buffer, 0, sizeof(lcd_buffer));
	st7565Start(&ST7565D1, &lcd_cfg, &SPID1, lcd_buffer);
	
	// EEPROM
	spiEepromStart(&spiEepromD1, &eeprom_cfg, &SPID1);
	
	// LTC's ICU stage
	icuStart(&ICUD1, &ltc_icucfg);
	icuEnable(&ICUD1);
	
	// SDIO 
	sdcStart(&SDCD1, &sdio_cfg);
	
	// just blink to indicate we haven't crashed
	/*blinkerThread = */chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, thBlinker, NULL);

	// start the FatFS wrapper
	wf_init (HIGHPRIO);	
	
	// read config goes here
	// @TODO: add config reading from eeprom
	
	// this will trigger the callback 
	chSysLock();
	chVTSetI(&adc_trigger, MS2ST(200), adc_trigger_cb, &adc_trigger);
	chSysUnlock();
	
	// init the screen, this will spawn a thread to keep it updated
	kuroBoxScreenInit();
	
	return KB_OK; 
}

//-----------------------------------------------------------------------------
int main(void) 
{
	global_state = GS_INIT;
	halInit();
	chSysInit();
	if ( KB_OK != kuroBoxInit() )
		global_state = GS_ERROR;
	else
		global_state = GS_WAIT_ON_SD;

	while( 1 )
	{
		chThdSleepMilliseconds(1000);
	}
}

