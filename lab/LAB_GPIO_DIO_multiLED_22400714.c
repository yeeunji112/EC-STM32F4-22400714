/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : YeeunJi
Created          : 05-03-2021
Modified         : 13-09-2026
Language/ver     : C++ in VS Code

Description      : LAB_GPIO_DIO_multiLED
/----------------------------------------------------------------*/

#include "ecRCC2.h"
#include "ecGPIO2.h"


#define NUM                 4
#define BUTTON_PIN          PA_4        // B4 Button

PinName_t LED_PIN[] = {PB_12, PB_13, PB_14, PB_15}; // LD0, LD1, LD2, LD3


void setup(void);


int main(void) {

    // Initialization --------------------------------------------------------
    setup();

    int increase = 0;
    int change = 0;

    // Infinite Loop ---------------------------------------------------------
    while(1) {

        if(GPIO_read(BUTTON_PIN) == LOW) {       // button pressed (active-low)

            if(change == 0) {
                //turn off all LEDs
                for(int i = 0; i < NUM; i++) {
                    GPIO_write(LED_PIN[i], LOW);
                }
                // turn on current LED
                GPIO_write(LED_PIN[increase], HIGH);
                // to make a sequence LED0->LED1->LED2->LED3->LED0
                increase = (increase + 1) % NUM;     
                // button press has already been processed
                change = 1;
            }

        } else {

            // Ready for the next button press
            change = 0;
        }
        delay_ms_HSI(100);    // delay 100 ms
    }

}


// Initialization ------------------------------------------------------------
void setup(void)
{
    RCC_HSI_init();

    
    GPIO_init(BUTTON_PIN, INPUT);       // Calls RCC_GPIOA_enable()
    GPIO_pupd(BUTTON_PIN, 1);           // Pull-up

   
    for(int i = 0; i < NUM; i++) {

        GPIO_init(LED_PIN[i], OUTPUT);   // Calls RCC_GPIOB_enable()
        GPIO_pupd(LED_PIN[i], 1);        // Pull-up
        GPIO_otype(LED_PIN[i], 1);       // Open-drain
        GPIO_ospeed(LED_PIN[i], 1);      // Medium speed
    }
}