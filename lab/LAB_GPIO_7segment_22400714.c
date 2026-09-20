/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : YeeunJi
Created          : 05-03-2021
Modified         : 17-09-2026 
Language/ver     : C++ in VS Code

Description      : LAB_GPIO_7segment_22400714
/----------------------------------------------------------------*/

#include "stm32f4xx.h"
#include "ecGPIO2.h"
#include "ecRCC2.h"

PinName_t pinFND[8]    = {PB_0, PB_1, PB_2, PB_3, PB_4, PB_5, PB_6, PB_7};
PinName_t selectFND[4]= {PA_10, PA_11, PC_4, PC_3};
// [YOUR CODE GOES HERE]

void setup(void){
    // Intialize System Clock
    RCC_HSI_init();    
    // Intialize FND pins OUT
    FND_init(pinFND);
      for(int i = 0 ; i<8;i++){
        //push-pull
        GPIO_otype(pinFND[i],0);
        GPIO_ospeed(pinFND[i],1);

    } 

    //SELECT intialize
    FND_select_init(selectFND);   
       for(int i = 0 ; i<4;i++){
       GPIO_otype(selectFND[i],0);
    }  
};

int main(void) {
    setup();
    uint8_t numDisplay1  = 0;
    uint8_t numDisplay2  = 5;
    uint8_t selectFND1  = 0;
    uint8_t selectFND2  = 1;

    while (1) {
        FND_select(selectFND1);
        FND_display(numDisplay1);
        delay_ms_HSI(1);             		
        FND_select(selectFND2);
        FND_display(numDisplay2);
        delay_ms_HSI(1);	
    }
  
}


