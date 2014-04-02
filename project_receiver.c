#include <stdio.h>
#include <at89lp51rd2.h>

#define CLK 22118400L
#define BAUD 115200L 
#define BRG_VAL (0x100-(CLK/(32L*BAUD)))
#define TIMER_2_RELOAD (0x10000L-(CLK/(32L*BAUD)))
#define DISTANCE 0.750000L
#define ZERO 0.00000L
#define UP 4.999999999L
#define DOWN 1024.0000000000L
#define ERROR 0.0400000L
#define FIFTY 0.500000L //these need to be fixed
#define THIRTYFIVE 0.350000L
#define TWENTY 0.200000L
#define TEN 0.100000L
#define MIN 0.380000L

//int txon;
float voltage;
char channel;
float v0;
float v1;
int min = MIN;
unsigned char moveback = 000000010;
unsigned char moveforward = 00000001;
unsigned char turn180 = 00000011;
unsigned char parallel = 00000100;
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

float d1 = 0.42;
float d2 = 0.785;
float d3 = 1.92;
float d4 = 3.2;

float distance = 1.92;

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
   
	   
  	return 0;
    
    
}






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

void turn_right(void)
{	
//		if (findvoltage0() >= (DISTANCE - ERROR) && findvoltage0() < (DISTANCE + ERROR) && findvoltage1() < (DISTANCE + ERROR) && findvoltage1() >= (DISTANCE - ERROR))
		{
			//turn right
		}
}

void turn_left(void)
{
//		if (findvoltage0() >= (DISTANCE - ERROR) && findvoltage0() < (DISTANCE + ERROR) && findvoltage1() < (DISTANCE + ERROR) && findvoltage1() >= (DISTANCE - ERROR)) 
		{
			//turn left
		}
			
}

void main(void)
{
	
  while(1)
  {
	v0 = (GetADC(0)*(UP/DOWN));
	v1 = (GetADC(1)*(UP/DOWN));
	//printf("v0 = %.6f, v1 = %.6f \n",v0,v1);
	printf("Last Command = %u \n",val);
	if (v0 < min )
	 {  

	 	rx_byte(min);
	 	
	 	
	 	if (val == moveforward)
	 	{
	 		if ( distance >= (d1 - ERROR) || distance <= (d1 +ERROR) )
	 	  		distance = d1;
	 		if ( distance >= (d2 - ERROR) || distance <= (d2 +ERROR) )
		 	  	distance = d1;
	 		if ( distance >= (d3 - ERROR) || distance <= (d3 +ERROR) )
		 	  	distance = d2;
	 		if ( distance >= (d4 - ERROR) || distance <= (d4 +ERROR) )
		 	  	distance = d3;
	 	} 

	 	if( val == moveback )
	 	 {
	 		if ( distance >= (d1 - ERROR) || distance <= (d1 +ERROR) )
	 			distance = d2;
	 	  	if ( distance >= (d2 - ERROR) || distance <= (d2 +ERROR) )
	 	  		distance = d3;
	 	  	if ( distance >= (d3 - ERROR) || distance <= (d3 +ERROR) )
	 	  		distance = d4;
	 	  	if ( distance >= (d4 - ERROR) || distance <= (d4 +ERROR) )
	 	  		distance = d4;
	 	  }	

	 	  if (val == turn180)
	 			turn180s();
	 	  if (val == parallel)
	 	    	parallels();
	 	    	
	}

	if (v0 == v1)
	{
		if (v0&&v1 > (distance - ERROR))
			move_closer();
		if (v0&&v1 < (distance + ERROR))
			move_further();
		if (v0&&v1 == (distance +ERROR) || v0&&v1 == (distance - ERROR) )
			stop();
		
	}
	
	if (v0 > v1)
	   turn_left();
	
	if (v0 < v1)
	   turn_right();
   
}  	
}
void move_closer(void)
{
	//move closer
}

void move_further(void)
{
	//move farther	
}

void stop(void)
{
	//stop
}	

void turn180s(void)
{
}
void parallels(void)
{
}
