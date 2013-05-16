/*
 * naniBox // kuroBox // spiEEPROM
 * dmo@nanibox.com
 * example at the bottom of the file
 */

#ifndef _naniBox_kuroBox_spiEEPROM
#define _naniBox_kuroBox_spiEEPROM

#include "ch.h"
#include "hal.h"

#define SPIEEPROM_24BIT_ADDRESS

#ifdef SPIEEPROM_16BIT_ADDRESS
	#define SPIEEPROM_PAGE_SIZE			32
	#define SPIEEPROM_PAGE_SIZE_SHIFT 	5
	#define SPIEEPROM_NUMBER_PAGES		128
#endif
#ifdef SPIEEPROM_24BIT_ADDRESS
	#define SPIEEPROM_PAGE_SIZE			256
	#define SPIEEPROM_PAGE_SIZE_SHIFT 	8
	#define SPIEEPROM_NUMBER_PAGES		4096
#endif
#ifndef SPIEEPROM_PAGE_SIZE
	#error No defined address space
#endif

#define SR_SRWD			(1<<7)	// Status Register Write Protect
#define SR_BP1			(1<<3)	// Block Protect 1
#define SR_BP0			(1<<2)	// Block Protect 0
#define SR_WEL			(1<<1)	// Write Enable Latched
#define SR_WIP			(1<<0)	// Write In Progress

//----------------------------------------------------------------------------
typedef struct spiEepromConfig spiEepromConfig;
struct spiEepromConfig
{
	SPIConfig spicfg;
};

//----------------------------------------------------------------------------
typedef struct spiEepromDriver spiEepromDriver;
struct spiEepromDriver
{
	const spiEepromConfig * cfgp;
	SPIDriver * spip;
};

//----------------------------------------------------------------------------
extern spiEepromDriver spiEepromD1;

//----------------------------------------------------------------------------
void spiEepromStart(spiEepromDriver * sedp, 
					const spiEepromConfig * secp,
					SPIDriver * spip);

void spiEepromReadPage(spiEepromDriver * sedp, uint16_t page, uint8_t * buf);
void spiEepromWritePage(spiEepromDriver * sedp, uint16_t page, const uint8_t * buf);

void spiEepromEnableWrite(spiEepromDriver * sedp);
void spiEepromDisableWrite(spiEepromDriver * sedp);

uint8_t spiEepromReadSR(spiEepromDriver * sedp);
void spiEepromWriteSR(spiEepromDriver * sedp, uint8_t sr);

#define spiEepromWIP(sedp)	(spiEepromReadSR(sedp)&SR_WIP)
#define spiEepromWEL(sedp)	(spiEepromReadSR(sedp)&SR_WEL)

#endif // _naniBox_kuroBox_spiEEPROM

/*
	Usage example:

	uint8_t eeprombuf[SPIEEPROM_PAGE_SIZE];
	while( spiEepromWip(&spiEepromD1) )
	{}
	spiEepromReadPage(&spiEepromD1, 0, eeprombuf);
	for ( uint8_t idx = 0 ; idx < SPIEEPROM_PAGE_SIZE ; ++idx )
		chprintf((BaseSequentialStream *)&SD2, "Byte: %2d : 0x%02x\n\r", idx, eeprombuf[idx]);

	for ( uint8_t idx = 0 ; idx < SPIEEPROM_PAGE_SIZE ; ++idx )
		eeprombuf[idx] += idx;
	spiEepromWritePage(&spiEepromD1, 0, eeprombuf);

	volatile uint32_t count = 0;
	while( spiEepromWip(&spiEepromD1) )
	{count++;}
	chprintf((BaseSequentialStream *)&SD2, "Count: %d\n\r", count);

	spiEepromReadPage(&spiEepromD1, 0, eeprombuf);
	for ( uint8_t idx = 0 ; idx < SPIEEPROM_PAGE_SIZE ; ++idx )
		chprintf((BaseSequentialStream *)&SD2, "Byte: %2d : 0x%02x\n\r", idx, eeprombuf[idx]);

*/