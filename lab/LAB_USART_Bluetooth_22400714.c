/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : Yeeun Ji
Created          : 10-08-2026
Modified         : 10-08-2026
Language/ver     : C in VS Code

Description      : bluetooth cononection
/----------------------------------------------------------------*/



//J, cw direction
//k, ccw direction
//p, emergency stop 
#include "ecSTM32F4v2.h"


//stepper motor pin name definition
PinName_t pinStepper[4]    = {PB_10,PB_4,PB_5,PB_3};
//URAT receive data
static volatile uint8_t PC_Data = 0;
static volatile uint8_t BT_Data = 0;

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
    int32_t rev=2;           // REQUIRED - two revolution

    //calculate the required steps based on the stepping mode
	if (mode == FULL){
		//Full step = 2048 steps per revolution
		steps = STEP_PER_REV*rev;
	}
	else{
		//half-step : 4096 steps per revolution
		steps=STEP_PER_REV*2*rev;
	}

    
	printf("MCU Initialized\r\n");	 //display on stm terminal

    uint8_t BTin[]="MCU Initalization\r\n";
    USART1_write(BTin, sizeof(BTin)-1);// display on bluetooth terminal
	while(1){
		
       // Execute clockwise rotation when J is received
        if(right == 1){

            // Clear the command flag before starting rotation
            right = 0;

            // Record the current rotation direction
            dir = 1;
             // Rotate two revolutions in the CW direction
            stepper_step(steps, DIR_CW);  
          
            // Reset direction after rotation if unchanged 
            if(dir == 1){
                dir = 0;
            }
        }
        // Execute counterclockwise rotation when K is received
        if(left == 1){
            // Clear the command flag before starting rotation
            left = 0;
            // Record the current rotation direction
            dir = 2;
             // Rotate two revolutions in the CCW direction
            stepper_step(steps, DIR_CCW);  
             // Reset direction after rotation if unchanged
            if(dir == 2){
                dir = 0;
            }

        }
         
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
        if(BT_Data == 'L'){
            GPIO_write(LED_PIN,LOW);
        }else if(BT_Data == 'H'){
            GPIO_write(LED_PIN,HIGH);
        }

        // Stop the current rotation if the opposite direction is requested
        if((BT_Data == 'J' && dir == 2) || (BT_Data == 'K' && dir == 1)) {
            
           stepper_stop();
        }
        // J: Request clockwise rotation
          if(BT_Data == 'J'){
           
            right = 1;
            left = 0;
        }
        // K: Request counterclockwise rotation
         else if(BT_Data == 'K'){
           
            left = 1;
            right = 0;
        }else if(BT_Data == 'P'){
           // P: Emergency stop
            stepper_stop();
            // Stop the motor and clear pending rotation commands
            right = 0;
            left = 0;
            dir = 0;
        } 
        
	}
}



