/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : YeeunJi
Created          : 05-03-2021
Modified         : 10-11-2026
Language/ver     : C++ in VS Code

Description      : Stepper Library. 
/----------------------------------------------------------------*/

#include "stm32f4xx.h"
#include "ecStepper.h"


// Stepper Motor variable
static PinName_t stepperPins[4];
static volatile uint32_t _steps;
static volatile uint32_t _step_delay = 100;		
static const StateStepper_t *fsm;				// points at fsm_full or fsm_half (chosen in stepper_init)
static volatile uint32_t state = 0;				// current FSM state



//The fsm_Full[4] array defines four states for full-stepping
//based on the FSM table. Each state specifies the next state
//for CW or CCW rotation and the corresponding GPIO output signals.
static const StateStepper_t fsm_Full[4] = {	
 {{S1,S3},{HIGH,HIGH,LOW,LOW}},	//
 {{S2,S0},{LOW,HIGH,HIGH,LOW}},	
 {{S3,S1},{LOW,LOW,HIGH,HIGH}},	
 {{S0,S2},{HIGH,LOW,LOW,HIGH}},	
 
	
};




static const StateStepper_t fsm_Half[8] = {  
	{{S1,S7},{1,0,0,0}},
	{{S2,S0},{1,1,0,0}},
	{{S3,S1},{0,1,0,0}},
	{{S4,S2},{0,1,1,0}},
	{{S5,S3},{0,0,1,0}},
	{{S6,S4},{0,0,1,1}},
	{{S7,S5},{0,0,0,1}},
	{{S0,S6},{1,0,0,1}},
	
};



// Initialization of Stepper Motor
void stepper_init(uint32_t mode, PinName_t *pinStepper){
	if (mode == HALF) 
		fsm = fsm_Half;
	else
		fsm = fsm_Full;

	//  GPIO Digital Out Initiation
	// For A, B, AN, BN
    //Initialize the four motor control pins as digital outputs
	for (int i=0; i<4; i++){
		stepperPins[i] = pinStepper[i];
		GPIO_init(pinStepper[i],OUTPUT); // OUTPUT
		GPIO_ospeed(pinStepper[i],2); //fast - speed 

	}
}

// Converts Motor [rpm] to step delay in [msec]
//ONLY FOR FULL STEP 
// void stepper_speed (uint32_t speedRPM){      // rpm
// 		_step_delay = 60*1000/(STEP_PER_REV*speedRPM); 		//in [msec]	
// 		// [usec] can be used for a higher accuracy
// 		// But needs delay_us() 
// 		//

		
// }

//delay_ms 사용한다면 이걸로
void stepper_speed(uint32_t speedRPM) {
    uint32_t stepsneed;

    // Calculate the required steps per revolution based on the stepping mode
    if (fsm == fsm_Half)
        stepsneed = STEP_PER_REV * 2;  // HALF: 4096 steps/rev
    else
        stepsneed = STEP_PER_REV;      // FULL: 2048 steps/rev

    // Calculate the delay between steps to achieve the desired RPM
    _step_delay = 60000 / (stepsneed * speedRPM);
}

//delay_us사용한다면이걸로
// void stepper_speed(uint32_t speedRPM)
// {
//     if (speedRPM == 0)
//         return;

//     uint32_t stepsPerRev;

//     if (fsm == fsm_Half)
//         stepsPerRev = STEP_PER_REV * 2;
//     else
//         stepsPerRev = STEP_PER_REV;

//     _step_delay = 60000000UL / (stepsPerRev * speedRPM);
// }



// Run Stepper Motor for given steps and direction
//full = 2048step, half = 4096 step
void stepper_step(uint32_t steps, uint32_t direction){
	// Exit if stepper_init() not called 
	if (fsm == 0) return;
	_steps = steps;

	// run for step size: 
	for (uint32_t i = 0; i < _steps; i++){
		// Update Present State
		state = fsm[state].next[direction];
		
		// Output of Present State
		stepper_out(state);
	
			//  wait between steps to control the motor speed
		delay_ms(_step_delay); //can't follow state if it doesn't exist
		//delay_us(_step_delay); //이거 틀리면 이부분 삭제하고 위에꺼 복구
	}
}

// Stepper Motor Output for given state
void stepper_out(uint32_t state){
	
	// stepperPins[0]=A, stepperPins[1]=B,
    // stepperPins[2] = AN, stepperPins[3] = BN
	// out[0]=A, out[1]=B, out[2]=AN, out[3]=BN

	for(int i = 0 ; i < 4 ;i ++){
	GPIO_write(stepperPins[i],fsm[state].out[i]);
	}
		

}


// Stop Stepper Motor
void stepper_stop(void) {
	//set the step count to zero to terminate the current stepping loop.
    _steps = 0;    
    
	// All pins DigitalOut '0'
	for(int i = 0 ; i <4 ; i ++){
		GPIO_write(stepperPins[i],LOW);
	}

}



