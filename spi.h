#ifndef msoe_sae_spi_h
#define msoe_sae_spi_h

#define SPI_CLOCK_DIV4 0x00
#define SPI_CLOCK_DIV16 0x01
#define SPI_CLOCK_DIV64 0x02
#define SPI_CLOCK_DIV128 0x03
#define SPI_CLOCK_DIV2 0x04
#define SPI_CLOCK_DIV8 0x05
#define SPI_CLOCK_DIV32 0x06

#define SPI_CLOCK_MASK 0x03  // SPR1 = bit 1, SPR0 = bit 0 on SPCR
#define SPI_2XCLOCK_MASK 0x01  // SPI2X = bit 0 on SPSR

typedef struct {
	register8_t SS_out;
	uint8_t SS_mask;
} spi_device_t;

typedef struct {
	register8_t ctrl;
	register8_t data;
	register8_t sreg;
	
}spi_t;


void InitializeSpiDevice(spi_device_t* dev,
						 register8_t* SS_out,
						 register8_t* SS_dir,
						 uint8_t SS_mask);



void InitializeSpiMaster(	spi_t* spi, 
							register8_t ctrl,register8_t data,register8_t sreg,
							register8_t sck_dir, uint8_t sck_msk,
							register8_t mosi_dir, uint8_t mosi_msk);
void DisableSpi(spi_t* spi);							
void SetClockDividerSpi(spi_t* spi,uint8_t rate);


void InitializeSpiDevice(spi_device_t* dev,
						 register8_t SS_out,
						 register8_t SS_dir,
						 uint8_t SS_mask)
{
	dev->SS_out = SS_out;
	dev->SS_mask = SS_mask;
	
	*SS_out |= SS_mask;
	*SS_dir |= SS_mask;
	
	
}


void InitializeSpiMaster(	spi_t* spi, 
							register8_t ctrl,register8_t data,register8_t sreg,
							register8_t sck_dir, uint8_t sck_msk,
							register8_t mosi_dir, uint8_t mosi_msk)
{
	spi->ctrl = ctrl;  //Save the control registers for later
	spi->data = data;
	spi->sreg = sreg;
	
	*ctrl |= _BV(MSTR);  //Set to master mode
	*ctrl |= _BV(SPE);  //Enable the spi
	
	
	*sck_dir |= sck_msk;  //Set the clock and the MOSI to outputs
	*mosi_dir |= mosi_msk;
	
	
}
void DisableSpi(spi_t* spi)
{
	*spi->ctrl &= ~SPE;
}
void SetClockDividerSpi(spi_t* spi,uint8_t rate)
{
  *spi->ctrl = (*spi->ctrl & ~SPI_CLOCK_MASK) | (rate & SPI_CLOCK_MASK);
  *spi->sreg = (*spi->sreg & ~SPI_2XCLOCK_MASK) | ((rate >> 2) & SPI_2XCLOCK_MASK);
}

void WriteSpi(spi_t* spi,spi_device_t* dev, uint8_t* data, uint16_t count)
{
	//Select the device
	*dev->SS_out &= ~dev->SS_mask;
	for (uint16_t x = 0; x < count; x++)
	{
		
		//Tell the micro to send the data
		*spi->data = *data;
		data++;  //Increment the data buffer
		//Wait for the data to send
		while (!(*spi->sreg & _BV(SPIF) ));
	}
	
	//Deselect the device
	*dev->SS_out |= dev->SS_mask;
}
void ReadSpi(spi_t* spi,spi_device_t* dev, uint8_t send, uint8_t* read, uint16_t count)
{
	
	//Select the device
	*dev->SS_out &= ~dev->SS_mask;
	
	for (uint16_t x = 0; x < count; x++)
	{
		//Tell the micro to send the data
		*spi->data = send;
		
		//Wait for the data to send
		while (!(*spi->sreg & _BV(SPIF)) );
		
		//Read in the data
		*read = *spi->data;
		
		//Increment the buffer position
		read++;
	}
	
	//Deselect the device
	*dev->SS_out |= dev->SS_mask;
}
void WriteReadSpi(spi_t* spi, spi_device_t* dev, uint8_t* data, uint8_t send, uint8_t* read, uint8_t count)
{
	//Select the device
	*dev->SS_out &= ~dev->SS_mask;
	
	
	
	for (uint8_t x = 0; x < send; x++)
	{
		
		//Tell the micro to send the data
		*spi->data = *data;
		data++;  //Increment the data buffer
		//Wait for the data to send
		while (!(*spi->sreg & _BV(SPIF) ));
	}
	for (uint8_t x = 0; x < count; x++)
	{
		
		//Tell the micro to send the data
		*spi->data = 0xff;
		
		//Wait for the data to send
		while (!(*spi->sreg & _BV(SPIF) ));
		
		read[x] = *spi->data;
		
		//read++;  //Increment the data buffer
	}
	
	
	
	//Deselect the device
	*dev->SS_out |= dev->SS_mask;
}


#endif

