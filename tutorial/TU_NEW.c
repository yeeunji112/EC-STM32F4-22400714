#include "ecSTM32F4v2.h"
// this should include #include "ecUART2.h" 
//#include "ecUART2.h" 

static volatile uint8_t PC_Data = 0;
static volatile uint8_t BT_Data = 0;
uint8_t PC_string[]="Hi_BT\r\n";
uint8_t BT_string[]="BT\r\n";

void setup(void){
	RCC_PLL_init();
	SysTick_init();
	
	// USART1: BT serial init 
	USART1_init();
	USART2_init();

	USART1_baud(BAUD_9600);
}

int main(void){	
	setup();
	//printf("MCU Initialized\r\n");
	//USART1_write(&BT_string,4);
	//USART1_write(BT_string,4);
	while(1){
		// USART Receive: Use Interrupt only
		// USART Transmit:  Interrupt or Polling
		USART1_write(PC_string, 7);
		delay_ms(2000);        
	}
}