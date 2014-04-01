#include <stdio.h>
#include <Rohit.h>
#include <stdlib.h>
#include <string.h>

// ~C51~ 
 
#define CLK 22118400L
#define BAUD 115200L
#define BRG_VAL (0x100-(CLK/(32L*BAUD)))

//We want timer 0 to interrupt every 100 microseconds ((1/10000Hz)=100 us)
#define FREQ 15600L
#define TIMER0_RELOAD_VALUE (65536L-(CLK/(12L*FREQ)))

//These variables are used in the ISR
volatile unsigned char pwmcount;

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
	TR0=0; // Stop timer 0
	TMOD=0x01; // 16-bit timer
	TH0=RH0=TIMER0_RELOAD_VALUE/0x100;
	TL0=RL0=TIMER0_RELOAD_VALUE%0x100;
	TR0=1; // Start timer 0 (bit 4 in TCON)
	ET0=1; // Enable timer 0 interrupt
	EA=1;  // Enable global interrupts
	
	
	txon = 0;
	P2_0 = 0; 
	P2_1 = 1;
    
    return 0;
}

// Interrupt 1 is for timer 0.  This function is executed every time
// timer 0 overflows: 100 us.
void pwmcounter (void) interrupt 1
{

_asm
	cpl P2_0;
	cpl P2_1;
_endasm;	
	
}


void main (void)
{	

	if(PushButton0 = 1)
	{
	  val = //NUMBER;
	  //call txbyte;
	}

	else if(PushButton1 = 1)
	{
	  val = //NUMBER;
	  //call txbyte;
	}


	else if(PushButton2 = 1)
	{
	  val = //NUMBER;
	  //call txbyte;
	}


	else if(PushButton3 = 1)
	{
	  val = //NUMBER;
	  //call txbyte;
	}
	 