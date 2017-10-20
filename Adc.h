#ifndef msoe_sae_Adc_h
#define msoe_sae_Adc_h

#define ADC_CHANNELS 16
typedef struct
{
	register8_t CSRA;
	register8_t CSRB;
	register8_t MUX;
	register16_t DR;
	
	bool doConversions[ADC_CHANNELS];
	uint16_t conversions[ADC_CHANNELS];
	uint8_t index;
} Adc_t;


void InitializeAdc(Adc_t* adc, register8_t CSRA, register8_t CSRB, register8_t MUX, register16_t DR)
{
	adc->CSRA = CSRA;
	adc->CSRB = CSRB;
	adc->MUX = MUX;
	adc->DR = DR;
	
	*adc->MUX = 1<<REFS0; // Internal VREF
	
	adc->index = 0;
	for (uint8_t x = 0; x < ADC_CHANNELS; x++)
	{
		adc->doConversions[x] = false;
	}
}


void NextChannel(Adc_t* localAdc)
{
	
	do 
	{
		localAdc->index++;
		if (localAdc->index >= ADC_CHANNELS)
		{
			localAdc->index = 0;
		}
	} while(!localAdc->doConversions[localAdc->index]);
	
	
	*localAdc->MUX &= ~0xf;
	*localAdc->MUX |= localAdc->index;
}
void EnableAdc(Adc_t* adc)
{
	
	
	*adc->CSRA |= 1 << ADPS2;
	*adc->CSRA |= 1 << ADPS1;
	*adc->CSRA |= 1 << ADPS0;
	
	*adc->CSRA |= 1 << ADEN;
	*adc->CSRA |= 1 << ADIE;
	*adc->CSRA |= 1 << ADSC;
	
	
	NextChannel(adc);
}

volatile Adc_t Adc;
ISR(ADC_vect)
{
	Adc_t* localAdc = (Adc_t*)&Adc;
	localAdc->conversions[localAdc->index] = *localAdc->DR;
	NextChannel(localAdc);

	
	*localAdc->CSRA |= 1<<ADSC;
}


#endif

