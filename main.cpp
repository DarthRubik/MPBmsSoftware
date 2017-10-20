#define __AVR_ATmega328P__

#define F_CPU 16000000UL
#pragma device atmega328p
#pragma executable ihex
#pragma output Output
#pragma platform avr
#pragma optimization 0



void debug(unsigned char data);
#include "avr/io.h"
#include "avr/delay.h"
#include "stddef.h"
#include "avr/interrupt.h"



#define CHG_LED_DDR  DDRD 
#define CHG_LED_PORT PORTD
#define CHG_LED_MASK (1<<7)
#define OK_LED_DDR   DDRD
#define OK_LED_PORT  PORTD
#define OK_LED_MASK  (1<<6)
#define FAN_DDR DDRB
#define FAN_PORT PORTB
#define FAN_MASK (1<<1)


#define HIGH_CHARGING_VOLTAGE 41000
#define LOW_CHARGING_VOLTAGE  40900

#define HIGH_RES_CURRENT_SENSOR_ADC 0
#define  LOW_RES_CURRENT_SENSOR_ADC 1

#define BATTERY_VOLTAGE_HIGH_CUTOFF 42000
#define BATTERY_VOLTAGE_LOW_CUTOFF  30000


#define BATTERY_TEMP_MAX 297


#define BATTERY_TEMP_FAN_ON 326
#define BATTERY_TEMP_FAN_OFF 344


int16_t debugData;

/*
	So we have two current sensors....a low res one and a high res one
	The 0 to 1023 on the high res one is -75 to 75 Amps, while the same
	range on the low res one is -750 to 750 Amps.
	
	We will store the current as a value between -5120 and 5110, so that the
	low one can be used when appropriate, and vice versa
	
	mh = 75amps/512adc
	ml = 750amps/512adc
	
	H = 512 * A / 75
*/

// 600 Amps
#define CURRENT_HARD_CUT_OFF 4096

// 400 Amps
#define CURRENT_SOFT_CUT_OFF 2731

//5 seconds
#define CURRENT_SOFT_TIME_OUT 5000

#include "typedefs.h"
#include "spi.h"
#include "BmsBoard.h"
#include "Timer.h"
#include "Serial.h"

#include "Adc.h"
#include "stdlib.h"


Serial_t serial;


void debug(uint8_t data)
{
	DDRC |= 0x1F;
	
	
	PORTC |= 0x10;
	PORTC = (0xF0 & PORTC) | (data&0x0F); //Bottom half
	_delay_ms(1000);
	PORTC &= ~0x10;
	PORTC = (0xF0 & PORTC) | ((data>>4)&0x0F); //Upper half
	_delay_ms(1000);
}
bool IsAnyDischarging(uint8_t x,uint8_t y);
bool IsAnyDischargingAtAll();


volatile BmsChain_t chain;
volatile uint16_t ms_count = 0;
//Serial_t serial;
uint16_t lowChargingVoltage = 0;


uint16_t int_count = 0;
//This is the 10 millisecond interrupt
ISR(TIMER0_COMPA_vect){
	ms_count += 10;
	int_count++;
	if (int_count >= 100)
	{
		int_count = 0;
	}
	BmsChain_t* localChain = (BmsChain_t*)&chain;
	
	
	if (CHG_LED_PORT & CHG_LED_MASK)
	{
			
		for (uint8_t x = 0; x < BMS_BOARD_COUNT; x++)
		{
			for (uint8_t y = 0; y < BMS_CELL_COUNT; y++)
			{
				if (localChain->boards[x].cells[y].DischargeFlag)
				{
					CHG_LED_PORT &= ~CHG_LED_MASK;
				}
			}
		}
	}
	
	#define DEBUG
	#ifdef DEBUG
	
	for (uint8_t x = 0; x < BMS_BOARD_COUNT; x++)
	{
		for (uint8_t y = 0; y < BMS_CELL_COUNT; y++)
		{
			if (CHG_LED_PORT & CHG_LED_MASK)
			{
				if (rand() % 100 < 40)
				{
					localChain->boards[x].cells[y].LastReading+=rand()%100;
				}
			}
			else
			{
				if (localChain->boards[x].cells[y].DischargeFlag)
				{
					if (rand() % 100 < 40)
					{
						localChain->boards[x].cells[y].LastReading-=rand()%100;
					}
				}
			}
		}
	}
	
	#else
	
	
	
	for (uint8_t x = 0; x < BMS_BOARD_COUNT; x++)
	{
		uint16_t Discharge = 0;
		for (uint8_t y = 0; y < BMS_CELL_COUNT; y++)
		{
			
			if (localChain->boards[x].cells[y].DischargeFlag && int_count > 1)
			{
				Discharge |= 1 << y;
			}
			if (Discharge != localChain->boards[x].config.DischargeCell)
			{
				localChain->boards[x].config.DischargeCell = Discharge;
				localChain->updateConfig = true;
			}
		}
	}
			
	if (int_count == 1 || !(IsAnyDischargingAtAll()))
	{
		ChainUpdateCellReadings(localChain);
		//while (true) { OK_LED_PORT ^= OK_LED_MASK; _delay_ms(1000);}
	}
	
	ChainUpdateGpioReadings(localChain);
	
	if (localChain->updateConfig)
	{
		localChain->updateConfig = false;
		ChainFlushConfig(localChain);
	}
	
	#endif
}

void numberToString(uint16_t number, char* ptr, uint8_t dp)
{
	if (number == 0)
	{
		ptr[0] = '0';
		ptr[1] = 0;
		return;
	}
	uint16_t div = 10000;
	uint8_t x = 6;
	bool leadingZeros = false;
	while (div != 0)
	{
		uint8_t digit = number / div;
		char temp = digit + '0';
		if (leadingZeros || temp != '0')
		{
			*ptr = temp;
			ptr++;
			leadingZeros = true;
		}
		number %= div;
		div /= 10;
		if (x == dp)
		{
			*ptr='.';
			ptr++;
		}
		x--;
	}
	*ptr = 0;
}

int16_t GetCurrentFlow()
{
	
	return debugData;
	
	int16_t lowRes;
	int16_t highRes;
	cli();
	lowRes = Adc.conversions[LOW_RES_CURRENT_SENSOR_ADC];
	highRes = Adc.conversions[HIGH_RES_CURRENT_SENSOR_ADC];
	sei();
	
	lowRes -= 511;
	lowRes *= 10;
	
	if (lowRes >= 500)
	{
		return lowRes;
	}
	
	highRes -= 511;
	return highRes;
	
	
	
	
}
bool IsAnyDischargingAtAll(void)
{
	for (uint8_t x = 0; x < BMS_BOARD_COUNT; x++)  //Go through each cell and find the highest
	{
		for (uint8_t y = 0; y < BMS_CELL_COUNT; y++)
		{
			if (!chain.boards[x].cells[y].IgnoreFlag){
				bool ret = false;
				cli();
				if (chain.boards[x].cells[y].DischargeFlag)
				{
					ret = true;
				}
				sei();
				
				if (ret)
					return true;
			}
		}
	}
	return false;
}
bool IsAnyDischarging(uint8_t boardIndex,uint8_t cellindex)
{
	bool odd = chain.boards[boardIndex].cells[cellindex].OddFlag;
	for (uint8_t x = 0; x < BMS_BOARD_COUNT; x++)  //Go through each cell and find the highest
	{
		for (uint8_t y = 0; y < BMS_CELL_COUNT; y++)
		{
			if (!chain.boards[x].cells[y].IgnoreFlag && odd != chain.boards[x].cells[y].OddFlag){
				bool ret = false;
				cli();
				if (chain.boards[x].cells[y].DischargeFlag)
				{
					ret = true;
				}
				sei();
				
				if (ret)
					return true;
			}
		}
	}
	return false;
}
void BalanceBmsCells()
{

	uint16_t highestVoltage = 0;
	uint8_t highestBoard;
	uint8_t highestCell;
	uint8_t allIn = true;
	uint16_t highestDischargeVoltage = 0;
	if (!IsAnyDischargingAtAll())
		lowChargingVoltage = 0xFFFF;
	
	
	for (uint8_t x = 0; x < BMS_BOARD_COUNT; x++)  //Go through each cell and find the highest
	{
		for (uint8_t y = 0; y < BMS_CELL_COUNT; y++)
		{
			if (!chain.boards[x].cells[y].IgnoreFlag){
				cli();
				uint16_t reading = chain.boards[x].cells[y].LastReading;
				bool discharging = chain.boards[x].cells[y].DischargeFlag;
				sei();
				
				if (reading >= highestVoltage)
				{
					highestVoltage = reading;
					highestBoard = x;
					highestCell = y;
				}
				if (reading >= highestDischargeVoltage && discharging)
				{
					highestDischargeVoltage = reading;
				}
				if (reading < lowChargingVoltage && !(IsAnyDischargingAtAll()))
				{
					lowChargingVoltage = reading;
				}
				if ((reading <= LOW_CHARGING_VOLTAGE || reading >= HIGH_CHARGING_VOLTAGE))
				{
					allIn = false;
				}
				allIn = false;
				
			}
			
		}
	}
	debugData = lowChargingVoltage;
	if (highestVoltage >= HIGH_CHARGING_VOLTAGE && !IsAnyDischarging(highestBoard,highestCell)) // if the highest voltage is greater than the threshold than discharge the cell
	{
		cli();
		chain.boards[highestBoard].cells[highestCell].DischargeFlag = true;
		chain.updateConfig = true;
		sei();
		CHG_LED_PORT &= ~CHG_LED_MASK;
	}		

	if (allIn)
	{
		CHG_LED_PORT &= ~CHG_LED_MASK;
		for (uint8_t x = 0; x < BMS_BOARD_COUNT; x++)
		{
			for (uint8_t y = 0; y < BMS_CELL_COUNT; y++)
			{
				if (!chain.boards[x].cells[y].IgnoreFlag)
				{
					cli();
					bool dis = chain.boards[x].cells[y].DischargeFlag;
					sei();
					if (dis)
					{
						cli();
						chain.boards[x].cells[y].DischargeFlag = false;
						chain.updateConfig = true;
						sei();
					}
				}
			}
		}
	}
	else if (!(CHG_LED_PORT & CHG_LED_MASK))
	{
		uint8_t startCharging = true;
		for (uint8_t x = 0; x < BMS_BOARD_COUNT; x++)
		{
			for (uint8_t y = 0; y < BMS_CELL_COUNT; y++)
			{
				if (!chain.boards[x].cells[y].IgnoreFlag)
				{
					cli();
					BmsCell_t cell;
					cell.LastReading = chain.boards[x].cells[y].LastReading;
					cell.DischargeFlag = chain.boards[x].cells[y].DischargeFlag;
					sei();
					
					if (cell.LastReading >= HIGH_CHARGING_VOLTAGE || 
						(cell.LastReading >= highestDischargeVoltage && highestDischargeVoltage != 0))
					{
						/*
						char thing[10];
						numberToString(highestDischargeVoltage, thing,-1);
						
						SerialSend(&serial,"\n\n\n\n\n\n\n\n\n\n");
						_delay_ms(1000);
						SerialSend(&serial,"\n\n\n\n\n\n\n\n\n\n");
						SerialSend(&serial, thing);
						while(1);*/
						startCharging = false;
						if (!cell.DischargeFlag && !IsAnyDischarging(x,y))
						{
							cli();
							chain.boards[x].cells[y].DischargeFlag = true;
							chain.updateConfig = true;
							sei();
						}
					}
					if (cell.DischargeFlag)
					{
						startCharging = false;
						if (cell.LastReading <= lowChargingVoltage)
						{
							/*
						char thing[10];
						numberToString(lowChargingVoltage, thing,-1);
						
						SerialSend(&serial,"\n\n\n\n\n\n\n\n\n\n");
						_delay_ms(1000);
						SerialSend(&serial, thing);
						SerialSend(&serial,"\n\n\n\n\n\n\n\n\n\nCells");
						
						
						for (uint8_t X = 0; X < BMS_BOARD_COUNT; X++)
						{
							for (uint8_t Y = 0; Y < BMS_CELL_COUNT; Y++)
							{
								if (!chain.boards[x].cells[y].IgnoreFlag)
								{
									numberToString(chain.boards[X].cells[Y].LastReading, thing,-1);
									SerialSend(&serial,thing);
									SerialSend(&serial, " ");
									SerialSend(&serial, chain.boards[X].cells[Y].DischargeFlag ? "1" : "0");
									SerialSend(&serial,"\n ");
								}
							}
						}
						
						*/
							cli();
							chain.boards[x].cells[y].DischargeFlag = false;
							chain.updateConfig = true;
							sei();							
							if (!(IsAnyDischargingAtAll()))
							{
								startCharging = true;

							}
							
						}
					}
				}
			}
		}
		if (startCharging)
		{
			CHG_LED_PORT |= CHG_LED_MASK;
		}
	}			

}


uint16_t GetBatteryTemp()
{
	//#warning WE NEED A METHOD OF GETTING THE TEMP
	cli();
	uint16_t ret = chain.boards[1].gpio_voltage[0];
	sei();
	return ret;
}

void HandleFan()
{
	// Higher voltage means lower temp
	uint16_t temp = GetBatteryTemp();
	if (FAN_PORT & FAN_MASK)
	{
		if (temp >= BATTERY_TEMP_FAN_OFF)
		{
			FAN_PORT &= ~FAN_MASK;
		}
	}
	else
	{
		if (temp <= BATTERY_TEMP_FAN_ON)
		{
			FAN_PORT |= FAN_MASK;
		}
	}
}

bool IsTempSafe()
{
	// Higher voltage means lower temp
	//return true;
	return GetBatteryTemp() > BATTERY_TEMP_MAX;
}

bool AreBatteriesSafe()
{
	for (int x = 0; x < BMS_BOARD_COUNT; x++)
	{
		for (int y = 0; y < BMS_CELL_COUNT; y++)
		{
			if (!chain.boards[x].cells[y].IgnoreFlag)
			{
				cli();
				uint16_t reading = chain.boards[x].cells[y].LastReading;
				sei();
				
				if (reading <= BATTERY_VOLTAGE_LOW_CUTOFF)
					return false;
				if (reading >= BATTERY_VOLTAGE_HIGH_CUTOFF)
					return false;
			}
		}
	}
	return true;
}
bool IsCurrentSafe()
{
	static uint16_t ms_since_low = 0;
	
	
	int16_t current = GetCurrentFlow();
	
	if (current >= CURRENT_HARD_CUT_OFF)
	{
		return false;
	}
	
	if (current >= CURRENT_SOFT_CUT_OFF)
	{
		cli();
		uint16_t ms = ms_count;
		sei();
		if ((uint16_t)(ms - ms_since_low) >= CURRENT_SOFT_TIME_OUT)
		{
			return false;
		}
	}
	else
	{
		cli();
		ms_since_low = ms_count;
		sei();
	}
	return true;
}
bool IsCarSafe()
{
	return IsCurrentSafe() && AreBatteriesSafe() && IsTempSafe();
}


void HumanReadablePrintCells()
{
	static uint16_t lastDisplay = 0;
	cli();
	uint16_t ms_current = ms_count;
	sei();
	if ((uint16_t)(ms_current - lastDisplay) > 1000)
	{
		lastDisplay = ms_current;
		for (uint8_t y = 0; y < BMS_BOARD_COUNT; y++)
		{
			for (uint8_t x = 0; x < 12; x++)
			{
				if (!chain.boards[y].cells[x].IgnoreFlag){
					char reading[8];
					cli();
					numberToString(chain.boards[y].cells[x].LastReading, reading,6);
					sei();
					SerialSend(&serial,reading);
					
					SerialSend(&serial,"=C#");
					numberToString(x+1, reading,255);
					SerialSend(&serial,reading);
					if (chain.boards[y].cells[x].DischargeFlag)
					{
						SerialSend(&serial,"*, ");
					}
					else
					{
						SerialSend(&serial," , ");
					}
				}
				
			}
			SerialSend(&serial,'\n');
			for (uint8_t x = 0; x < 6; x++)
			{
				char reading[8];
				cli();
				numberToString(chain.boards[y].gpio_voltage[x], reading,6);
				sei();
				SerialSend(&serial,reading);
				SerialSend(&serial,"=GPIO");
				numberToString(x, reading,255);
				SerialSend(&serial,reading);
				SerialSend(&serial," , ");
			}
			SerialSend(&serial,'\n');
		}
		SerialSend(&serial,'\n');
		SerialSend(&serial,'\n');
		SerialSend(&serial,'\n');
	}
		
}


void GraphingPrintCells()
{
	
	static uint16_t lastDisplay = 0;
	cli();
	uint16_t ms_current = ms_count;
	sei();
	
	if ((uint16_t)(ms_current - lastDisplay) > 100)
	{
		lastDisplay = ms_current;
		for (uint8_t y = 0; y < BMS_BOARD_COUNT; y++)
		{
			for (uint8_t x = 0; x < 12; x++)
			{
				if (!chain.boards[y].cells[x].IgnoreFlag){
					cli();
					uint16_t last = chain.boards[y].cells[x].LastReading;
					bool dis = chain.boards[y].cells[x].DischargeFlag;
					sei();
					SerialSend(&serial, last>>0);
					SerialSend(&serial, last>>8);
					SerialSend(&serial, dis ? 1 : 0);
				}
				
			}
		}
		uint16_t current = GetCurrentFlow();
		uint16_t temp = GetBatteryTemp();
		
		SerialSend(&serial, current >> 0);
		SerialSend(&serial, current >> 8);
		SerialSend(&serial, temp >> 0);
		SerialSend(&serial, temp >> 8);
		
		
		SerialSend(&serial,"\n\n\n\n\n\n\n\n");
	}
}


int main()
{

	CHG_LED_DDR |= CHG_LED_MASK;
	CHG_LED_PORT |= CHG_LED_MASK;
	OK_LED_PORT |= OK_LED_MASK;
	OK_LED_PORT |= OK_LED_MASK;
	
	srand(1234);
	for (uint8_t x = 0; x < BMS_BOARD_COUNT; x++)
	{
		for (uint8_t y = 0; y < BMS_CELL_COUNT; y++)
		{

			chain.boards[x].cells[y].LastReading= 0;//rand()%(HIGH_CHARGING_VOLTAGE-3500)+3500;
		}
	}
	
	SerialBegin(&serial,9600, &UDR0, &UCSR0A, &UCSR0B, &UBRR0);
	OK_LED_DDR |= OK_LED_MASK;
	Timer_t timer;
	IntializeTimer(&timer,&TCCR0A,&TCCR0B, &OCR0A, &TIMSK0, &TCNT0,0);
	
	timer.timeMask = CS_DIV1024;
	*(uint8_t*)timer.ocra = 156;
	TimerStart(&timer);
	TimerSetOverflowInterrupt(&timer,true);
	InitializeAdc((Adc_t*)&Adc,&ADCSRA,&ADCSRB,&ADMUX,&ADC);
	
	Adc.doConversions[0] = true;
	Adc.doConversions[1] = true;
	EnableAdc((Adc_t*)&Adc);
	

	spi_t spi;
	spi_device_t dev;	
	InitializeSpiDevice(&dev,&PORTB,&DDRB,1<<2);
	InitializeSpiMaster(&spi,
						&SPCR,&SPDR,&SPSR,
						&DDRB,1<<5,
						&DDRB,1<<3);
	SetClockDividerSpi(&spi,SPI_CLOCK_DIV16);
	
	chain.spi = &spi;
	chain.device = &dev;
	
	
	for (int x = 0; x < BMS_BOARD_COUNT; x++)
	{
		chain.boards[x].config.GPIO1 = 0;  //Set pulldowns
		chain.boards[x].config.GPIO2 = 0;
		chain.boards[x].config.GPIO3 = 0;
		chain.boards[x].config.GPIO4 = 0;
		chain.boards[x].config.GPIO5 = 0;
		for (int y = 0; y < BMS_CELL_COUNT; y++)
		{
			chain.boards[x].cells[y].DischargeFlag = false;
			chain.boards[x].cells[y].IgnoreFlag = ignoreCell[y];
			chain.boards[x].cells[y].OddFlag = oddCells[y];
		}
	}
	chain.boards[0].cells[0].Error = 1;  //C1
	chain.boards[0].cells[1].Error = -3;  //C2
	chain.boards[0].cells[2].Error = -3;   //C3
	chain.boards[0].cells[3].Error = -3;   //C4
	chain.boards[0].cells[4].Error = -3;   //C5
	//chain.boards[0].cells[5].Error = 1;   //C6
	chain.boards[0].cells[6].Error = -5;   //C7
	chain.boards[0].cells[7].Error = -3;   //C8
	chain.boards[0].cells[8].Error = -3;   //C9
	chain.boards[0].cells[9].Error = -3;   //C10
	chain.boards[0].cells[10].Error = 106;  //C11
	//chain.boards[0].cells[11].Error = 1;  //C12
	
	chain.boards[1].cells[0].Error = 3; //C1
	chain.boards[1].cells[1].Error = -4; //C2
	chain.boards[1].cells[2].Error = -3;  //C3
	chain.boards[1].cells[3].Error = -1;  //C4
	chain.boards[1].cells[4].Error = -5;  //C5
	//chain.boards[1].cells[5].Error = 1;  //C6
	chain.boards[1].cells[6].Error = -6;  //C7
	chain.boards[1].cells[7].Error = -7;  //C8
	chain.boards[1].cells[8].Error = -6;  //C9
	chain.boards[1].cells[9].Error = -6;  //C10
	chain.boards[1].cells[10].Error = 199; //C11
	//chain.boards[1].cells[11].Error = 1; //C12

	
	
	
	
	chain.adcConversionMode = 0x260 | 0x2 << 7;
	chain.gpioConversionMode = 0b10101100000;
	_delay_ms(1000);
	
	
	TimerSetOverflowInterrupt(&timer,true);
	sei();
	_delay_ms(1000);
	while(true){
		
		
		
		//HumanReadablePrintCells();
		//while (true) { OK_LED_PORT ^= OK_LED_MASK; _delay_ms(1000);}
		
		GraphingPrintCells();
		
		if (IsCarSafe())
		{
			OK_LED_PORT |= OK_LED_MASK;
		}
		else
		{
			OK_LED_PORT &= ~OK_LED_MASK;
		}
		HandleFan();
		#define USE_CELL_BALANCING
		#ifdef USE_CELL_BALANCING
			BalanceBmsCells();
		#endif
		
		
	}
	
	//DDRC |= 0x1F;
	
	
	uint8_t x = 0xaa;
	/*WriteSpi(&spi,&dev,&x,1);
	_delay_ms(1000);
	WriteSpi(&spi,&dev,&x,1);
	_delay_ms(1000);*/
	//writeConfig(&spi,&dev);
	
	
	
	//uint8_t config2[12];
	//CopyDataAndPEC(config2+4,((uint8_t*)&config),6);
	

	
	//ChainWriteCommand(&chain,1,(uint8_t*)config2,8);
//	ChainWriteCommand(&chain,1,(uint8_t*)config2,8);
	//writeConfig(&spi,&dev);
	cli();
	ChainFlushConfig((BmsChain_t*)&chain);
	sei();
	//ChainWriteCommand(&chain,1,(uint8_t*)config2,8);
	
	//writeConfig(&spi,&dev);

	
	while(true){
		/*
		_delay_ms(1000);
		*dev.SS_out &= ~dev.SS_mask;
		_delay_ms(5);
		*dev.SS_out |= dev.SS_mask;
		writeConfig(&spi,&dev);
		writeConfig(&spi,&dev);
		writeConfig(&spi,&dev);*/
		
		
		_delay_ms(1000);
		//ChainUpdateCellReadings(&chain);
		
		chain.boards[0].config.DischargeCell = 0;
		
		cli();
		ChainFlushConfig((BmsChain_t*)&chain);
		sei();
		
		
		//debug(ChainReadGpioAdc(&chain,BMS_TEMP_SENSOR_REGISTER,BMS_TEMP_SENSOR_OFFSET));
		/*
		for (int x = 0; x < 12; x++)
		{
			debug(chain.boards[0].cells[x].LastReading>>8);
		}*/
		
			
	}
}