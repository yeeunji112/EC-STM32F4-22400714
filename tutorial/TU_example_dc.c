/*---------------------------------------------------------------
  IR Proximity Sensor Test
  Board  : NUCLEO-F411RE
  Sensor : IR Proximity Sensor (D0 Output)
  Input  : PA4
  LED    : PA5 (LD2)
  Serial : USART2
---------------------------------------------------------------*/

#include "ecSTM32F4v2.h"

#define IR_PIN   PA_4
#define LED_PIN  PA_5

uint32_t last_print = 0;
uint32_t IR_value = 0;
uint32_t pause = 0;


// Initialization
void setup(void) {

    RCC_PLL_init();
    SysTick_init();

    // IR Sensor Initialization
    GPIO_init(IR_PIN, INPUT);
    GPIO_pupd(IR_PIN, 1);       // Pull-up

    // LED Initialization
    GPIO_init(LED_PIN, OUTPUT);
    GPIO_write(LED_PIN, LOW);

    // USART2 Initialization
    USART2_init();
}


// Main
int main(void) {

    setup();

    while(1) {

        // Read IR Sensor
        IR_value = GPIO_read(IR_PIN);

        // Assuming Active-Low Sensor
        if(IR_value == 0) {

            pause = 1;
            GPIO_write(LED_PIN, HIGH);

        }
        else {

            pause = 0;
            GPIO_write(LED_PIN, LOW);

        }

        // Print Every 500ms
        if(timer_check(&last_print, 500)) {

            printf("IR Sensor = %lu\r\n",
                   (unsigned long)IR_value);

            printf("PAUSE = %lu\r\n",
                   (unsigned long)pause);

            printf("--------------------\r\n");
        }
    }
}


// USART2 RX Interrupt
void USART2_IRQHandler(void) {

    if(is_USART2_RXNE()) {
        USART2_read();
    }
}