#ifndef msoe_sae_Timer_h
#define msoe_sae_Timer_h

//#define HARD_CODE_TIMER_TEST
#define CS_OFF        0x00
#define CS_NONE       0x01
#define CS_DIV8       0x02
#define CS_DIV64      0x03
#define CS_DIV256     0x04
#define CS_DIV1024    0x05
#define CS_EXT_FALL   0x06
#define CS_EXT_RISE   0x07
#define TIMER_TCCR TCCR
#define TIMER_B B
#define TIMER_OCR OCR
#define TIMER_A A
#define TIMER_TIMSK TIMSK
#define UseTimer(Name,Number)	UseInterrupt(Name ## Timer,TIMER ## Number ## _COMPA_vect)\
								Timer Name(& Name ## Timer,& TCCR ## Number ## B, & OCR ## Number ## A, & TIMSK ## Number, &TCNT ## Number);

								
		
typedef struct  {
	register8_t tccrb;
	volatile void* ocra;
	register8_t timsk;
	volatile void* tcnt;
	
	uint8_t timeMask;
	uint8_t doubleWidth;
}Timer_t;


		void IntializeTimer(Timer_t* timer,register8_t tccra,register8_t tccrb,volatile void* ocra,register8_t timsk,volatile void* tcnt,uint8_t doubleWidth) 
		//: time(1), timeMask(CS_NONE), tccrb(tccrb), ocra(ocra), timsk(timsk),tcnt(tcnt)
		{
			timer->doubleWidth = doubleWidth;
			timer->timeMask = CS_NONE;
			timer->tccrb = tccrb;
			timer->ocra = ocra;
			timer->timsk = timsk;
			timer->tcnt = tcnt;
			
			if (!doubleWidth){ //One byte
				*tccra |= 1 << WGM01;  //Make it roll over at the ocra value
				*(register8_t)timer->ocra = 0xff;
			}
			else{
				*tccrb |= 1 << WGM12; //Make it roll over at the ocra value
				//SetOverflow(system::CPU::systemTick(0xFFFF * time.Count()));
				*(register16_t)timer->ocra = 0xff;
			}
		}

		void TimerStart(Timer_t* timer){
			*timer->tccrb |= timer->timeMask << CS00;
		}
		void TimerStop(Timer_t* timer){
			*timer->tccrb &= ~(0x07 << CS00);
		}
		void TimerSetOverflowInterrupt(Timer_t* timer,bool value){
			if (value){
				*timer->timsk |= 1 << OCIE1A;
			}
			else{
				*timer->timsk &= ~(1 << OCIE1A);
			}
		}
		bool TimerGetOverflowInterrupt(Timer_t* timer){
			return *timer->timsk & (1 << OCIE1A);
		}

#endif

