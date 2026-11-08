#include "ecSTM32F4v2.h"// this should include #include "ecUART2.h" 
//#include "ecUART2.h" 

//one character만 
static volatile uint8_t PC_Data = 0;
static volatile uint8_t BT_Data = 0;
uint8_t PC_string[]="Loop:\r\n";
uint8_t BT_string[]="BT\r\n";

void setup(void){
	RCC_PLL_init();
	SysTick_init();
	
	// USART2: USB serial init
	USART2_init();  // default 9600
	//USART2_baud(BAUD_38400);
	
	// USART1: MCU2 serial init 
	USART1_init(); // default 9600
	//USART1_baud(BAUD_38400);
}

int main(void){	
	setup();
	printf("MCU Initialized 2\r\n");	
	USART1_write(&BT_string,4);
	while(1){
		// USART Receive: Use Interrupt only
		// USART Transmit: Polling
		//USART2_write(PC_string, 7);
		delay_ms(2000);        
	}
}

void USART2_IRQHandler(){       // USART2 RX Interrupt 
	if(is_USART2_RXNE()){
		PC_Data = USART2_read();		// RX from UART2 (PC)
		USART2_write(&PC_Data,1);		// TX to USART2	 (PC)	 Echo of keyboard typing
		USART1_write(&PC_Data,1);	  // TX to Other MCU
	}
}

void USART1_IRQHandler(){          	// USART2 RX Interrupt
	if(is_USART1_RXNE()){
		BT_Data = USART1_read();				// RX from other MCU UART1		
		printf("RX: %c \r\n",BT_Data); 	// TX to USART2(PC)
	}
}




///string 보내기 
// #include "ecSTM32F4v2.h"  // this should include #include "ecUART2.h"

// #define MAX_BUF 10
// #define END_CHAR 13

// static volatile uint8_t buffer[MAX_BUF]={0, };
// static volatile uint8_t PC_string[MAX_BUF]={0, };
// static volatile uint8_t PC_data = 0;

// static volatile int idx = 0;
// static volatile int bReceive =0;

// void setup(void){
//     RCC_PLL_init();
//     SysTick_init();

//     // USART2: USB serial init
//     USART2_init();
//     //USART2_baud(BAUD_38400);
//     // USART1: MCU2 serial init
//     USART1_init();
//     //USART1_baud(BAUD_38400);
// }

// int main(void){
//     setup();
//     printf("MCU Initialized\r\n");

//     while(1){
//         if (bReceive == 1){
//             printf("PC_string: %s\r\n", PC_string);
//             bReceive = 0;
//         }
//     }
// }

// void USART2_IRQHandler(){       // USART2 RX Interrupt
//     if(is_USART2_RXNE()){
//         PC_data = USART2_read();     // RX from UART2 (PC)
//         USART2_write(&PC_data,1);    // TX to USART2 (PC) Echo of keyboard typing

//         // Creates a String from received serial character
//         if(PC_data != END_CHAR && (idx < MAX_BUF)){
//             buffer[idx] = PC_data;
//             idx++;
//         }
//         else if (PC_data== END_CHAR) {
//             bReceive = 1;
//             // reset PC_string
//             memset(PC_string, 0, sizeof(char) * MAX_BUF);
//             // copy to PC_string
//             memcpy(PC_string, buffer, sizeof(char) * idx);
//             // reset buffer
//             memset(buffer, 0, sizeof(char) * MAX_BUF);
//             idx = 0;
//             USART1_write(PC_string,sizeof(PC_string)); // TX to other MCU
//         }
//         else{       // if(idx >= MAX_BUF)
//             idx = 0;
//             // reset PC_string
//             memset(PC_string, 0, sizeof(char) * MAX_BUF);
//             // reset buffer
//             memset(buffer, 0, sizeof(char) * MAX_BUF);
//             printf("ERROR : Too long string\r\n");
//         }
//     }
// }

// // Added: receive a string from the other MCU through PA9/PA10.
// void USART1_IRQHandler(){
//     if(is_USART1_RXNE()){
//         PC_data = USART1_read();
//         USART2_write(&PC_data,1); // Echo received character to local PC

//         if(PC_data != END_CHAR && (idx < MAX_BUF)){
//             buffer[idx] = PC_data;
//             idx++;
//         }
//         else if (PC_data == END_CHAR){
//             bReceive = 1;
//             memset(PC_string, 0, sizeof(char) * MAX_BUF);
//             memcpy(PC_string, buffer, sizeof(char) * idx);
//             memset(buffer, 0, sizeof(char) * MAX_BUF);
//             idx = 0;
//         }
//         else{
//             idx = 0;
//             memset(PC_string, 0, sizeof(char) * MAX_BUF);
//             memset(buffer, 0, sizeof(char) * MAX_BUF);
//             printf("ERROR : Too long string\r\n");
//         }
//     }
// }
