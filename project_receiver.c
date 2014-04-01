#include <stdio.h>
#include <rohit.h>

#define CLK 22118400L
#define BAUD 115200L 
#define BRG_VAL (0x100-(CLK/(32L*BAUD)))
#define FREQ 10000L
#define TIMER_2_RELOAD (0x10000L-(CLK/(32L*BAUD)))
#define TIMER0_RELOAD_VALUE (65536L-(CLK/(12L*FREQ)))
#define DISTANCE 0.750000L
#define ZERO 0.00000L
#define UP 4.999999999L
#define DOWN 1024.0000000000L
#define ERROR 0.0400000L
#define FIFTY 0.500000L
#define THIRTYFIVE 0.350000L
#define TWENTY 0.200000L
#define TEN 0.100000L
#define MIN 0.380000L


volatile unsigned char pwmcount;
volatile unsigned char pwm1;
volatile unsigned char pwm2;
volatile unsigned char pwm3;
volatile unsigned char pwm4;

int txon;
float voltage;
char channel;
float v0;
float v1;
int min = MIN;
unsigned char moveback;
unsigned char moveforward;
unsigned char turn180;
unsigned char parallel;
unsigned char val;
void wait_bit_time(void);
void wait_one_and_half_bit_time(void);
unsigned int GetADC(unsigned char channel);
void SPIWrite(unsigned char value);

void move_closer(void);
void move_further(void);
void stop(void);
void turn180s(void);
void parallels(void);

float d1 = FIFTY;
float d2 = THIRTYFIVE;
float d3 = TWENTY;
float d4 = TEN;

float distance = TWENTY;

//void InitTimer0 ( void );



unsigned char _c51_external_startup(void)
{
	// Configure ports as a bidirectional with internal pull-ups.
	P0M0=0;	P0M1=0;
	P1M0=0;	P1M1=0;
	P2M0=0;	P2M1=0;
	P3M0=0;	P3M1=0;
	AUXR=0B_0001_0001; // 1152 bytes of internal XDATA, P4.4 is a general purpose I/O
	P4M0=0;	P4M1=0;
    setbaud_timer2(TIMER_2_RELOAD); // Initialize serial port using timer 2
    
     
    // Instead of using a timer to generate the clock for the serial
    // port, use the built-in baud rate generator.
    PCON|=0x80;
	SCON = 0x52;
    BDRCON=0;
    BRL=BRG_VAL;
    BDRCON=BRR|TBCK|RBCK|SPD;
    
    // Initialize timer 0 for ISR 'pwmcounter()' below
	TR0=0; // Stop timer 0
	TMOD=0x01; // 16-bit timer
	TH0=TIMER0_RELOAD_VALUE/0x100;
	TL0=TIMER0_RELOAD_VALUE%0x100;
	TR0=1; // Start timer 0 (bit 4 in TCON)
	ET0=1; // Enable timer 0 interrupt
	EA=1;  // Enable global interrupts
   
	pwmcount = 0;
	   
  	return 0;
    
    
}

void InitTimer0 (void)
{ 
	// Initialize timer 0 for ISR 'pwmcounter()' below
	TR0=0; // Stop timer 0
	TMOD=0x01; // 16-bit timer
	// Use the autoreload feature available in the AT89LP51RB2
	// WARNING: There was an error in at89lp51rd2.h that prevents the
	// autoreload feature to work.  Please download a newer at89lp51rd2.h
	// file and copy it to the crosside\call51\include folder.
	TH0=TIMER0_RELOAD_VALUE/0x100;
	TL0=TIMER0_RELOAD_VALUE%0x100;
	TR0=1; // Start timer 0 (bit 4 in TCON)
	ET0=1; // Enable timer 0 interrupt
	EA=1;  // Enable global interrupts
}

void pwmcounterz ( void ) interrupt 1
{
	if(++pwmcount>99) pwmcount=0;
	P3_2=(pwm1>pwmcount)?1:0;
	P3_3=(pwm2>pwmcount)?1:0;
	P3_4=(pwm3>pwmcount)?1:0;
	P3_5=(pwm4>pwmcount)?1:0;

}

void delay( void )
{
	unsigned int a,b,c,d,e,f,g;
	for( a = 0; a < 10000; a++);
		for( b = 0; b < 10000; b++);
			for( c = 0; c < 10000; c++);
				for( d = 0; d < 10000; d++);
					for( e = 0; e < 10000; e++);
						for( f = 0; f < 10000; f++);
							for( g = 0; g < 10000; g++);

}

//for the transmission in the remote

/*void tx_byte ( unsigned char val )
{
	unsigned char j;
	//Send the start bit
	txon=0;
	wait_bit_time();
	for (j=0; j<8; j++)
	{
		txon=val&(0x01<<j)?1:0;
		wait_bit_time();
	}
	txon=1;
	//Send the stop bits
	wait_bit_time();
	wait_bit_time();
}*/

//for the reception in the car

unsigned char rx_byte ( int min )
{
	unsigned char j, val;
	int volt_zero;

	//Skip the start bit
	val=0;
	wait_one_and_half_bit_time();
	for(j=0; j<8; j++)
	{
		volt_zero = GetADC(0);
		val|=(volt_zero>min)?(0x01<<j):0x00;
		wait_bit_time();
		
	}
	//Wait for stop bits
	wait_one_and_half_bit_time();
	return val;
}


void wait_bit_time(void)
{
	_asm	
	    mov R2, #2
	L3:	mov R1, #30
	L2:	mov R0, #22
	L1:	djnz R0, L1 ; 2 machine cycles-> 2*0.5425347us*22=23.9us
	    djnz R1, L2 ; 23.9us*30=717us
	    djnz R2, L3 ; 717us*2=1.434ms
	    ret
    _endasm;    
}


void wait_one_and_half_bit_time(void)
{
	_asm	
	    mov R2, #3
	L_3:mov R1, #30
	L_2:mov R0, #22
	L_1:djnz R0, L_1 ; 2 machine cycles-> 2*0.5425347us*22=23.9us
	    djnz R1, L_2 ; 23.9us*30=717us
	    djnz R2, L_3 ; 717us*3=2.151ms
	    ret
    _endasm;    
}

void SPIWrite(unsigned char value)
{
	SPSTA&=(~SPIF); // Clear the SPIF flag in SPSTA
	SPDAT=value;
	while((SPSTA & SPIF)!=SPIF); //Wait for transmission to end
}

unsigned int GetADC(unsigned char channel)
{
	unsigned int adc;

	// initialize the SPI port to read the MCP3004 ADC attached to it.
	SPCON&=(~SPEN); // Disable SPI
	SPCON=MSTR|CPOL|CPHA|SPR1|SPR0|SSDIS;
	SPCON|=SPEN; // Enable SPI
	
	P1_4=0; // Activate the MCP3004 ADC.
	SPIWrite(channel|0x18);	// Send start bit, single/diff* bit, D2, D1, and D0 bits.
	for(adc=0; adc<10; adc++); // Wait for S/H to setup
	SPIWrite(0x55); // Read bits 9 down to 4
	adc=((SPDAT&0x3f)*0x100);
	SPIWrite(0x55);// Read bits 3 down to 0
	P1_4=1; // Deactivate the MCP3004 ADC.
	adc+=(SPDAT&0xf0); // SPDR contains the low part of the result. 
	adc>>=4;
		
	return adc;
}

//get the volatage from ch0 in ADC and change it accordingly.
float findvoltage0( void )
{
	return (float) (GetADC(0)*(UP/DOWN));
}
	
//gets the volatage from ch1 in ADC and changes accordingly	
float findvoltage1( void )
{
	return (float) (GetADC(1)*(UP/DOWN));
}

void same ( void )
{
	if( findvoltage0() < ( findvoltage1() + ERROR ) && findvoltage0() > ( findvoltage1() - ERROR )  )
	{
		if( findvoltage0() < (DISTANCE - ERROR) && findvoltage1() < (DISTANCE - ERROR) )
				{
						InitTimer0();
						pwm1 = 0;
						pwm2 = 100;
						pwm3 = 100;
						pwm4 = 0;
				}
					 if (findvoltage0() >= (DISTANCE - ERROR) && findvoltage0() < (DISTANCE + ERROR) && findvoltage1() < (DISTANCE + ERROR) && findvoltage1() >= (DISTANCE - ERROR)) 
					{
						InitTimer0();
						pwm1 = 0;
						pwm2 = 0;
						pwm3 = 0;
						pwm4 = 0;
					}
				}
	    if( findvoltage0() > (DISTANCE + ERROR) && findvoltage1() > (DISTANCE + ERROR) )
				{
						InitTimer0();
						pwm1 = 100;
						pwm2 = 0;
						pwm3 = 0;
						pwm4 = 100;		
				}
				if (findvoltage0() >= (DISTANCE - ERROR) && findvoltage0() < (DISTANCE + ERROR) && findvoltage1() < (DISTANCE + ERROR) && findvoltage1() >= (DISTANCE - ERROR)) 
					{
						InitTimer0();
						pwm1 = 0;
						pwm2 = 0;
						pwm3 = 0;
						pwm4 = 0;
					}
}

void turn_right(void)
{	
	//if( GetADC(0) > ( GetADC(1) + ERROR ) || GetADC(0) < ( GetADC(1) - ERROR ))
	
		//if( GetADC(0) > ( GetADC(1) + ERROR ) )
		//{			
			InitTimer0();
			pwm1 = 100;
			pwm2 = 0;
			pwm3 = 0;
			pwm4 = 0;
		//}	
		if (GetADC(0) >= (DISTANCE - ERROR) && GetADC(0) < (DISTANCE + ERROR) && GetADC(1) < (DISTANCE + ERROR) && GetADC(1) >= (DISTANCE - ERROR)) 
		{
			InitTimer0();
			pwm1 = 0;
			pwm2 = 0;
			pwm3 = 0;
			pwm4 = 0;
		}
					
	
}

void turn_left(void)
{
			
			InitTimer0();
			pwm1 = 0;
			pwm2 = 0;
			pwm3 = 0;
			pwm4 = 100;
		
		
		if (findvoltage0() >= (DISTANCE - ERROR) && findvoltage0() < (DISTANCE + ERROR) && findvoltage1() < (DISTANCE + ERROR) && findvoltage1() >= (DISTANCE - ERROR)) 
		{
			InitTimer0();
			pwm1 = 0;
			pwm2 = 0;
			pwm3 = 0;
			pwm4 = 0;
		}
			
}

void main(void) //add in errors??
{
	
  while(1)
  {
	v0 = GetADC(0);
	v1 = GetADC(1);
	
	if (v0 < min )
	 {  
	 	rx_byte(min);
	 	
	 	if (val == moveforward)
	 	{
	 	 if ( distance == d1 )
	 	  {
	 	  	distance = d1;
	 	  }
	 	  if ( distance == d2 )
	 	  {
	 	  	distance = d1;
	 	  }
	 	  if ( distance == d3 )
	 	  {
	 	  	distance = d2;
	 	  }
	 	  if ( distance == d4 )
	 	  {
	 	  	distance = d3;
	 	  }
	 	} 
	 	if( val == moveback )
	 	 {
	 	  if ( distance == d1 )
	 	  	{
	 	  	distance = d2;
	 	 
	 	  	} 
	 	  	if ( distance == d2 )
	 	  	{
	 	  	distance = d3;
	 	 
	 	  	} 
	 	  	if ( distance == d3 )
	 	  	{
	 	  	distance = d4;
	 	 
	 	  	}
	 	  	 if ( distance == d4 )
	 	  	{
	 	  	distance = d4;
	 	 
	 	  	}
	 	  }	
	 	  if (val == turn180)
	 	  	{
	 			turn180s();
	 		}
	 	  if (val == parallel)
	 	    {
	 	    	parallels();
	 	    }
	 	    	
	}

	if (v0 == v1)
	{
		if (v0&&v1 > distance)
		{
		move_closer();
		}
		if (v0&&v1 < distance)
		{
		move_further();
		}
		if (v0&&v1 == distance)
		{
		stop();
		}
		
	}
	
	if (v0 > v1)
	{
	   turn_left();
	}
	
	if (v0 < v1)
	{
	   turn_right();	
	}
   
}  	
}
void move_closer(void)
{
	pwm1 = 100;
	pwm2 = 0;
	pwm3 = 100;
	pwm4 = 0;
		
}

void move_further(void)
{
 	pwm1 = 0;
 	pwm2 = 100;
 	pwm3 = 0;
 	pwm4 = 100;
	
}

void stop(void)
{
	pwm1 = 0;
 	pwm2 = 0;
 	pwm3 = 0;
 	pwm4 = 0;
}	

void turn180s(void)
{
}
void parallels(void)
{
}
