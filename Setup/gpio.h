/*  
    S1 = PE10
    S2 = PE11
    S3 = PE12
    LED1 = PE13
    LED2 = PE14
    LED3 = PE15
 */
#include "stm32f4xx.h"

void GPIO_init(void);
void vLedOff(uint8_t led);
void vLedOn(uint8_t led);
void vLedAllOff(void);
void vLedAllOn(void);