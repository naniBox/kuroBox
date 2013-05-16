/*
 * naniBox // kuroBox // spiEEPROM
 * dmo@nanibox.com
 */

#include "spiEEPROM.h"
#include "ch.h"
#include "chprintf.h"

//----------------------------------------------------------------------------
spiEepromDriver spiEepromD1;

//----------------------------------------------------------------------------
#define OP_WREN			0x06	// Write Enable Latch
#define OP_WRDI			0x04	// Write Disable
#define OP_RDSR			0x05	// Read Status Register
#define OP_WRSR			0x01	// Write Status Register
#define OP_READ			0x03	// Read Data
#define OP_WRITE		0x02	// Write Data

//----------------------------------------------------------------------------
void 
spiEepromStart(spiEepromDriver * sedp, 
			   const spiEepromConfig * secp,
			   SPIDriver * spip)
{
	sedp->cfgp = secp;
	sedp->spip = spip;
}

//----------------------------------------------------------------------------
void
spiEepromReadPage(spiEepromDriver * sedp, uint16_t page, uint8_t * buf)
{
	uint32_t addr = page << SPIEEPROM_PAGE_SIZE_SHIFT;

	spiStart(sedp->spip, &sedp->cfgp->spicfg);
	spiSelect(sedp->spip);
	spiPolledExchange(sedp->spip, OP_READ);
#ifdef SPIEEPROM_24BIT_ADDRESS
	spiPolledExchange(sedp->spip, (addr>>16)&0xff);
#endif
	spiPolledExchange(sedp->spip, (addr>>8)&0xff);	
	spiPolledExchange(sedp->spip, addr&0xff);		
	for ( uint16_t idx = 0 ; idx < SPIEEPROM_PAGE_SIZE ; ++idx )
		*buf++ = spiPolledExchange(sedp->spip, 0);
	spiUnselect(sedp->spip);
}

//----------------------------------------------------------------------------
void
spiEepromWritePage(spiEepromDriver * sedp, uint16_t page, const uint8_t * buf)
{	
	uint32_t addr = page << SPIEEPROM_PAGE_SIZE_SHIFT;

	spiStart(sedp->spip, &sedp->cfgp->spicfg);
	spiSelect(sedp->spip);

	spiPolledExchange(sedp->spip, OP_WRITE);
#ifdef SPIEEPROM_24BIT_ADDRESS
	spiPolledExchange(sedp->spip, (addr>>16)&0xff);
#endif
	spiPolledExchange(sedp->spip, (addr>>8)&0xff);
	spiPolledExchange(sedp->spip, addr&0xff);
	for ( uint16_t idx = 0 ; idx < SPIEEPROM_PAGE_SIZE ; ++idx )
		spiPolledExchange(sedp->spip, *buf++);

	spiUnselect(sedp->spip);
}

//----------------------------------------------------------------------------
void
spiEepromEnableWrite(spiEepromDriver * sedp)
{
	spiStart(sedp->spip, &sedp->cfgp->spicfg);
	spiSelect(sedp->spip);
	spiPolledExchange(sedp->spip, OP_WREN);
	spiUnselect(sedp->spip);
}

//----------------------------------------------------------------------------
void
spiEepromDisableWrite(spiEepromDriver * sedp)
{
	spiStart(sedp->spip, &sedp->cfgp->spicfg);
	spiSelect(sedp->spip);
	spiPolledExchange(sedp->spip, OP_WRDI);
	spiUnselect(sedp->spip);
}

//----------------------------------------------------------------------------
uint8_t 
spiEepromReadSR(spiEepromDriver * sedp)
{
	spiStart(sedp->spip, &sedp->cfgp->spicfg);
	spiSelect(sedp->spip);
	spiPolledExchange(sedp->spip, OP_RDSR);
	uint8_t sr = spiPolledExchange(sedp->spip, 0x00);
	spiUnselect(sedp->spip);
	return sr;
}

//----------------------------------------------------------------------------
void 
spiEepromWriteSR(spiEepromDriver * sedp, uint8_t sr)
{
	spiStart(sedp->spip, &sedp->cfgp->spicfg);
	spiSelect(sedp->spip);
	spiPolledExchange(sedp->spip, OP_WRSR);
	spiPolledExchange(sedp->spip, sr);
	spiUnselect(sedp->spip);
}
