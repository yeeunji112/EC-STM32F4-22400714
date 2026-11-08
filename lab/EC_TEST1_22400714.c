/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : YeeunJi
Student ID       : 22400714
Created          : 10-22-2026 [WRITE THE DATE!!!!]
Language/ver     : C in VS Code

Description      : EC Test 1 (2026) Programming Test
                   
                   Rename this file to  EC_Test1_yourID.c
/----------------------------------------------------------------*/


#include "stm32f411xe.h"

//==================================================================
//  EC Library Include
//==================================================================
#include "ecSTM32F4v2.h"


//==================================================================
// User GPIO Pin definitions
//==================================================================
// YOUR CODE GOES HERE
// YOUR CODE GOES HERE
#define SW1_PIN   PD_2
#define SW2_PIN   PA_4
#define photo     PA_0
#define LED_0     PB_12

PinName_t pinStepper[4]    = {PC_5,PC_6,PC_7,PC_8}; //FULL 
// Stepper ( A, B, AN, BN )
// YOUR CODE GOES HERE

// FND segments
// YOUR CODE GOES HERE
PinName_t pinFND[8]    = {PB_0, PB_1, PB_2, PB_3, PB_4, PB_5, PB_6, PB_7};


// FND digit select : FND_0 ~ FND_3
// YOUR CODE GOES HERE
PinName_t selectFND[4]= {PA_10, PA_11, PC_4, PC_3};

// Global Variables
// use   volatile uint32_t   instead of uint32_t if used in Interrupt ISR
volatile uint32_t motor_mode   = 0;	
volatile uint32_t motor_dir    = 0;	//1 ccw, 0 = cw 
volatile uint32_t motor_speed  = 0;	
// YOUR CODE GOES HERE
// YOUR CODE GOES HERE
// YOUR CODE GOES HERE
volatile uint32_t steps = 1; 
volatile uint32_t val = 0;	
volatile uint32_t sw1_press = 0;	
volatile uint32_t sw2_press = 0;	
volatile uint32_t blink = 0;
volatile uint32_t rev = 3;
volatile uint32_t as = 0;
volatile uint32_t increase = 0;
//==================================================================
//  Setup 
//==================================================================
void setup(void){
	RCC_PLL_init();					// System Clock = 84MHz
	SysTick_init();					// SysTick, 1 msec
	
	// USART2, 9600 baud
	// YOUR CODE GOES HERE			
	// YOUR CODE GOES HERE			
	USART2_init();  // default 9600
	// GPIO Inputs
	// YOUR CODE GOES HERE
	// YOUR CODE GOES HERE
	GPIO_init(SW1_PIN, INPUT);  // calls RCC_GPIOA_enable()
    GPIO_pupd(SW1_PIN,1); //pull-up
	GPIO_init(SW2_PIN, INPUT);  // calls RCC_GPIOA_enable()
    GPIO_pupd(SW2_PIN,1); //pull-up

    GPIO_init(photo, INPUT);    // calls RCC_GPIOB_enable()
    GPIO_pupd(photo,1); //pull-up

	GPIO_init(LED_0, OUTPUT); 

	stepper_init(FULL, pinStepper); 				// Stepper GPIO pin initialization
	//stepper_speed(10);  
	
	FND_display_init(pinFND);
    //push pull
    for(int i = 0 ; i<8;i++){
        //push pull
        GPIO_otype(pinFND[i],0);
    }
	 //selection of 7-segment Display 
    FND_select_init(selectFND);    
    // Intialize  Others
     for(int i = 0 ; i<4;i++){
        //push pull
        GPIO_otype(selectFND[i],0);
     }

	// EXTI 
	// YOUR CODE GOES HERE
	// YOUR CODE GOES HERE
	  //intialization for exti PA_4
    EXTI_init(SW2_PIN, FALL, 5); 

    //intialization for exti PD_2
    EXTI_init(SW1_PIN, FALL, 5);
	// GPIO Outputs	
	// YOUR CODE GOES HERE
	// YOUR CODE GOES HERE
	// YOUR CODE GOES HERE

	// Other Setup
	// YOUR CODE GOES HERE
}


//==================================================================
//  Main
//==================================================================
int main(void){
	setup();

	// YOUR CODE GOES HERE
	printf("MCU Initialized \r\n");	

	while(1){

		// Check PAUSE/RESUME 
		// YOUR CODE GOES HERE
		// YOUR CODE GOES HERE
		//디폴트
		 uint8_t numDisplay1  = 0;
		//  if(motor_speed == 0){
		// 	stepper_step(1, motor_dir);
		// 	FND_select(0); 
		//  	FND_display(numDisplay1);
		// 	GPIO_write(LED_0,HIGH);
		//  }
		
		// stepper_step(1, motor_dir);
		// FND_select(0); 
		// FND_display(numDisplay1);
		// GPIO_write(LED_0,HIGH);
		
		 
		// Related to controlling motor speed/dir etc
		// HINT:  use  stepper_speed(vel);	only when vel>0
		
		
		
		if(motor_mode == 0){
			
			stepper_stop();
			FND_select(0); 
			FND_display(numDisplay1);
			// stepper_stop();
			//stepper_step(1, motor_dir);
		
		  	GPIO_write(LED_0,HIGH);
			
		} else if(motor_mode == 1){
			FND_select(0);
         	FND_display(1);
        	//delay_ms_HSI(1);
			stepper_speed(2);
			GPIO_write(LED_0,HIGH);
			//steps = STEP_PER_REV*rev;
			stepper_step(steps, motor_dir);
			
		}
		
		else if(motor_mode == 2){
			// FND_select(0);
         	// FND_display(2);
        	// delay_ms_HSI(1);
			GPIO_write(LED_0,HIGH);
			stepper_speed(6);
			//steps = STEP_PER_REV*rev;
			stepper_step(steps, motor_dir);
			//GPIO_write(LED_0,HIGH);
		
		}

		else if(motor_mode == 3){
			// FND_select(0);
         	// FND_display(3);
        	// delay_ms_HSI(1);
			stepper_speed(10);
			GPIO_write(LED_0,HIGH);

			//steps = STEP_PER_REV*rev;
			stepper_step(steps, motor_dir);
			
		}

		// // steps = STEP_PER_REV*rev;
		// // stepper_step(steps, motor_dir);




		if(as == 0){
			// FND_select(0);
         	// FND_display(0);
        	// delay_ms_HSI(1); 
			stepper_step(steps, 0);
		}

		//dir ccw 이면 fnd1에 1
		if(as == 1){
			// FND_select(1);
         	// FND_display(1);
        	// delay_ms_HSI(1); 
			stepper_step(steps, 1);
		}
		

	
	//printf("mode = %lu\r\n",motor_mode);		
	delay_ms(1);
	}
}


//==================================================================
//  EXTI Interrupt Service Routines
//==================================================================

	// YOUR CODE GOES HERE
	// YOUR CODE GOES HERE
	// YOUR CODE GOES HERE
	// YOUR CODE GOES HERE
	//PA_4 : Pause / resume counting when SW2 is pressed 

void EXTI4_IRQHandler(void) {  //careful for pin number
  
    if(is_pending_EXTI(SW2_PIN)){                //check if the EXTI pending flag is set
	
	//motor_speed -= 1;		
	motor_mode--; 
	if(motor_mode<0){
		motor_mode = 0;

	}
	FND_display(motor_mode);
	
	    // if(motor_mode == 0){
		// 	stepper_stop();
		// }
	    // if(motor_mode == 1){
		// 	stepper_speed(2);
		// }else if(motor_mode == 2){
		// 	stepper_speed(6);
		// }else if(motor_mode == 3){
		// 	stepper_speed(10);
		// }
    clear_pending_EXTI(SW2_PIN);  //clear the EXTI pending flag
    }               

}


//PD_2: INCREASE THE MOTOR SPEED, SW1 
void EXTI2_IRQHandler(void){
    if(is_pending_EXTI(SW1_PIN)){
		//motor_speed++; 
		//sw1_press = 1;
		// increase ++; 
		motor_mode++; 
		if(motor_mode>3){
		motor_mode = 3;
	}
		// if(increase == 1) {
		// 	motor_mode=2; 
		// }
		//increase++;
        FND_display(motor_mode); // the display is handled in the main loop instead of inside the ISR
        // if(motor_mode == 1){
		// 	stepper_speed(2);
		// }else if(motor_mode == 2){
		// 	stepper_speed(6);
		// }else if(motor_mode == 3){
		// 	stepper_speed(10);
		// }
		clear_pending_EXTI(SW1_PIN);  //clear the EXTI pending flag
    }
}



//==================================================================
//  UART2 Interrupt Service Routines
//==================================================================
// void USART2_IRQHandler(void){
// 	if(is_USART2_RXNE()){
	
// 	// HINT: 	comparing a character:  
// 	//  		uint8_t ch = USART2_read();		
// 	//			if(ch == 'C' || ch == 'c')	 etc..
// 	//
// 	// YOUR CODE GOES HERE
// 	// YOUR CODE GOES HERE
// 	}
//}

void USART2_IRQHandler(){       // USART2 RX Interrupt 
	if(is_USART2_RXNE()){
		//PC_data = USART2_read();    
        //read a character from the local pc 
		uint8_t ch = USART2_read();    
        

       //send each character immdeiately to the other MCU
       //이건 상대방 teraterm에 뜨게 하는게 아니라 mcu에 보내는거

      	//USART1_write((uint8_t *)&ch, 1);
 		USART2_write((uint8_t *)&ch, 1);

        //echo the character --> this should allow to show what is been written
        if(ch == 'C' || ch == 'c'){
			motor_dir = 0; 
			FND_select(0);
         	FND_display(0);
        	delay_ms_HSI(1); 
				//stepper_speed(5);
				as = 0;
				//steps = STEP_PER_REV*rev;
				//stepper_step(steps, DIR_CW);
			}

		else if(ch == 'B' || ch == 'b'){
			motor_dir = 1; //ccw
			FND_select(1);
         	FND_display(1);
        	delay_ms_HSI(1); 
			//stepper_speed(5);
			as = 1;
			//steps = STEP_PER_REV*rev;
			//stepper_step(steps, 0);
		}
	}
	

}



		//dir ccw 이면 fnd1에 1
	

// void USART1_IRQHandler(){          	// USART2 RX Interrupt
// 	if(is_USART1_RXNE()){
// 		BT_Data = USART1_read();				// RX from other MCU UART1		
// 		printf("RX: %c \r\n",BT_Data); 	// TX to USART2(PC)
// 	}
// }

// void USART2_IRQHandler(){       // USART2 RX Interrupt 
// 	if(is_USART2_RXNE()){
// 		PC_Data = USART2_read();		// RX from UART2 (PC)
// 		USART2_write(&PC_Data,1);		// TX to USART2	 (PC)	 Echo of keyboard typing
// 		USART1_write(&PC_Data,1);	  // TX to Other MCU
// 	}
// }

//교수님코드 보기

//PAUSE / RESUME 
//check pause/ resume
//exti안에서는 원래 간단하게 써야돼. 
// user function을 만들더라도 안에 들어간 알고리즘에 dealy 가들어가면 안되고 printf 안되고, stepper motor도 exti안엣 ㅓ돌리면 안돼
//dc motor는 된지만 stepper motor는 안돼
//stepper_spped는 0을 줄수없어서 if,else로 rpm이 0보다 클때, stepper_step으로 하고, else인경우 stop 
//if(msTicks - t_2sec>=2000){
// t_2sec = msTicks;
// ifledblink led toggle(led_0)}