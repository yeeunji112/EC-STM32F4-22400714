/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : [ YOUR NAME GOES HERE !!!!!]
Created          : 10-08-2026
Modified         : 00-00-2026 [WRITE THE DATE!!!!]
Language/ver     : C in VS Code

Description      : Tutorial - USART2 with the STUDENT library (ecUART2_student)
/----------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>

#include "stm32f4xx.h"
#include "ecGPIO2.h"
#include "ecRCC2.h"
#include "ecSysTick2.h"
#include "ecUART2_student.h"

uint8_t PC_string[] = "Hello Handong\r\n";
uint8_t BT_string[20]={0};

uint32_t loop=0;

// Initialiization 
void setup(void)
{
	RCC_PLL_init();
	SysTick_init();
	USART2_init();
}

int main(void) {
	// Initialiization --------------------------------------------------------
	setup();

	// Method 1 : printf() 
	printf("MCU Initialized\r\n");

	// Method 2 : USART_write() 
	USART2_write(PC_string, sizeof(PC_string));

	// Inifinite Loop ----------------------------------------------------------
	while (1){
		// Method 3 :  Combination of string and number
		int n = sprintf((char*)BT_string, "Loop: %lu >> \r\n", loop);  
		USART2_write(BT_string, n);		
		loop++;
		delay_ms(2000);  
	}
}
