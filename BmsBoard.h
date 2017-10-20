#ifndef msoe_sae_BmsBoard_h
#define msoe_sae_BmsBoard_h

#include "spi.h"

#define BMS_BOARD_COUNT 2
#define BMS_CELL_COUNT 12

#define BMS_TEMP_SENSOR_PIN 1
#define BMS_TEMP_SENSOR_REGISTER 0x000C
#define BMS_TEMP_SENSOR_OFFSET 0

static const uint16_t crc15Table[256] = 
	{  0x0, 0xc599, 0xceab, 0xb32,  0xd8cf, 0x1d56, 0x1664, 0xd3fd, 0xf407, 0x319e, 0x3aac,
    0xff35, 0x2cc8, 0xe951, 0xe263, 0x27fa, 0xad97, 0x680e, 0x633c, 0xa6a5, 0x7558, 0xb0c1,
    0xbbf3, 0x7e6a, 0x5990, 0x9c09, 0x973b, 0x52a2, 0x815f, 0x44c6, 0x4ff4, 0x8a6d, 0x5b2e,
    0x9eb7, 0x9585, 0x501c, 0x83e1, 0x4678, 0x4d4a, 0x88d3, 0xaf29, 0x6ab0, 0x6182, 0xa41b,
    0x77e6, 0xb27f, 0xb94d, 0x7cd4, 0xf6b9, 0x3320, 0x3812, 0xfd8b, 0x2e76, 0xebef, 0xe0dd,
    0x2544, 0x2be,  0xc727, 0xcc15, 0x98c,  0xda71, 0x1fe8, 0x14da, 0xd143, 0xf3c5, 0x365c,
    0x3d6e, 0xf8f7, 0x2b0a, 0xee93, 0xe5a1, 0x2038, 0x7c2,  0xc25b, 0xc969, 0xcf0, 0xdf0d,
    0x1a94, 0x11a6, 0xd43f, 0x5e52, 0x9bcb, 0x90f9, 0x5560, 0x869d, 0x4304, 0x4836, 0x8daf,
    0xaa55, 0x6fcc, 0x64fe, 0xa167, 0x729a, 0xb703, 0xbc31, 0x79a8, 0xa8eb, 0x6d72, 0x6640,
    0xa3d9, 0x7024, 0xb5bd, 0xbe8f, 0x7b16, 0x5cec, 0x9975, 0x9247, 0x57de, 0x8423, 0x41ba,
    0x4a88, 0x8f11, 0x57c,  0xc0e5, 0xcbd7, 0xe4e,  0xddb3, 0x182a, 0x1318, 0xd681, 0xf17b,
    0x34e2, 0x3fd0, 0xfa49, 0x29b4, 0xec2d, 0xe71f, 0x2286, 0xa213, 0x678a, 0x6cb8, 0xa921,
    0x7adc, 0xbf45, 0xb477, 0x71ee, 0x5614, 0x938d, 0x98bf, 0x5d26, 0x8edb, 0x4b42, 0x4070,
    0x85e9, 0xf84,  0xca1d, 0xc12f, 0x4b6,  0xd74b, 0x12d2, 0x19e0, 0xdc79, 0xfb83, 0x3e1a, 0x3528,
    0xf0b1, 0x234c, 0xe6d5, 0xede7, 0x287e, 0xf93d, 0x3ca4, 0x3796, 0xf20f, 0x21f2, 0xe46b, 0xef59,
    0x2ac0, 0xd3a,  0xc8a3, 0xc391, 0x608,  0xd5f5, 0x106c, 0x1b5e, 0xdec7, 0x54aa, 0x9133, 0x9a01,
    0x5f98, 0x8c65, 0x49fc, 0x42ce, 0x8757, 0xa0ad, 0x6534, 0x6e06, 0xab9f, 0x7862, 0xbdfb, 0xb6c9,
    0x7350, 0x51d6, 0x944f, 0x9f7d, 0x5ae4, 0x8919, 0x4c80, 0x47b2, 0x822b, 0xa5d1, 0x6048, 0x6b7a,
    0xaee3, 0x7d1e, 0xb887, 0xb3b5, 0x762c, 0xfc41, 0x39d8, 0x32ea, 0xf773, 0x248e, 0xe117, 0xea25,
    0x2fbc, 0x846,  0xcddf, 0xc6ed, 0x374,  0xd089, 0x1510, 0x1e22, 0xdbbb, 0xaf8,  0xcf61, 0xc453,
    0x1ca,  0xd237, 0x17ae, 0x1c9c, 0xd905, 0xfeff, 0x3b66, 0x3054, 0xf5cd, 0x2630, 0xe3a9, 0xe89b,
    0x2d02, 0xa76f, 0x62f6, 0x69c4, 0xac5d, 0x7fa0, 0xba39, 0xb10b, 0x7492, 0x5368, 0x96f1, 0x9dc3,
    0x585a, 0x8ba7, 0x4e3e, 0x450c, 0x8095
                                            };

uint16_t pec15_calc(uint8_t len, //Number of bytes that will be used to calculate a PEC
                    uint8_t *data //Array of data that will be used to calculate  a PEC
                   )
{
  uint16_t remainder,addr;

  remainder = 16;//initialize the PEC
  for (uint8_t i = 0; i<len; i++) // loops for each byte in data array
  {
    addr = ((remainder>>7)^data[i])&0xff;//calculate PEC table address
    remainder = (remainder<<8)^crc15Table[addr];
  }
  return(remainder*2);//The CRC15 has a 0 in the LSB so the remainder must be multiplied by 2
}


static const bool ignoreCell[BMS_CELL_COUNT] = {false,false,false,false,false,true,false,false,false,false,false,true};
												//C1   C2    C3    C4    C5    C6    C7    C8   C9    c10   C11   C12
static const bool oddCells[BMS_CELL_COUNT] =   {true ,false,true ,false,true ,false,false,true,false,true ,false,false};

typedef struct
{
	uint16_t LastReading;
	uint8_t DischargeFlag;
	uint8_t IgnoreFlag;
	uint8_t OddFlag;
	int16_t Error;  //In hundredths of a percent
	
} BmsCell_t;
typedef struct
__attribute__((packed))
{
	// CFGR0
	uint8_t AdcModeOption : 1;
	uint8_t Unused0 : 1;
	//Whether or not the reference is always on or just during conversion
	uint8_t ReferenceOn : 1;
	
	
	//When writing to the register these represent
	//the pull down for each of the GPIO pins
	//A "1" enables the pull down
	
	//When reading from the register these represent
	//the logical state of the pin
	uint8_t GPIO1 : 1;
	uint8_t GPIO2 : 1;
	uint8_t GPIO3 : 1;
	uint8_t GPIO4 : 1;
	uint8_t GPIO5 : 1;
	
	
	uint16_t UnderVoltageThreshold : 12;

	uint16_t OvervoltageThreshold : 12;
	
	uint16_t DischargeCell : 12;
	uint8_t DischargeTimeOut : 4;
	
} SingleBoardConfig_t;
typedef struct
{
	SingleBoardConfig_t config;
	BmsCell_t cells[BMS_CELL_COUNT];
	uint16_t gpio_voltage[6];	
} BmsBoard_t;

typedef struct
{
	spi_t* spi;
	spi_device_t* device;
	BmsBoard_t boards[BMS_BOARD_COUNT];
	uint16_t adcConversionMode;
	uint16_t gpioConversionMode;
	uint8_t updateConfig;
} BmsChain_t;
uint8_t ChainRead(BmsChain_t* chain, uint16_t command, uint8_t* r_config,uint8_t length, uint8_t checkPEC);

void CopyDataAndPEC(uint8_t* destination, uint8_t* data, uint8_t length)
{
	uint16_t PEC = pec15_calc(length, data);
	for (uint8_t x = 0; x < length; x++)
	{
		*destination = *data;
		destination++;
		data++;
	}
	*destination = PEC >> 8;
	destination++;
	*destination = PEC;
}

void ChainWakeUp(BmsChain_t* chain)
{
	uint8_t x = 0xaa;
	WriteSpi(chain->spi,chain->device,&x,1);
	WriteSpi(chain->spi,chain->device,&x,1);
}
void ChainWriteCommand(BmsChain_t* chain,uint16_t command, uint8_t* data, uint8_t length)
{
	
	//Note that the first four slots of the data array must be empty
	data[0] = command >> 8;
	data[1] = command;
	uint16_t pec = pec15_calc(2, data);
	data[2] = pec >> 8;
	data[3] = pec;
	
	ChainWakeUp(chain);
	
	WriteSpi(chain->spi,chain->device,data,length+4);
	
}
void ChainCommand(BmsChain_t* chain,uint16_t command)
{
	uint8_t data[4];
	data[0] = command >> 8;
	data[1] = command;
	uint16_t pec = pec15_calc(2, data);
	data[2] = pec >> 8;
	data[3] = pec;
	
	ChainWakeUp(chain);
	
	WriteSpi(chain->spi,chain->device,data,4);
	
}
void ChainFlushConfig(BmsChain_t* chain)
{
	//static_assert(sizeof(SingleBoardConfig_t)==6,"");
	
	
	uint8_t arr[(2+sizeof(SingleBoardConfig_t))*BMS_BOARD_COUNT+4];
	
	uint8_t index = 4;
	for (uint8_t x = 0; x < BMS_BOARD_COUNT; x++)
	{
		
		CopyDataAndPEC(arr+index,(uint8_t*)&chain->boards[x].config,sizeof(SingleBoardConfig_t));
		index += sizeof(SingleBoardConfig_t) + 2;
	}

	ChainWriteCommand(chain,1,arr,(2+sizeof(SingleBoardConfig_t))*BMS_BOARD_COUNT);
	
}

static const uint16_t CellRegisterGroupVoltageCommand[4] = {0x0004, 0x0006, 0x0008, 0x000A};
static const uint16_t GPIORegisterGroupVoltageCommand[2] = {0x000C, 0x000E};
void ChainUpdateCellReadings(BmsChain_t* chain)
{
	ChainCommand(chain,chain->adcConversionMode);
	const uint8_t bytesPerChunk = (3*2+2);
	uint8_t data[BMS_BOARD_COUNT*bytesPerChunk];
	//_delay_ms(10);
	for (uint8_t y = 0; y < BMS_CELL_COUNT; y+=3)
	{
		ChainRead(chain, CellRegisterGroupVoltageCommand[y/3],data,BMS_BOARD_COUNT*bytesPerChunk,0);
		for (uint8_t x = 0; x < BMS_BOARD_COUNT; x++)
		{
			chain->boards[BMS_BOARD_COUNT - 1 - x].cells[y].LastReading = data[x*bytesPerChunk];
			chain->boards[BMS_BOARD_COUNT - 1 - x].cells[y].LastReading |= data[x*bytesPerChunk+1]<<8;
			chain->boards[BMS_BOARD_COUNT - 1 - x].cells[y+1].LastReading = data[x*bytesPerChunk+2];
			chain->boards[BMS_BOARD_COUNT - 1 - x].cells[y+1].LastReading |= data[x*bytesPerChunk+3]<<8;
			chain->boards[BMS_BOARD_COUNT - 1 - x].cells[y+2].LastReading = data[x*bytesPerChunk+4];
			chain->boards[BMS_BOARD_COUNT - 1 - x].cells[y+2].LastReading |= data[x*bytesPerChunk+5]<<8;
			uint16_t pec1 = pec15_calc(6,&data[x*bytesPerChunk]);
			uint16_t pec2 = data[x*bytesPerChunk+6]<<8;
			pec2 |= data[x*bytesPerChunk+7];
			if (pec1 != pec2)
			{
				
			}
			for (uint8_t z = 0; z < 3; z++)
			{
				BmsCell_t* cell = &chain->boards[BMS_BOARD_COUNT - 1 - x].cells[y+z];
				cell->LastReading += ((int32_t)cell->LastReading * (int32_t)cell->Error)/10000;
			}
		}
		
	}
}

void ChainGpioAdcConvert(BmsChain_t* chain, uint8_t pin, uint8_t mode)
{
	//ChainCommand(
}
void ChainUpdateGpioReadings(BmsChain_t* chain)
{

	ChainCommand(chain,chain->gpioConversionMode);
	const uint8_t bytesPerChunk = (3*2+2);
	uint8_t data[BMS_BOARD_COUNT*bytesPerChunk];
	//_delay_ms(10);
	for (uint8_t y = 0; y < 6; y+=3)
	{
		ChainRead(chain, GPIORegisterGroupVoltageCommand[y/3],data,BMS_BOARD_COUNT*bytesPerChunk,0);
		for (uint8_t x = 0; x < BMS_BOARD_COUNT; x++)
		{
			chain->boards[BMS_BOARD_COUNT -1 - x].gpio_voltage[y]    = data[x*bytesPerChunk];
			chain->boards[BMS_BOARD_COUNT -1 - x].gpio_voltage[y]   |= data[x*bytesPerChunk+1]<<8;
			chain->boards[BMS_BOARD_COUNT -1 - x].gpio_voltage[y+1]  = data[x*bytesPerChunk+2];
			chain->boards[BMS_BOARD_COUNT -1 - x].gpio_voltage[y+1] |= data[x*bytesPerChunk+3]<<8;
			chain->boards[BMS_BOARD_COUNT -1 - x].gpio_voltage[y+2]  = data[x*bytesPerChunk+4];
			chain->boards[BMS_BOARD_COUNT -1 - x].gpio_voltage[y+2] |= data[x*bytesPerChunk+5]<<8;
			uint16_t pec1 = pec15_calc(6,&data[x*bytesPerChunk]);
			uint16_t pec2 = data[x*bytesPerChunk+6]<<8;
			pec2 |= data[x*bytesPerChunk+7];
			if (pec1 != pec2)
			{
			}
		}
	}
}

uint8_t ChainRead(BmsChain_t* chain, uint16_t command, uint8_t* r_config,uint8_t length, uint8_t checkPEC)
{
	uint8_t data[4];
	data[0] = command >> 8;
	data[1] = command;
	uint16_t pec = pec15_calc(2, data);
	data[2] = pec >> 8;
	data[3] = pec;
	ChainWakeUp(chain);
	
	WriteReadSpi(chain->spi,chain->device,data,4,r_config,length);
	if (checkPEC)
	{	
		uint16_t received_pec = r_config[length-2] << 8;
		received_pec |= r_config[length-1];
		uint16_t data_pec = pec15_calc(length-2, &r_config[0]);
		if (data_pec != received_pec)
		{	  	
			return -1;
		}
	}
	return 0;
}


void writeConfig(spi_t* spi,spi_device_t* dev)
{
  uint8_t config[6] =
  {
	0xfe,0xaa,0,0,0,0
  };
  const uint8_t total_ic = 1;
  const uint8_t BYTES_IN_REG = 6;
  const uint8_t CMD_LEN = 4+(8*total_ic);
  uint8_t cmd[CMD_LEN*sizeof(uint8_t)];
  uint16_t cfg_pec;
  uint8_t cmd_index; //command counter

  //cmd = (uint8_t *)malloc(CMD_LEN*sizeof(uint8_t));

  //1
  cmd[0] = 0x00;
  cmd[1] = 0x01;
  cmd[2] = 0x3d;
  cmd[3] = 0x6e;

  //2
  cmd_index = 4;
  for (uint8_t current_ic = total_ic; current_ic > 0; current_ic--)       // executes for each LTC6804 in daisy chain, this loops starts with
  {
    // the last IC on the stack. The first configuration written is
    // received by the last IC in the daisy chain
/*
    for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG; current_byte++) // executes for each of the 6 bytes in the CFGR register
    {
      // current_byte is the byte counter

      //cmd[cmd_index] = config[current_ic-1][current_byte];            //adding the config data to the array to be sent
	  cmd[cmd_index] = config[current_byte];
      cmd_index = cmd_index + 1;
    }
    //3
    //cfg_pec = (uint16_t)pec15_calc(BYTES_IN_REG, &config[current_ic-1][0]);   // calculating the PEC for each ICs configuration register data
	cfg_pec = (uint16_t)pec15_calc(BYTES_IN_REG, &config[0]);   // calculating the PEC for each ICs configuration register data
    cmd[cmd_index] = (uint8_t)(cfg_pec >> 8);
    cmd[cmd_index + 1] = (uint8_t)cfg_pec;*/
	CopyDataAndPEC(&cmd[cmd_index],config,6);
    cmd_index = cmd_index + 8;
  }
  
  //4
  //wakeup_idle ();                                 //This will guarantee that the LTC6804 isoSPI port is awake.This command can be removed.
  //5
  
  for (int x = 0; x < CMD_LEN; x++)
  {
	  //debug(cmd[x]);
  }
  //while(1);
  
  *dev->SS_out &= ~dev->SS_mask;
  _delay_us(2);
  *dev->SS_out |= dev->SS_mask;
  
  WriteSpi(spi,dev,&cmd[3],1);
  WriteSpi(spi,dev,&cmd[3],1);
  
  WriteSpi(spi,dev,cmd,CMD_LEN);
  //output_low(LTC6804_CS);
  //spi_write_array(CMD_LEN, cmd);
  //output_high(LTC6804_CS);
  //free(cmd);
}
uint8_t readConfig(spi_t* spi, spi_device_t* dev, uint8_t* r_config)
{
	const uint8_t total_ic = 1;
	const uint8_t BYTES_IN_REG = 8;

  uint8_t cmd[4];
  uint8_t rx_data[(8*total_ic)*sizeof(uint8_t)];
  int8_t pec_error = 0;
  uint16_t data_pec;
  uint16_t received_pec;

  //rx_data = (uint8_t *) malloc((8*total_ic)*sizeof(uint8_t));

  //1
  cmd[0] = 0x00;
  cmd[1] = 0x02;
  cmd[2] = 0x2b;
  cmd[3] = 0x0A;

  //2
//  wakeup_idle (); //This will guarantee that the LTC6804 isoSPI port is awake. This command can be removed.
  //3
  
  *dev->SS_out &= ~dev->SS_mask;
  _delay_us(2);
  *dev->SS_out |= dev->SS_mask;
  WriteSpi(spi,dev,&cmd[3],1);
  WriteSpi(spi,dev,&cmd[3],1);
  /*
  output_low(LTC6804_CS);
  spi_write_read(cmd, 4, rx_data, (BYTES_IN_REG*total_ic));         //Read the configuration data of all ICs on the daisy chain into
  output_high(LTC6804_CS);                          //rx_data[] array*/
  WriteReadSpi(spi,dev,cmd,4,r_config,(BYTES_IN_REG*total_ic));
  received_pec = r_config[6] << 8;
  received_pec |= r_config[7];
  data_pec = pec15_calc(6, &r_config[0]);
  if (data_pec != received_pec)
  {	  
	
	  return -1;
  }
  return 0;
  
  /*WriteSpi(spi,dev, cmd,4);
  ReadSpi(spi,dev, 0xFF, r_config, (BYTES_IN_REG*total_ic));*/
  /*
  for (uint8_t current_ic = 0; current_ic < total_ic; current_ic++)       //executes for each LTC6804 in the daisy chain and packs the data
  {
    //into the r_config array as well as check the received Config data
    //for any bit errors
    //4.a
    for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG; current_byte++)
    {
      //r_config[current_ic][current_byte] = rx_data[current_byte + (current_ic*BYTES_IN_REG)];
	  r_config[current_byte] = rx_data[current_byte + (current_ic*BYTES_IN_REG)];
    }
    //4.b
    //received_pec = (r_config[current_ic][6]<<8) + r_config[current_ic][7];
	received_pec = (r_config[6]<<8) + r_config[7];
	
    //data_pec = pec15_calc(6, &r_config[current_ic][0]);
	data_pec = pec15_calc(6, &r_config[0]);
	
    if (received_pec != data_pec)
    {
      pec_error = -1;
    }
  }

  //free(rx_data);
  //5
  return(pec_error);*/
}



#endif

