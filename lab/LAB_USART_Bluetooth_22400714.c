/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : Yeeun Ji
Created          : 10-08-2026
Modified         : 10-08-2026
Language/ver     : C in VS Code

Description      : bluetooth cononection
/----------------------------------------------------------------*/


//check the bluetooth connection L이랑 h 를 pc에서 보냄으로써
//connect stepper motor 해서
//J면 cw direction
//k면 ccw direction
//p면 emergency stop 
#include "ecSTM32F4v2.h"


//stepper motor pin name definition
PinName_t pinStepper[4]    = {PB_10,PB_4,PB_5,PB_3};
//URAT receive data
static volatile uint8_t PC_Data = 0;
static volatile char BT_Data = 0;

//stepper motor control variables
volatile uint32_t steps; 
volatile uint32_t right = 0; 
volatile uint32_t left = 0;
volatile uint32_t stop;

//current motor direction : 0 = stop, 1 = CW, 2= CCW
volatile uint32_t dir = 0;


// Select stepping mode: FULL or HALF
uint32_t mode = FULL;

// ON-BOARD LED (LD2)
#define LED_PIN    PA_5 //LD2

void setup(void){
    //Initialize system clock (84Mhz) and SystickPLL 84MHZ
	RCC_PLL_init();
	SysTick_init();
	
	// USART2: USB serial init
    //No Parity, 8-bit Data, 1-bit Stop bit, 9600 baud-rate
	USART2_init();  // default 9600
	//USART2_baud(BAUD_38400);
	
	// USART1: MCU2 serial init 
	USART1_init(); // default 9600
	//USART1_baud(BAUD_38400);

     //LED PIN INTIALIZATION - FOR BLUTOOTH
    GPIO_init(LED_PIN, OUTPUT);    // calls RCC_GPIOB_enable()

	stepper_init(mode, pinStepper); 				// Stepper GPIO pin initialization
	stepper_speed(10);                 		//  set stepper motor speed(based on selected stepping mode

}



int main(void){	
	setup();

    
	printf("MCU Initialized\r\n");	 //display on stm terminal

    uint8_t BTin[]="MCU Initalization\r\n";
    USART1_write(BTin, sizeof(BTin)-1);// display on bluetooth terminal
	while(1){
		
     
	}
	
	
}

// USART2 RX interrupt: Read and discard unused PC input

void USART2_IRQHandler(){       // USART2 RX Interrupt 
	if(is_USART2_RXNE()){
		 USART2_read();		// RX from UART2 (PC)
		 
    }
}

// USART1 RX interrupt: Receive commands from Bluetooth
void USART1_IRQHandler(){          	
	if(is_USART1_RXNE()){          
		// Read a character received through Bluetooth
        BT_Data = USART1_read();				// RX from other MCU UART1	
        
    

       // Handle newline characters on both terminals
       if(BT_Data == '\r' || BT_Data == '\n'){
        uint8_t nextline[] = "\r\n";

        // Send newline to Bluetooth terminal
        USART1_write(nextline,2);
        // Send newline to PC Tera Term
        USART2_write(nextline, 2);
       }
       else{
         // Echo received character to Bluetooth terminal
         USART1_write((uint8_t *)&BT_Data,1);
        

        // Display Bluetooth input on PC Tera Term
       USART2_write((uint8_t *)&BT_Data, 1);
       }

         // LED control: L = OFF, H = ON
        if(BT_Data == '0'){
            GPIO_write(LED_PIN,LOW);
            //PWM_duty(LED_panel,1.0);
           
        }else if(BT_Data == '1'){
            GPIO_write(LED_PIN,HIGH);
            //PWM_duty(LED_panel,0);

  
        }
    }
}
//  if(BT_Data>='0' && BT_Data<='9'){
//             //이거는 test하기 힘들어
//             float value = (float)(BT_Data-'0'); //
//             float BT_duty=value*0.1;
//             PWM_duty(LED_panel,BT_duty);