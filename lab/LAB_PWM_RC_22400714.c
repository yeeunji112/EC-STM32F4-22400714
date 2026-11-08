/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : YeeunJi
Created          : 05-03-2021
Modified         : 10-05-2026
Language/ver     : C++ in VS Code

Description      : LAB -  PWM RC 
/----------------------------------------------------------------*/

#include "ecSTM32F4v2.h"

              
#define BUTTON_PIN          PC_13        // FOR RESET

volatile uint32_t change = 0; //Flag for reset
volatile uint32_t angle = 0;  //current servo angle in degrees
volatile uint32_t PW = 0;     //PWM width (in microseconds)
volatile uint32_t reach = 0;  //Direction -> 0: increase , 1: decrease
// Initialization
void setup(void){
    RCC_PLL_init();
    SysTick_init();
    
    //Button Initialization
    GPIO_init(BUTTON_PIN,INPUT);
    GPIO_pupd(BUTTON_PIN, 1); //pull-up

    //intialization for exti for the user button
    EXTI_init(BUTTON_PIN, FALL, 10); 

    


    //Configure TIM3 to generate an update interrupt every 500ms
    TIM_UI_init(TIM3, 500);          
    //TIM_UI_enable(TIM3);

    //Initialize PWM on PA1 with a 20 ms period (50Hz)
     PWM_init(PA_1,20);
}


int main(void) {
    // Initialization --------------------------------------------------
    setup();

    // Infinite Loop ---------------------------------------------------
    while(1){
        
        
        //PULSE WIDTH
        //pw_us = 500+(2000/180)*angle;; --> 오차가 큼. 
        //PWM_pulsewidth()
        //pulse width (US 기준)   - 정수로 정의되어있기 때문에 us가 편함 + 정수나눗셈 오차 줄이기 위해
        
        //Reset the servo angle to 0 when the button is pressed 
        if(change == 1){
            reach = 0;
            angle = 0;
            change = 0;
        }

        //conver the servo angle into PWM pulse width(500-2500 us)
        //multiply before diving to reduce integer truncation error
        //integer division truncates the fractional part
        PW = 500+(2000 * angle)/180;  

        //update the PWM pulse width
        PWM_pulsewidth_us(PA_1, PW);
        

    }
}

//TIM3 update interrupt handler -> changes the angle every 500ms
void TIM3_IRQHandler(void) {

    if (is_UIF(TIM3)) {
        clear_UIF(TIM3);

        //increase the angle by 10 degreses until 180 degrees
        if(reach == 0){
            angle += 10;
        
            if(angle >= 180){
            reach = 1;
            }
         }
        else{
            //prevent unsigned integer underflow
            if(angle <=10){
                angle = 0;
                reach = 0;
            }else{
                //decrease angle by 10 degrees
                angle -=10;
            }
        
        }
    }
}

//External interrupt handler for resetting the servo motor
void EXTI15_10_IRQHandler(void) {  //careful for pin number
  
    if(is_pending_EXTI(BUTTON_PIN)){                //check if the EXTI pending flag is set
    
    change = 1;  // set reset flag
    //This allows approximately 500 ms before the next angle update
    TIM3->CNT = 0; // reset TIM 3 counting from zero
    //clear the TIM3 update interrupt flag
    clear_UIF(TIM3);

    //clear EXTI pending flag
    clear_pending_EXTI(BUTTON_PIN); 
    }               

}

