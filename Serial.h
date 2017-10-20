#ifndef msoe_sae_Serial_h
#define msoe_sae_Serial_h
typedef struct
{
	register8_t udr;
	register8_t ucsra;
	register8_t ucsrb;
	
} Serial_t;

void SerialBegin(Serial_t* serial,uint32_t baud, register8_t udr,register8_t ucsra, register8_t ucsrb, register16_t ubrr)
{
	serial->udr = udr;
	serial->ucsra = ucsra;
	serial->ucsrb = ucsrb;
	*ubrr = 1000000UL/baud - 1;  //Set the baud rate (assuming we are using the external 16mhz clock)
	*ucsrb |= 1 << RXEN0;            //Enable receive
	*ucsrb |= 1 << TXEN0;	         //Enable transmit
}

void SerialSend(Serial_t* serial,uint8_t data)
{
	*serial->udr = data;
	while (!(*serial->ucsra & 1<<TXC0));
	
	*serial->ucsra |= 1<<TXC0;
}
void SerialSend(Serial_t* serial, const char* data)
{
	while (*data)
	{
		SerialSend(serial,*data);
		data++;
	}
}

#endif

