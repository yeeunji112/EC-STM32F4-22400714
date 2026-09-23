/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : YeeunJi
Created          : 05-03-2021
Modified         : 17-09-2026 
Language/ver     : C++ in VS Code

Description      : LAB_GPIO_7segment_22400714 (Counter with button)
/----------------------------------------------------------------*/

#include "stm32f4xx.h"
#include "ecGPIO2.h"
#include "ecRCC2.h"

#define BUTTON_PIN      PA_4        // button sw2 on jkit
PinName_t pinFND[8]    = {PB_0, PB_1, PB_2, PB_3, PB_4, PB_5, PB_6, PB_7};
PinName_t selectFND[4]= {PA_10, PA_11, PC_4, PC_3};
uint8_t numDisplay[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

void setup(void){
    // Intialize System Clock
    RCC_HSI_init();    
    // Intialize FND pins and Others
    
    //bUTTON PIN
    GPIO_init(BUTTON_PIN, INPUT);       // Calls RCC_GPIOA_enable()
    GPIO_pupd(BUTTON_PIN, 1);           // Pull-up
    
    //7 segment 
    FND_display_init(pinFND);
    //push pull
    for(int i = 0 ; i<8;i++){
        //push pull
        GPIO_otype(pinFND[i],0);
       //no pull up pull down
        GPIO_pupd(pinFND[i], 0);
        // medium speed 
        GPIO_ospeed(pinFND[i],1);
    }
    
   
    //selection of 7-segment Display 
    FND_select_init(selectFND);    
    // Intialize  Others
     for(int i = 0 ; i<4;i++){
        //push pull
        GPIO_otype(selectFND[i],0);
     }
    
};

int main(void) {
    setup();

    uint8_t selectFND0  = 0;
    int increase = 0 ;
    int pressed = 0 ;
    while (1) {
        
        //show '0' first 
        FND_select(selectFND0);
        FND_display(numDisplay[increase]);

        if(GPIO_read(BUTTON_PIN) == LOW){ 
           if(pressed == 0) {
            //always first digit 

            //0->1->2->3->4->5->6->7->8->9->0
            increase = (increase + 1) % 10;     

            //to make sure no change while button is pressed
            pressed = 1; 
           }
        } else {
            //wait for push button
            pressed = 0 ;
           
            }
            //to avoid debouncing 
         delay_ms_HSI(50);	


    }
}

    
