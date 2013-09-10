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
#include "kb_featureA.h"
#include "kb_gpio.h"
#include "kb_gps.h"
#include "kb_menu.h"
#include "kb_screen.h"
#include "kb_serial.h"
#include "kb_time.h"
#include "kb_vectornav.h"
#include "kb_writer.h"

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
static Thread * blinkerThread;
static kuroBoxState_t global_state;
extern bool_t kuroBox_request_standby;

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

// I2C interface #2, this lives on Layer1's expansion board
static const I2CConfig i2cfg2 = {
    OPMODE_I2C,
    400000,
    FAST_DUTY_CYCLE_2,
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
		//palTogglePad(GPIOB, GPIOB_LED1);
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

	kuroBox_request_standby = 0;

#define DO_FAKE_STANDBY
#ifdef DO_FAKE_STANDBY
	while ( !kbg_getBtn0() )
		;

	__DSB();                                                     /* Ensure all outstanding memory accesses included
																  buffered write are completed before reset */
	SCB->AIRCR  = ((0x5FA << SCB_AIRCR_VECTKEY_Pos)      |
				 (SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) |
				 SCB_AIRCR_SYSRESETREQ_Msk);                   /* Keep priority group unchanged */
	__DSB();                                                     /* Ensure completion of memory access */
	while(1)
	  ;
#else

#if _DEBUG
	DBGMCU->CR |= DBGMCU_CR_DBG_STANDBY;
#endif

		//PWR->CSR &= ~PWR_CSR_WUF;
		PWR->CSR |= PWR_CSR_EWUP;

		// Clear Wakeup flag
		PWR->CR |= PWR_CR_CWUF;

		// Select STANDBY mode
		PWR->CR |= PWR_CR_PDDS;

		// Set SLEEPDEEP bit of Cortex System Control Register
		SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

		// This option is used to ensure that store operations are completed
		#if defined ( __CC_ARM   )
		__force_stores();
		#endif
		// Request Wait For Interrupt
		__WFI();

#endif

}

//-----------------------------------------------------------------------------
int
kuroBoxInit(void)
{
	halInit();
	chSysInit();

	PWR->CSR &= ~PWR_CSR_EWUP;

	kbg_setLED1(1);
	kbg_setLED2(1);
	kbg_setLED3(1);

	// Serial
	kuroBoxSerialInit(NULL, NULL);

	// ACD to monitor voltage levels.

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
	blinkerThread = chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, thBlinker, NULL);

	// read config goes here
	// @TODO: add config reading from eeprom
	
	// start all the ADC stuff!
	kuroBoxADCInit();
	
	// init the screen, this will spawn a thread to keep it updated
	kuroBoxScreenInit();

	// LTC's, interrupt driven, very quick now
	kuroBoxTimeInit();

	// the actual logging thread
	kuroBoxWriterInit();

	// this turns on Layer 1 power, this turns on the mosfets controlling the
	// VCC rail. After this, we can start GPS, VectorNav and altimeter
	kbg_setL1PowerOn();
	// wait for it to stabilise before continuing
	chThdSleepMilliseconds(500);

	// gps uart
	kuroBoxGPSInit();

	VND1.spip = &SPID2;
	VND1.gpdp = &GPTD14;
	kuroBoxVectorNavInit(&VND1, NULL); // use the defaults

	// start I2C here, since it's potentially a shared bus
	i2cStart(&I2CD2, &i2cfg2);

	// and the altimeter, also spawns a thread.
	kuroBoxAltimeterInit();

	kuroBoxMenuInit();

	// set initial button state, AFTER menus!
	kuroBoxButtonsInit();

	kuroBoxConfigInit();

	kuroBoxFeatureAInit();

	// indicate we're ready
	chprintf(DEBG, "%s\n\r\n\r", BOARD_NAME);
	chThdSleepMilliseconds(100);
	kbg_setLED1(0);
	kbg_setLED2(0);
	kbg_setLED3(0);

	// all external interrupts, all the system should now be ready for it
	extStart(&EXTD1, &extcfg);

	return KB_OK; 
}

//-----------------------------------------------------------------------------
int
kuroBoxStop(void)
{
	kbg_setLED3(1);

	extStop(&EXTD1);

	kuroBoxFeatureAStop();

	kuroBoxConfigStop();
	kuroBoxMenuStop();
	kuroBoxWriterStop();
	kuroBoxVectorNavStop(&VND1);
	kuroBoxTimeStop();
	kuroBoxGPSStop();
	kuroBoxButtonsStop();
	kuroBoxScreenStop();
	kuroBoxADCStop();
	chThdTerminate(blinkerThread);
	chThdWait(blinkerThread);
	sdcStop(&SDCD1);
	spiStop(&SPID1);

	kuroBoxSerialStop();
	chSysDisable();

	kbg_setLED1(0);
	kbg_setLED2(0);
	kbg_setLED3(0);

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
		{
			kuroBox_standby();
			break;
		}
	}
	while( 1 )
		;
}

