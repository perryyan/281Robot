#include <stdio.h>
#include <at89lp51rd2.h>
#include <stdlib.h>
#include <string.h>

// ~C51~ 
 
#define CLK 22118400L
#define BAUD 115200L
#define BRG_VAL (0x100-(CLK/(32L*BAUD)))

//We want timer 0 to interrupt every 100 microseconds ((1/10000Hz)=100 us)
#define FREQ 31200L
#define TIMER0_RELOAD_VALUE (65536L-(CLK/(12L*FREQ)))

//These variables are used in the ISR
volatile unsigned char pwmcount;

int txon;
int PushButton0;
int PushButton1;
int PushButton2;
int PushButton3;

void wait_bit_time(void);
void wait_button(void);

unsigned char val;
unsigned char value;
unsigned char gee;


unsigned char _c51_external_startup(void)
{
	// Configure ports as a bidirectional with internal pull-ups.
	P0M0=0;	P0M1=0;
	P1M0=0;	P1M1=0;
	P2M0=0;	P2M1=0;
	P3M0=0;	P3M1=0;
	AUXR=0B_0001_0001; // 1152 bytes of internal XDATA, P4.4 is a general purpose I/O
	P4M0=0;	P4M1=0;

    // Initialize the serial port and baud rate generator
    PCON|=0x80;
	SCON = 0x52;
    BDRCON=0;
    BRL=BRG_VAL;
    BDRCON=BRR|TBCK|RBCK|SPD;
   
    
    
	// Initialize timer 0 for ISR 'pwmcounter()' below
	txon = 1;
	P2_0 = 0; 
	P2_1 = 1;
	P4_3 = 0;
	
	TR0=0; // Stop timer 0
	TMOD=0x01; // 16-bit timer
	TH0=RH0=TIMER0_RELOAD_VALUE/0x100;
	TL0=RL0=TIMER0_RELOAD_VALUE%0x100;
	TR0=1; // Start timer 0 (bit 4 in TCON)
	ET0=1; // Enable timer 0 interrupt
	EA=1;  // Enable global interrupts
    
    return 0;
}



// Interrupt 1 is for timer 0.  This function is executed every time
// timer 0 overflows: 100 us.
void pwmcounter (void) interrupt 1
{
	if(P2_0==1)
	{
		 P2_0 = 0;
		 P2_1 = 1;
	}
	else
	{
		P2_0=1;
		P2_1=0;
	}
}


void tx_byte ( unsigned char val )
{
	unsigned char j;
	//printf("val = %u \n",val);
	//Send the start bit
	TR0=0;
	wait_bit_time();
	for (j=0; j<8; j++)
	{
		TR0=val&(0x01<<j)?1:0;
		wait_bit_time();
	}
	TR0=1;
	//Send the stop bits
	wait_bit_time();
	wait_bit_time();
	
}

void wait_bit_time(void)
{
	_asm	
	    mov R2, #20
	L3:	mov R1, #30
	L2:	mov R0, #22
	L1:	djnz R0, L1 ; 2 machine cycles-> 2*0.5425347us*22=23.9us
	    djnz R1, L2 ; 23.9us*30=717us
	    djnz R2, L3 ; 717us*2=1.434ms
	    ret
    _endasm;    
}

void wait_button(void)
{
 	_asm
 	
 		mov R2, #2 	
	H3:	mov R1, #250
	H2:	mov R0, #184
	H1:	djnz R0, H1 ; 2 machine cycles-> 2*0.5425347us*22=23.9us
	    djnz R1, H2 ; 23.9us*30=717us
	    djnz R2, H3 ; 717us*2=1.434ms
	    ret
    _endasm; 

}

void main (void)
{	
while(1)
	{
		
		if(P2_7 == 1)
		{
		P4_3 =	1;
		wait_button();
		value = 0b00000011;
		tx_byte(value);
		  
		}
		
		else if(P2_6 == 1)
		{
		P4_3 = 1;
		wait_button();
		  value = 0b00001101;
		  tx_byte(value);
		  
		}
	
		else if(P2_5 == 1)
		{
		P4_3 = 1;
		wait_button();
		  value = 0b00010111;
		  tx_byte(value);
		  
		}
	
		else if(P2_4 == 1)
		{
		P4_3 = 1;
		wait_button();
		  value = 0b00100001;
		  tx_byte(value);
		  
		 }

		else
		{
		P4_3 = 0;
		}
	}
}	 
