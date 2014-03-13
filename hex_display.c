/* hex_display.c 
 * Author: Perry Yan, Derek Zellmann
 * Description: a collection of functions to write to hex displays.  
 * Originally used for Lab 4, EECE 281
 */

#include "hex_display.h"
 
// Pick the hex display and turn it on
void TurnOnDigit(int which) {
	switch(which) {
		case 0:
			P2_0 = 0;
			P2_1 = 1;
			P2_2 = 1;
			break;
		case 1:
			P2_0 = 1;
			P2_1 = 0;
			P2_2 = 1;
			break;
		case 2:
			P2_0 = 1;
			P2_1 = 1;
			P2_2 = 0;
			break;
		default:
			P2_0 = 1;
			P2_1 = 1;
			P2_2 = 1;
			break;
	}
	return;
}

// This function shows an ASCII digit (0..9) on the hex displays
void showOnHex (int digit) {
	switch(digit) {
		case 0:
			setBCDBySeg(0,0,0,0,0,0,1,1);
			break;
		case 1:
			setBCDBySeg(1,0,0,1,1,1,1,1);
			break;
		case 2:
			setBCDBySeg(0,0,1,0,0,1,0,1);		
			break;
		case 3:
			setBCDBySeg(0,0,0,0,1,1,0,1);		
			break;
		case 4:
			setBCDBySeg(1,0,0,1,1,0,0,1);
			break;						
		case 5:
			setBCDBySeg(0,1,0,0,1,0,0,1);
			break;
		case 6:
			setBCDBySeg(0,1,0,0,0,0,0,1);
			break;
		case 7:
			setBCDBySeg(0,0,0,1,1,1,1,1);
			break;
		case 8:
			setBCDBySeg(0,0,0,0,0,0,0,1);
			break;
		case 9:
			setBCDBySeg(0,0,0,0,1,0,0,1);
			break;			
		default:
			setBCDBySeg(1,1,1,1,1,1,1,1);
			break;
	}
	return;
}

// Set the individual hex display segments 
void setBCDBySeg(int a, int b, int c, int d, int e, int f, int g, int p) {
	P1_0 = a;
	P1_1 = b;
	P1_2 = c;
	P1_3 = d;
	P1_4 = e;
	P1_5 = f;
	P1_6 = g;
	P1_7 = p;
	return;
}				
		