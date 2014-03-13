/* main.c
 * Author: Perry Yan
 * Description: Our main function
 */
 
#include "main.h"
#include "spi.c"
#include "hex_display.c"

unsigned char _c51_external_startup(void)
{
	// Configure ports as a bidirectional with internal pull-ups.
	P0M0=0;	P0M1=0;
	P1M0=0;	P1M1=0;
	P2M0=0;	P2M1=0;
	P3M0=0;	P3M1=0;
	AUXR=0B_0001_0001; // 1152 bytes of internal XDATA, P4.4 is a general purpose I/O
	P4M0=0;	P4M1=0;
	
	SPI_Startup(); // Initialize SPI (from spi.c)
    
    return 0;
}

void main(void) {
	while(1) {
		printf("Hello World!\r");
	}
}