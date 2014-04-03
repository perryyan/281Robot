#include <stdio.h>
#include <at89lp51rd2.h>

// ~C51~ 
 
#define CLK 22118400L
#define BAUD 115200L
#define BRG_VAL (0x100-(CLK/(32L*BAUD)))

//We want timer 0 to interrupt every 100 microseconds ((1/10000Hz)=100 us)
#define FREQ 10000L
#define FREQ1 20L

#define TIMER0_RELOAD_VALUE (65536L-(CLK/(12L*FREQ)))
#define TIMER1_RELOAD_VALUE (65536L-(CLK/(12L*FREQ1)))
#define TIMER_2_RELOAD (0x10000L-(CLK/(32L*BAUD)))

#define BATTERY 5.16
#define BATTERY_MULTIPLIER (36000/(BATTERY*95/100)/(BATTERY*95/100)/1000)

          //communicator definitions
//#define UP 4.999999999L
//#define DOWN 1024.0000000000L


//These variables are used in the ISR
volatile unsigned char pwmcount;//only used by interrupt
volatile unsigned char pwm1,pwm2;//used by the interrupt
char motormode = 0;//used by the interrupt
volatile char motorautostop = 0;
volatile int motordelay = 0;//incremented by the interrupt, so it's global
volatile int motordelaysize = 0;
char accelerationvalue[10];

// RECEIVER CODE
float voltage;
char channel;
int min;
unsigned char moveback = 0b000011101;//29
unsigned char turn180 = 0b00111111;//63
unsigned char moveforward = 0b00000011;//3
unsigned char parallel = 0b00100001;//33
unsigned char val;
float volts0=0,volts1=0;
void wait_bit_time(void);
void wait_one_and_half_bit_time(void);
unsigned int GetADC(unsigned char channel);
void SPIWrite(unsigned char value);

void move_closer(void);
void move_further(void);
void stop(void);
void turn180s(void);
void parallels(void);

float d1 = 500;
float d2 = 350;
float d3 = 200;
float d4 = 100;

float distance = 500;


//RECEIVER CODE END

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


    // Initialize the serial port and baud rate generator
    PCON|=0x80;
	SCON = 0x52;
    BDRCON=0;
    BRL=BRG_VAL;
    BDRCON=BRR|TBCK|RBCK|SPD;
	
	// Initialize timer 0 for ISR 'pwmcounter()' below
	TR0=0; // Stop timer 0
	TR1=0;
	TMOD=0x01; // 16-bit timer (also timer 1, otherwise = 0x01)
	// Use the autoreload feature available in the AT89LP51RB2
	// WARNING: There was an ERROR in at89lp51rd2.h that prevents the
	// autoreload feature to work.  Please download a newer at89lp51rd2.h
	// file and copy it to the crosside\call51\include folder.
	TH0=RH0=TIMER0_RELOAD_VALUE/0x100;
	TL0=RL0=TIMER0_RELOAD_VALUE%0x100;

	TH1=RH1=TIMER1_RELOAD_VALUE/0x100;
	TL1=RL1=TIMER1_RELOAD_VALUE%0x100;
	//TH1=RH1=TIMER0_RELOAD_VALUE/0x100;
	//TL1=RL1=TIMER0_RELOAD_VALUE%0x100;
	TR0=0; // Don't start timer 0 (bit 4 in TCON)
	TR1=0;
	ET0=1; // Enable timer 0 interrupt
	ET1=0;
	EA=1;  // Enable global interrupts
	
	pwmcount=0;
    
    return 0;
}

//RECEIVER CODE
unsigned char rx_byte (void)
{
	unsigned char j;
	int volt_zero;
	
	while(GetADC(0)<=min);
	wait_bit_time();
	wait_bit_time();

	//Skip the start bit
	val=0;
	//wait_one_and_half_bit_time();
	for(j=0; j<8; j++)
	{
		volt_zero = GetADC(0);
		val|=(volt_zero>min)?(0x01<<j):0x00;
		wait_bit_time();
		
	}
	//printf("Last Command = %u \n",val);
	return val;
}
void wait_bit_time(void)
{
	_asm	
	    mov R2, #20
	L3:	mov R1, #30
	L2:	mov R0, #20
	L1:	djnz R0, L1 ; 2 machine cycles-> 2*0.5425347us*22=23.9us
	    djnz R1, L2 ; 23.9us*30=717us
	    djnz R2, L3 ; 717us*2=1.434ms
	    ret
    _endasm;    
}
void wait_one_and_half_bit_time(void)
{
	_asm	
	    mov R2, #30
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
/*//get the volatage from ch0 in ADC and change it accordingly.
float findvoltage0( void )
{
	return (float) (GetADC(0)*(UP/DOWN));
}	
//gets the volatage from ch1 in ADC and changes accordingly	
float findvoltage1( void )
{
	return (float) (GetADC(1)*(UP/DOWN));
}
*/
//RECEIVER CODE END

void millisecdelay(float millisecondstodelay,char stopautomatically,char batterydependent)
{

	if(batterydependent)
		millisecondstodelay = millisecondstodelay*BATTERY_MULTIPLIER;
	if(!stopautomatically)
	{	
		motordelay = 1;
		while(motordelay<millisecondstodelay);
		motordelay = 0;
	}
	else
	{
		motorautostop = 1;
		motordelaysize = (millisecondstodelay)?millisecondstodelay:10;
		motordelay = 1;
	}
}
void motorcontrol(char motorstate,char motorspeed,char rate,char acceleration)
{	
	//motorstate:
	//  - from 0 to 8 it's the same as motormode
	//  - 9 is turn right, and 10 is turn left, both wheels moving forward
	//  - 11 is turn right, and 12 is turn left, both wheels moving backward
	//setting the acceleration controls how fast the car speeds up
	//whether in the turn, or going straight. Setting acceleration in a
	//completely forward or reverse turn also controls the tightness of the turn
	//for a tight turn, it's better to use one wheel stopped, or rotation
	//setting the motorspeed sets the car's speed
	//motorspeed and rate are both out of 100

	/*
	NOTES: make an array of pwm values that are calculated beforehand, so that the pwm can be set quickly
	*/
	char i;
	char accstate;
	int accdividevalue = (101-acceleration);
	if(rate>100)
		rate = 100;
	if(motorspeed>100)
		motorspeed = 100;
	if(acceleration>100)
		acceleration = 100;
	if(motorstate<=8&&motorstate>=0)
	{
		if(acceleration)
			for(i = 0;i<10;i++)
				accelerationvalue[i] = (5+i)*motorspeed/14;
		else
			pwm1 = pwm2 = motorspeed;
		motormode = motorstate;
		if(motordelay)
			motordelay = 1;
		if(acceleration)
			for(accstate=0;accstate<10&&motordelay;)
			{	
				pwm1 = pwm2 = accelerationvalue[accstate];
				accstate=motordelay/accdividevalue;
			}
	}
	else if(motorstate==9)//curve right(forward)
	{
		motormode = 1;
		pwm1 = motorspeed;
		pwm2 = motorspeed*(100-rate)/100;
	}
	else if(motorstate==10)//curve left(forward)
	{
		motormode = 1;
		pwm1 = motorspeed*(100-rate)/100;
		pwm2 = motorspeed;
	}
	else if(motorstate==11)//curve right(reverse)
	{
		motormode = 2;
		pwm1 = motorspeed;
		pwm2 = motorspeed*(100-rate)/100;
	}
	else if(motorstate==12)//curve left(reverse)
	{
		motormode = 2;
		pwm1 = motorspeed*(100-rate)/100;
		pwm2 = motorspeed;
	}
	else
		motormode = 0;
}

void motordecelerate(void)
{
	motorcontrol(1,40,0,0);
	millisecdelay(100,0,1);
	motorcontrol(1,30,0,0);
	millisecdelay(100,0,1);
	motorcontrol(1,15,0,0);
	millisecdelay(100,0,1);
	motormode = 0;
}
void parallelpark0(char moveforward,int movetotime,int movealongtime)
{
	//values for left side of the car's body lined up with the back of the car in front
	//and one hand width from the car in front (also moveforward = 1)
	//movetotime = 100, movealongtime = 500
	if(moveforward&&movetotime)
	{	millisecdelay(movetotime,1,1);
		motorcontrol(1,50,0,70);
		while(motormode);
	}
	else if(movetotime)
	{	millisecdelay(movetotime,1,1);
		motorcontrol(2,50,0,70);
		while(motormode);
	}

	millisecdelay(1470,1,1);
	motorcontrol(10,75,70,0);
	while(motormode);
	
	motorcontrol(1,40,0,0);
	millisecdelay(movealongtime,0,1);

	motordecelerate();
}
void parallelpark1(void)
{

	millisecdelay(1100,1,1);
	motorcontrol(2,40,0,80);//start moving backwards
	while(motormode);

	
	millisecdelay(200,1,1);
	motorcontrol(11,40,40,0);//curve right backwards a little
	while(motormode);

	millisecdelay(100,1,1);
	motorcontrol(11,40,50,0);
	while(motormode);

	millisecdelay(200,1,1);
	motorcontrol(11,40,55,0);
	while(motormode);

	millisecdelay(200,1,1);
	motorcontrol(11,40,65,0);
	while(motormode);

	millisecdelay(370,1,1);
	motorcontrol(11,40,70,0);//curve right backwards a lot
	while(motormode);

	millisecdelay(150,1,1);
	motorcontrol(11,40,65,0);
	while(motormode);

	millisecdelay(150,1,1);
	motorcontrol(11,40,55,0);
	while(motormode);

	millisecdelay(100,1,1);
	motorcontrol(11,40,50,0);
	while(motormode);

	millisecdelay(100,1,1);
	motorcontrol(11,40,40,0);//curve right backwards a little
	while(motormode);


	
	millisecdelay(1200,1,1);
	motorcontrol(2,40,0,0);//move backwards
	while(motormode);

	millisecdelay(250,1,1);
	motorcontrol(12,40,40,0);//curve left backwards a little


	while(motormode);
	millisecdelay(300,1,1);
	motorcontrol(12,40,60,0);
	while(motormode);
	millisecdelay(570,1,1);
	motorcontrol(12,40,80,0);//curve left backwards a lot
	while(motormode);
	millisecdelay(250,1,1);
	motorcontrol(12,40,55,0);
	while(motormode);
	millisecdelay(100,1,1);
	motorcontrol(12,40,40,0);//curve left backwards a little
	while(motormode);
	
	millisecdelay(800,1,1);
	motorcontrol(1,50,0,70);
	while(motormode);

	motordecelerate();
	
}
void rotatedegreescw100(float degrees)
{
	float rotatedegrees = 1730*degrees/360;//multiplication comes first because 8052 is bad at float math
	millisecdelay(rotatedegrees,1,1);
	motorcontrol(7,100,0,0);
	while(motormode);
}
void rotatedegreescw50(float degrees)
{
	float rotatedegrees = 3750*degrees/360;//multiplication comes first because 8052 is bad at float math
	millisecdelay(rotatedegrees,1,1);
	motorcontrol(7,50,0,0);
	while(motormode);
}

// Interrupt 1 is for timer 0.  This function is executed every time
// timer 0 overflows: 100 µs. Controls the pwm for both motors

// motormode:
//	=0,stopped
//	=1,forward
//	=2,reverse
//	=3,turn right on right wheel(forward)
//	=4,turn left on left wheel(forward)
//	=5,turn right on left wheel(reverse)
//	=6,turn left on right wheel(reverse)
//	=7,spin right
//	=8,spin left

// motorstate only:
//  =9,curve right(forward)
//  =10,curve left(forward)
//  =11,curve right(reverse)
//  =12,curve left(reverse)

void pwmcounter (void) interrupt 1
{
	if(++pwmcount>99) pwmcount=0;
	P2_0=(pwm1>pwmcount&&(motormode==2||motormode==6||motormode==8))?0:1;//left motor reverse
	P2_1=(pwm1>pwmcount&&(motormode==1||motormode==3||motormode==7))?0:1;//left motor forward
	
	P2_2=(pwm2>pwmcount&&(motormode==2||motormode==5||motormode==7))?0:1;//right motor reverse 
	P2_3=(pwm2>pwmcount&&(motormode==1||motormode==4||motormode==8))?0:1;//right motor forward

	if(pwmcount%10==0&&motordelay>0)
		motordelay++;
	if(motorautostop&&motordelay>motordelaysize)
	{	motordelay = 0;
		motordelaysize = 0;
		motormode = 0;
		motorautostop = 0;	}
	if(!motormode)
		pwm1 = pwm2 = 0;
}


void lowbuzz(char time)
{
	int i;
	int timecompare = time*5;
	int j;
	TR0 = 0;
	for(i=0;i<timecompare;i++)
	{
		for(j=0;j<400;j++);
		P0_0 = !P0_0;
	}
	TR0 = 1;
}
void deepbuzz(char time)
{
	int i;
	int timecompare = time*5;
	int j;
	TR0 = 0;
	for(i=0;i<timecompare;i++)
	{
		for(j=0;j<600;j++);
		P0_0 = !P0_0;
	}
	TR0 = 1;
}
void scalebuzz(char time)
{
	int i;
	float timecompare = time*5;
	float notecompare = 800;
	float notecompareadd = notecompare/timecompare;
	int j;
	TR0 = 0;
	for(i=0;i<timecompare;i++)
	{
		for(j=0;j<notecompare&&notecompare>0;j++);
		P0_0 = !P0_0;
		notecompare = notecompare-notecompareadd;
	}
	TR0 = 1;
}
void nobuzz(char time)
{
	int i;
	int timecompare = time*5;
	int j;
	for(i=0;i<timecompare;i++)
		for(j=0;j<200;j++);
}
void errorbuzz(void)
{
	lowbuzz(20);
	nobuzz(5);
	lowbuzz(20);
	nobuzz(5);
	lowbuzz(20);
	nobuzz(5);
}
void honkbuzz(void)
{
	lowbuzz(50);
	nobuzz(10);
	lowbuzz(90);
	nobuzz(2);
}

void buttoncommands(void)
{	 	
	
	if(val == moveforward)
	{
		if(distance==d1)
	 	  	distance = d1;
	 	if(distance==d2)
			distance = d1;
	 	if(distance==d3)
	 	  	distance = d2;
	 	if(distance==d4)
	 	  	distance = d3;
		printf("\x1B[9;1H\x1B[0Kbutton1 pushed");
	} 
	else if(val == moveback)
	{
		if(distance==d1)
	 	  	distance = d2;
	 	if(distance==d2)
	 	  	distance = d3;
	 	if(distance==d3)
	 	  	distance = d4;
	 	if(distance==d4)
	 	  	distance = d4;
		printf("\x1B[9;1H\x1B[0Kbutton2 pushed");
	}	
	else if (val == turn180)
	{
		TR0 = 1;
		printf("\x1B[9;1H\x1B[0Kbutton3 pushed");
	 	rotatedegreescw50(180);
		TR0 = 0;
	}
	else if (val == parallel)
	{
		TR0 = 1;
		printf("\x1B[9;1H\x1B[0Kbutton4 pushed");
		parallelpark1();
		TR0 = 0;
	}
	 	    	
}
void fixposition(float difference,int ERROR)
{
	char i;
	TR0 = 1;
	
	if(((volts0<distance/5&&difference>40)||difference>15)&&volts0>volts1)
		motorcontrol(7,70,0,0);
	else if(((volts0<distance/5&&difference>40)||difference>15)&&volts1>volts0)
		motorcontrol(8,70,0,0);
	else if(difference<15)//if pretty much facing the remote
	{	if(volts0>distance+ERROR)//if too close
			motorcontrol(2,70,0,0);
		else if(volts0<distance-ERROR)//if too far
			motorcontrol(1,70,0,90);
		else
			motormode = 0;
	}

	/*
	else if((volts0>volts1)&&volts0>distance+ERROR)//if left is closer and car is too close
	{	motorcontrol(11,70,70,0);
		
		printf("\x1B[3;1H\x1B[0Kgo back right");	}
	else if((volts0>volts1)&&volts0<distance-ERROR)//left closer and car too far
	{	motorcontrol(10,70,70,0);
		
		printf("\x1B[3;1H\x1B[0Kgo forward left");	}
	else if((volts1>volts0)&&volts1>distance+ERROR)//right closer and car too close
	{	motorcontrol(12,70,70,0);
		
		printf("\x1B[3;1H\x1B[0Kgo back left");	}
	else if((volts1>volts0)&&volts1<distance+ERROR)//right closer and car too far
	{	motorcontrol(9,70,70,0);
		
		printf("\x1B[3;1H\x1B[0Kgo forward right");	}

	*/
	else
		motormode = 0;
	for(i = 0;i<100;i++)
		if(GetADC(0)<min)
			return;

	TR0 = 0;




}

void main (void)
{
	float difference;
	printf("\x1B[2J");//clears screen
	printf( "Project 2 Motor Control Running...\n" );
	TR0=0;
	

	while(1)
	{
		min = 5;
		if (GetADC(0) <= min )
		{	rx_byte();
		printf("\x1B[6;1H\x1B[0Kval = %d",val);
			buttoncommands();
		}
		else
		{
			volts0 = GetADC(0);
			volts1 = (GetADC(1)+15)*0.9;
			if(volts0>volts1)
				difference = (volts0 - volts1)/(volts1+volts0)*100;
			else
				difference = (volts1 - volts0)/(volts1+volts0)*100;
			

			fixposition(difference,10);
			printf("\x1B[2;1H%%difference = %.2f\x1B[4;1Hv0 = \x1B[0K%.2f\x1B[5;1Hv1 = \x1B[0K%.2f",difference,volts0,volts1);
		}


		
		


		

		








	}
}