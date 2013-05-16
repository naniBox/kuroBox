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

#include "callbacks.h"
#include "screen.h"

/* Total number of channels to be sampled by a single ADC operation.*/
#define ADC_NUM_CHANNELS   3

/* Depth of the conversion buffer, channels are sampled four times each.*/
#define ADC_BUF_DEPTH      2

void adc_cb(ADCDriver *adcp, adcsample_t *buffer, size_t n);
static adcsample_t adc_samples[ADC_NUM_CHANNELS * ADC_BUF_DEPTH];

//-----------------------------------------------------------------------------
static const ADCConversionGroup adc_grp_cfg = 
{
	FALSE,						// circular
	ADC_NUM_CHANNELS,		// num channels
	adc_cb,						// completetion callback
	NULL,						// error callback
	/* HW dependent part.*/
	0,
	ADC_CR2_SWSTART,
	ADC_SMPR1_SMP_AN10(ADC_SAMPLE_56) | 
	ADC_SMPR1_SMP_SENSOR(ADC_SAMPLE_56) | 
	ADC_SMPR1_SMP_VBAT(ADC_SAMPLE_56),
	0,
	ADC_SQR1_NUM_CH(ADC_NUM_CHANNELS),
	0,
	ADC_SQR3_SQ1_N(ADC_CHANNEL_IN10) | 
	ADC_SQR3_SQ2_N(ADC_CHANNEL_SENSOR) | 
	ADC_SQR3_SQ3_N(ADC_CHANNEL_VBAT)
};

//-----------------------------------------------------------------------------
adcsample_t adc_make_avg(int channel)
{
	int avg = 0;
	for ( int idx = 0 ; idx < ADC_BUF_DEPTH ; idx++ )
	{
		avg += adc_samples[idx*ADC_NUM_CHANNELS+channel];
	}
	return avg/ADC_BUF_DEPTH;
}

//-----------------------------------------------------------------------------
void adc_cb(ADCDriver *adcp, adcsample_t *buffer, size_t n)
{
	(void) buffer; (void) n;
	/* Note, only in the ADC_COMPLETE state because the ADC driver fires an
	 intermediate callback when the buffer is half full.*/
	if (adcp->state == ADC_COMPLETE) 
	{
		adcsample_t voltage = adc_make_avg(0);
		kbs_setVoltage((voltage-100)/10);
		// don't care about these yet 
		// @TODO: remove them from actual ADC capture?
		//adcsample_t temperature = adc_make_avg(1);
		//adcsample_t vbat= adc_make_avg(2);
	}
}

//-----------------------------------------------------------------------------
void adc_trigger_cb(void * p)
{
	VirtualTimer * vt = (VirtualTimer *)p;
	chSysLockFromIsr();
	adcStartConversionI(&ADCD1, &adc_grp_cfg, adc_samples, ADC_BUF_DEPTH);
	chVTSetI(vt, MS2ST(200), adc_trigger_cb, vt);
	chSysUnlockFromIsr();
}
