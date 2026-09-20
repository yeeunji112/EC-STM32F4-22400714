/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : YeeunJi
Created          : 05-03-2021
Modified         : 13-09-2026 
Language/ver     : C++ in VS Code

Description      : LAB_GPIO_DIO_LED_Photosensor
/----------------------------------------------------------------*/

#include "ecRCC2.h"
#include "ecGPIO2.h"

#define LED_PIN         PB_12       //LD0
#define BUTTON_PIN      PA_0        // PHOTODETECTOR

void setup(void);
    
int main(void) { 
    // Initialiization --------------------------------------------------------
    setup();

    //Inifinite Loop ----------------------------------------------------------
    while(1){       
        if(GPIO_read(BUTTON_PIN) == LOW)  GPIO_write(LED_PIN, HIGH);
        else                            GPIO_write(LED_PIN, LOW);
        delay_ms_HSI(100);                              // delay 100 ms
    }

}

// Initialiization 
void setup(void)
{
    RCC_HSI_init(); 
    GPIO_init(BUTTON_PIN, INPUT);  // calls RCC_GPIOA_enable()
    GPIO_pupd(BUTTON_PIN,1); //pull-up

    GPIO_init(LED_PIN, OUTPUT);    // calls RCC_GPIOB_enable()
    GPIO_pupd(LED_PIN,1); //pull-up
    GPIO_otype(LED_PIN, 1); //open-drain
    GPIO_ospeed(LED_PIN,1); //medium - speed 
}
