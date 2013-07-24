/*
	kuroBox / naniBox
	Copyright (c) 2013
	david morris-oliveros // dmo@nanibox.com // naniBox.com

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
#include <time.h>
#include "spiEEPROM.h"
#include "ST7565.h"

#include "kb_adc.h"
#include "kb_buttons.h"
#include "kb_config.h"
#include "kb_gpio.h"
#include "kb_gps.h"
#include "kb_logger.h"
#include "kb_menu.h"
#include "kb_screen.h"
#include "kb_serial.h"
#include "kb_time.h"
#include "kb_vectornav.h"

//-----------------------------------------------------------------------------
// types and stuff
#define GS_INIT 		0
#define GS_WAIT_ON_SD	1
#define GS_RUNNING		2
#define GS_ERROR		3
typedef uint32_t kuroBoxState_t;

//-----------------------------------------------------------------------------
// forward declarations
int kuroBoxStop(void);

//-----------------------------------------------------------------------------
// static data
static uint8_t lcd_buffer[ST7565_BUFFER_SIZE];
/*static Thread * blinkerThread;*/
static kuroBoxState_t global_state;
extern bool_t kuroBox_request_standby;

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
static const EXTConfig extcfg = {
  {
	{EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOA, btn_0_exti_cb},	// 0
	{EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOA, btn_1_exti_cb},	// 1

    {EXT_CH_MODE_DISABLED, NULL},	// 2
	{EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOC, gps_timepulse_exti_cb},	// 3
    {EXT_CH_MODE_DISABLED, NULL},	// 4
    {EXT_CH_MODE_DISABLED, NULL},	// 5
    {EXT_CH_MODE_DISABLED, NULL},	// 6
    {EXT_CH_MODE_DISABLED, NULL},	// 7
    {EXT_CH_MODE_DISABLED, NULL},	// 8
	{EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOA, ltc_exti_cb},	// 9
    {EXT_CH_MODE_DISABLED, NULL},	// 10
	{EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOE, vn_dr_int_exti_cb},	// 11
    {EXT_CH_MODE_DISABLED, NULL},	// 12
    {EXT_CH_MODE_DISABLED, NULL},	// 13
    {EXT_CH_MODE_DISABLED, NULL},	// 14
    {EXT_CH_MODE_DISABLED, NULL},	// 15
    {EXT_CH_MODE_DISABLED, NULL},	// 16
    {EXT_CH_MODE_DISABLED, NULL},	// 17
    {EXT_CH_MODE_DISABLED, NULL},	// 18
    {EXT_CH_MODE_DISABLED, NULL},	// 19
    {EXT_CH_MODE_DISABLED, NULL},	// 20
    {EXT_CH_MODE_DISABLED, NULL},	// 21
    {EXT_CH_MODE_DISABLED, NULL}	// 22
  }
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
		//palTogglePad(GPIOD, GPIOD_SERIAL1_PWR);
		chThdSleepMilliseconds(1000);
	}
	return 0;
}

//-----------------------------------------------------------------------------
void
kuroBox_panic(int msg)
{
	return;// this function is doing more harm than good...
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
static void
kuroBox_standby(void)
{
	kuroBoxStop();

#if _DEBUG
	DBGMCU->CR &= ~DBGMCU_CR_DBG_STANDBY;
#endif

	kuroBox_request_standby = 0;
	palSetPad(GPIOA, GPIOA_LED3);
	RTC->BKP0R = 0;
	RCC->APB2LPENR = 0;
	RCC->APB1LPENR = 0;

	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
	PWR->CR |= (PWR_CR_PDDS | PWR_CR_LPDS | PWR_CR_CSBF | PWR_CR_CWUF);
	PWR->CSR |= PWR_CSR_EWUP;
	PWR->CSR &= ~PWR_CSR_WUF;
	__WFI();
}

//-----------------------------------------------------------------------------
int
kuroBoxInit(void)
{
	halInit();
	chSysInit();

	PWR->CSR &= ~PWR_CSR_EWUP;

	palSetPad(GPIOB, GPIOB_LED1);
	palSetPad(GPIOB, GPIOB_LED2);
	palSetPad(GPIOA, GPIOA_LED3);

	// Serial
	kuroBoxSerialInit(NULL, NULL);
	chprintf(DEBG, "%s\n\r\n\r", BOARD_NAME);

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
	
	// SDIO 
	sdcStart(&SDCD1, &sdio_cfg);
	
	// just blink to indicate we haven't crashed
	/*blinkerThread = */chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, thBlinker, NULL);

	// read config goes here
	// @TODO: add config reading from eeprom
	
	// start all the ADC stuff!
	kuroBoxADCInit();
	
	// init the screen, this will spawn a thread to keep it updated
	kuroBoxScreenInit();

	// gps uart
	kuroBoxGPSInit();

	// LTC's, interrupt driven, very quick now
	kuroBoxTimeInit();
	
	VND1.spip = &SPID2;
	VND1.gpdp = &GPTD14;
	kuroBoxVectorNavInit(&VND1, NULL); // use the defaults

	// the actual logging thread
	kuroBoxLoggerInit();

	kuroBoxMenuInit();

	// set initial button state, AFTER menus!
	kuroBoxButtonsInit();

	kuroBoxConfigInit();

	// indicate we're ready
	chThdSleepMilliseconds(100);
	palClearPad(GPIOB, GPIOB_LED1);
	palClearPad(GPIOB, GPIOB_LED2);
	palClearPad(GPIOA, GPIOA_LED3);

	// all external interrupts, all the system should now be ready for it
	extStart(&EXTD1, &extcfg);

	return KB_OK; 
}

//-----------------------------------------------------------------------------
int
kuroBoxStop(void)
{
	extStop(&EXTD1);

	kuroBoxConfigStop();
	kuroBoxMenuStop();
	kuroBoxLoggerStop();
	kuroBoxVectorNavStop(&VND1);
	kuroBoxTimeStop();
	kuroBoxGPSStop();
	kuroBoxButtonsStop();
	kuroBoxScreenStop();
	kuroBoxADCStop();
	sdcStop(&SDCD1);
	spiStop(&SPID1);
	adcStop(&ADCD1);
	kuroBoxSerialStop();
	chSysDisable();

	return KB_OK;
}

//-----------------------------------------------------------------------------
int main(void) 
{

	/*
		struct tm timp;
		timp.tm_sec = 0;
		timp.tm_min = 18;
		timp.tm_hour = 0;
		timp.tm_mday = 25; // -1
		timp.tm_mon = 5;
		timp.tm_year = 113;
		rtcSetTimeTm(&RTCD1, &timp);
	 */

	if ( KB_OK != kuroBoxInit() )
		global_state = GS_ERROR;
	else
		global_state = GS_WAIT_ON_SD;

	while( 1 )
	{
		chThdSleepMilliseconds(100);
		if ( kuroBox_request_standby )
			kuroBox_standby();
	}
}

