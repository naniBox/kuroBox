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

//----------------------------------------------------------------------------
#ifndef _naniBox_kuroBox_vectornav
#define _naniBox_kuroBox_vectornav

//----------------------------------------------------------------------------
#include <ch.h>
#include <hal.h>

//----------------------------------------------------------------------------
// @TODO: Move to the .c?
#define     VN100_REG_USER_TAG  0
#define     VN100_REG_MODEL     1
#define     VN100_REG_HWREV     2
#define     VN100_REG_SN        3
#define     VN100_REG_FWVER     4
#define     VN100_REG_SBAUD     5
#define     VN100_REG_ADOR      6
#define     VN100_REG_ADOF      7
#define     VN100_REG_YPR       8
#define     VN100_REG_QTN       9
#define     VN100_REG_QTM       10
#define     VN100_REG_QTA       11
#define     VN100_REG_QTR       12
#define     VN100_REG_QMA       13
#define     VN100_REG_QAR       14
#define     VN100_REG_QMR       15
#define     VN100_REG_DCM       16
#define     VN100_REG_MAG       17
#define     VN100_REG_ACC       18
#define     VN100_REG_GYR       19
#define     VN100_REG_MAR       20
#define     VN100_REG_REF       21
#define     VN100_REG_SIG       22
#define     VN100_REG_HSI       23
#define     VN100_REG_ATP       24
#define     VN100_REG_ACT       25
#define     VN100_REG_RFR       26
#define     VN100_REG_YMR       27
#define     VN100_REG_ACG       28

#define     VN100_REG_RAW       251
#define     VN100_REG_CMV       252
#define     VN100_REG_STV       253
#define     VN100_REG_COV       254
#define     VN100_REG_CAL       255

#define 	VN100_HEADER_SIZE	4

#define     VN100_REG_USER_TAG_SIZE  	20
#define     VN100_REG_MODEL_SIZE     	24
#define     VN100_REG_HWREV_SIZE     	4
#define     VN100_REG_SN_SIZE        	12
#define     VN100_REG_FWVER_SIZE     	4
										// 64 bytes for all that data

#define     VN100_REG_YPR_SIZE       	(3*4)

#define     VN100_REG_RAW_SIZE       	(10*4)
#define     VN100_REG_CMV_SIZE       	(10*4)

#define		VN100_MAX_MSG_SIZE			1024

//----------------------------------------------------------------------------
typedef struct VectorNavConfig VectorNavConfig;
struct VectorNavConfig
{
	SPIConfig spicfg;
	GPTConfig gptcfg;
};

//----------------------------------------------------------------------------
typedef struct VectorNavDriver VectorNavDriver;
struct VectorNavDriver
{
	const VectorNavConfig * cfgp;
	SPIDriver * spip;
	GPTDriver * gpdp;
};

//-----------------------------------------------------------------------------
typedef struct vnav_data_t vnav_data_t;
struct vnav_data_t
{
	float ypr[3];
	uint32_t ypr_ts;
};

//-----------------------------------------------------------------------------
extern VectorNavDriver VND1;

//-----------------------------------------------------------------------------
const vnav_data_t * kbv_getYPR(void);

//-----------------------------------------------------------------------------
void kbv_drIntExtiCB(EXTDriver *extp, expchannel_t channel);
int kuroBoxVectorNavInit(VectorNavDriver * nvp, const VectorNavConfig * cfg);
int kuroBoxVectorNavStop(VectorNavDriver * nvp);

//-----------------------------------------------------------------------------
uint8_t kbv_readRegister(VectorNavDriver * nvp, uint8_t reg, uint8_t size, uint8_t * buf);

#endif // _naniBox_kuroBox_vectornav
