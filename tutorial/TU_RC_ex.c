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

volatile uint32_t change = 0;
volatile uint32_t angle = 0;
volatile uint32_t PW = 0;
volatile uint32_t reach = 0;
// Initialization
void setup(void){
    RCC_PLL_init();
    SysTick_init();
    
    //Button Initialization
    GPIO_init(BUTTON_PIN,INPUT);
    GPIO_pupd(BUTTON_PIN, 1); //pull-up

    //intialization for exti
    EXTI_init(BUTTON_PIN, FALL, 10); 

    


    //TIM2 Update-Event Interrupt every 1 msec
    TIM_UI_init(TIM3, 500);           // UEV every 1 msec
    //TIM_UI_enable(TIM3);
    //TIM_UI_init_us(TIM2, 1000);   // UEV every 1000usec=1msec

    //pwm initialization with 20msec
     PWM_init(PA_1,20);
}


int main(void) {
    // Initialization --------------------------------------------------
    setup();

    // Infinite Loop ---------------------------------------------------
    while(1){
        //45도
        
        //PULSE WIDTH
        //pw_us = 500+(2000/180)*angle;; --> 오차가 큼. 
        //PWM_pulsewidth()
        //pulse width (US 기준)   - 정수로 정의되어있기 때문에 us가 편함 + 정수나눗셈 오차 줄이기 위해
        PW = 500+(2000 * angle)/180;  //정수 나눗셈 반올림
        PWM_pulsewidth_us(PA_1, PW);
        
        if(change == 1){
            reach = 0;
            angle = 0;
            change = 0;
        }

    }
}


void TIM3_IRQHandler(void) {

    if (is_UIF(TIM3)) {
        clear_UIF(TIM3);

        if(reach == 0){
            angle += 10;
        
            if(angle >= 180){
            reach = 1;
            }
         }
        else{
            if(angle<=10){ //0으로 밑으로 내려가는걸 방지
                angle = 0;
                reach = 0;
            }else{
            angle -=10;
            }
        }
    }

}

void EXTI15_10_IRQHandler(void) {  //careful for pin number
  
    if(is_pending_EXTI(BUTTON_PIN)){                //check if the EXTI pending flag is set
    
    change = 1;  // reset flag
    clear_pending_EXTI(BUTTON_PIN);  //clear the EXTI pending flag
    }               

}

