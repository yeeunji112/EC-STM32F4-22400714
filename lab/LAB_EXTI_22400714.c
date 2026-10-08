/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : YeeunJi
Created          : 05-03-2021
Modified         : 10-05-2026
Language/ver     : C++ in VS Code

Description      : LAB -  EXTI 7 segment (show 0~9 and repeat)
/----------------------------------------------------------------*/


// #include "ecRCC2.h"
// #include "ecGPIO2.h"
// #include "ecSysTick2.h"
// #include "ecEXTI2.h"

#include "ecSTM32F4v2.h"

#define SW2_PIN    PA_4			//EVAL board JKIT SW2

#define  SW1_PIN        PD_2      // sw1, reset 



PinName_t pinFND[8]    = {PB_0, PB_1, PB_2, PB_3, PB_4, PB_5, PB_6, PB_7}; //a,b,c,d,e,f,g,dot
PinName_t selectFND[4]= {PA_10, PA_11, PC_4, PC_3};
uint8_t numDisplay[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

//global variables shared between main and ISR
//"volatile" prevents the compiler from optimizing accesses to variables that may change in an ISR
volatile int increase = 0 ; 
volatile int change = 0 ;
volatile int pause = 0 ;


// Initialiization 
void setup(void)
{
    // RCC_PLL_init();                 // System Clock = 84MHz
    // SysTick_init();                 // SysTick Timer Initialization

    MCU_init();                        


    //Initialization SW1 
    GPIO_init(SW1_PIN,INPUT);
    GPIO_pupd(SW1_PIN, 1); //pull-up

    GPIO_init(SW2_PIN,INPUT);
    GPIO_pupd(SW2_PIN, 1); // pull - up

    // Initialize 7 semgnet
    FND_display_init(pinFND);

    for(int i = 0 ; i<8; i++)
{
    GPIO_otype(pinFND[i],0); // push pull 
    GPIO_pupd(pinFND[i],0); //No pull up pull down
    GPIO_ospeed(pinFND[i],1); // medium speed
}    

    FND_select_init(selectFND);  
	GPIO_otype(selectFND[0],0); //push-pull

    //intialization for exti PA_4
    EXTI_init(SW2_PIN, FALL, 10); 

    //intialization for exti PD_2
    EXTI_init(SW1_PIN, FALL, 10);
}


// MAIN  ----------------------------------------
int main(void) {

    setup();

    uint8_t selectFND0  = 0;

    //select the first digit to initially show 0 
    FND_select(selectFND0); //DIGIT
    //FND_display(increase);

	while (1){
        
      
        //store and display the counter value from 0 to 9
        FND_display(increase);

        
        delay_ms(1000); // count up at the rate of 1sec = 1000ms

        //if a reset interrupt occurs during the delay, skip the increment 
        //so that counting restarts from 0
        if(change == 1){
          
            change = 0;
            continue;  //skip the increment below and  restart counting from 0

        }


        //pause counting when sw2 is pressed
        if( pause == 1 ){  
            //FND_display(increase);   
            continue; //skip the increment below t keep the counter paused
        }
        //increase from 0 to 9 and repeat
        increase = (increase + 1) % 10;

    }

}


//PA_4 : Pause / resume counting when SW2 is pressed 
void EXTI4_IRQHandler(void) {  //careful for pin number
  
    if(is_pending_EXTI(SW2_PIN)){                //check if the EXTI pending flag is set
    //FND_display(increase);      //no necessary code because it will be handled in the main loop 
    pause ^= 1;  // toggle pause so that counting can be paused and resumed.
    clear_pending_EXTI(SW2_PIN);  //clear the EXTI pending flag
    }               

}

//PD_2: reset the counter when SW1 is pressed
void EXTI2_IRQHandler(void){
    if(is_pending_EXTI(SW1_PIN)){
        increase = 0 ;           //reset the counter to 0
        //FND_display(increase); // the display is handled in the main loop instead of inside the ISR
        change = 1;               // Indicate that a reset interrupt has occured
        clear_pending_EXTI(SW1_PIN);  //clear the EXTI pending flag
    }
}


