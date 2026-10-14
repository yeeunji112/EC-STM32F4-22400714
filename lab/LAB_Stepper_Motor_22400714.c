
/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : YeeunJi
Created          : 05-03-2021
Modified         : 10-13-2026
Language/ver     : C++ in VS Code

Description      : LAB_Stepper_Motor.c
/----------------------------------------------------------------*/

#include "ecSTM32F4v2.h"
// #include "stm32f411xe.h"
// #include "ecGPIO2.h"
// #include "ecRCC2.h"
// #include "ecEXTI2.h"
// #include "ecSysTick2.h"
// #include "ecStepper.h"


#define BUTTON_PIN  PC_13		// B1 Button
// ( A, B,  AN,  BN)
PinName_t pinStepper[4]    = {PB_10,PB_4,PB_5,PB_3};

//Motor control flag : 0 = Run, 1 = STOP
volatile uint8_t stop = 0 ;
// Select stepping mode: FULL or HALF
uint32_t mode = FULL;

// Initialiization 
void setup(void){	
	RCC_PLL_init();                                 // System Clock = 84MHz
	SysTick_init();                                 // Systick init
	
 	GPIO_init(BUTTON_PIN, INPUT);           		// GPIOC pin13 initialization
    EXTI_init(BUTTON_PIN, FALL,0);           		// External Interrupt Setting

	
	stepper_init(mode, pinStepper); 				// Stepper GPIO pin initialization
	stepper_speed(14);                 		//  set stepper motor speed(based on selected stepping mode
}

int main(void) { 
	// Initialiization --------------------------------------------------------
	setup();

	//set the desired number of revolution
	uint32_t rev=1;           // one revolution = one round
	uint32_t steps;
	
	//calculate the required steps based on the stepping mode
	if (mode == FULL){
		//Full step = 2048 steps per revolution
		steps = STEP_PER_REV*rev;
	}
	else{
		//half-step : 4096 steps per revolution
		steps=STEP_PER_REV*2*rev;
	}
	

	//if you want to see only one revolution use below code and in while, it should be empty
	//just for demo video.
	stepper_step(steps, DIR_CW); 
	stepper_stop();

	// Inifinite Loop ----------------------------------------------------------
	//continuous motor rotation with button control
	while(1){
	
		// if(stop== 0){
		// 	//Rotate the motor ocnterclockwise
		// 	stepper_step(steps, DIR_CW); 
		// }else{
		// 	stepper_stop(); // Keep all motor coils OFF while stopped
		// }
   
	
	}
}



//IF pushing the button permanently set the motor to stop
void EXTI15_10_IRQHandler(void) {
    if (is_pending_EXTI(BUTTON_PIN)) {

        stop = 1;          // Permanently set the motor to Stop
        stepper_stop();    // Stop the motor and turn off all coils

        clear_pending_EXTI(BUTTON_PIN);
    }
}