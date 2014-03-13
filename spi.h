/* spi.h
 * Author: Perry Yan
 * Description: use this to unify all the separate .c and .h files
 */

#include "main.h"

// Function prototypes
void SPI_Startup(void);
void SPIWrite(unsigned char value);
unsigned int GetADC(unsigned char channel);
float voltage (unsigned char channel);