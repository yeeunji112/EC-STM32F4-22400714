/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : YeeunJi
Created          : 05-03-2021
Modified         : 11-05-2026
Language/ver     : C++ in VS Code

Description      : LAB -  PWM DC (FAN CONTROLLING)
/----------------------------------------------------------------*/

#include "ecSTM32F4v2.h"

    
#define BUTTON_PIN          PC_13        

#define Direction_Pin        PC_2
#define proximity            PA_4      //selected IR proximity sensor pin
#define LED_PIN              PA_5      // LED (LD2)
#define OBJECT_DETECTED()   (GPIO_read(proximity) != 0)   // CdS : dark -> HIGH

//FSM state defnition
#define S0    0
#define S1    1
#define S2    2

const char *modeName[3] = {"OFF", "MID", "HIGH"};

//led output
#define LED_OFF    0
#define LED_ON     1
#define LED_Blink  2
//fsm table(mealy)
typedef struct{
    //Inputs: Button press (X) / OBJECT DETECTED (Y) 
    uint32_t next[2][2];  //NEXT STATE
    float dutyratio[2][2];   //PWM DUTY RATIO
    uint32_t LED[2][2];      // LED OUTPUT
}state_table;

state_table table[3]={

    //X: [0] Button not pressed, [1] Button pressed
    //Y : [0] Object not detected, [1] Object detected
    //S0 - OFF
    {{{S0,S0},{S1,S0}},
    {{0.0, 0.0}, {0.5,0.0}},
    {{LED_OFF,LED_Blink}, {LED_ON,LED_Blink}}
    },
    //S1 - MID
    {
    {{S1,S1},{S2,S1}},
    {{0.5, 0.0}, {1.0,0.0}},
    {{LED_ON,LED_Blink}, {LED_ON,LED_Blink}}
    },
    //S2- HIGH
    {
    {{S2,S2},{S0,S2}},
    {{1.0, 0.0}, {0.0,0.0}},
    {{LED_ON,LED_Blink}, {LED_OFF,LED_Blink}}
    }

};


volatile uint32_t mode = S0;
volatile uint32_t pause = 0;
volatile uint32_t PW = 0;
volatile uint32_t X = 0;

// Initialization
void setup(void){
    RCC_PLL_init();
    SysTick_init();
    
    //Button Initialization
    GPIO_init(BUTTON_PIN,INPUT);
    GPIO_pupd(BUTTON_PIN, 1); //pull-up

    //Dierection pin initialization
    GPIO_init(Direction_Pin,OUTPUT);
    GPIO_otype(Direction_Pin, 0);           //push-pull
    GPIO_write(Direction_Pin, LOW);      //keep the direction fixed

    //intialization for PC13
    EXTI_init(BUTTON_PIN, FALL, 10); 

    //IR proximity initialization
    GPIO_init(proximity,INPUT);
    GPIO_pupd(proximity, 1); //pull-up

    //LED initialization
    GPIO_init(LED_PIN, OUTPUT);
    

    //TIM3 Update-Event Interrupt every 500ms
    TIM_UI_init(TIM3, 500);           
    //TIM_UI_enable(TIM3); //not used

    //pwm (TIM2_CH1) initialization with 1 msec period
    PWM_init(PA_0,1);           //TIM2_CH1 (1msec) --> default duty is 50%
    //set the initial PWM duty ratio to 0%
    PWM_duty(PA_0, 0.0f);     

    //USART2 initialization
    USART2_init(); 

}

int main(){

    uint32_t last_print = 0;

    setup();

    while(1){


       // X: Button input, Y: IR proximity sensor (PAUSE)
        // The proximity sensor is active-low
        if(GPIO_read(proximity) == 0){

            //object detected: Enter PAUSE mode
            pause = 1; 

            X = 0;        //to maintain the current mode
            PWM_duty(PA_0,table[mode].dutyratio[X][pause]); // staop the motor
        }else{
            //objected removed: resume the prvious operating mode
            pause = 0;
            
            //set the PWM duty ratio based on the FSM table
            PWM_duty(PA_0,table[mode].dutyratio[X][pause]);
           
            //Led control according to the FSM output
            if(table[mode].LED[X][pause]==LED_ON){
                GPIO_write(LED_PIN,HIGH);
            }else{
                
                GPIO_write(LED_PIN,LOW);
            }

            //mode update for FSM : S0->S1->S2->S0
            mode = table[mode].next[X][pause];
            //clear the button flag
            X = 0;
        }

    
        

        //convert duty ratio to a percentage
        uint32_t printduty = (uint32_t)(table[mode].dutyratio[0][pause] * 100);

        // Display the current mode and LED status every 2 seconds
        if(table[mode].LED[0][pause] == LED_OFF){
            if(timer_check(&last_print, 2000)){
                //use %s for string
                printf("The Mode : %s, Duty Ratio : %lu%%\r\n",modeName[mode],(unsigned long)printduty);
                printf("LED OFF\r\n");
            }
            //led on case -> mid and high
        }else if(table[mode].LED[0][pause] == LED_ON){
            if(timer_check(&last_print, 2000)){
                printf("The Mode : %s, Duty Ratio : %lu%%\r\n",modeName[mode],(unsigned long)printduty);
                printf("LED ON\r\n");
            }
        }
        else{
            //display pause -> blinking
            if(timer_check(&last_print, 2000)){
                printf("Current status is PAUSE\r\n");
                printf("The Mode : %s, Duty Ratio : %lu%%\r\n",modeName[mode],(unsigned long)printduty);

                printf("LED is blinking\r\n");
            }
        }


    }
}
     






//500msec
void TIM3_IRQHandler(void) {
    if (is_UIF(TIM3)) {
        clear_UIF(TIM3);
        //check FSM LED during PAUSE
// LED[0][pause] : button input does not affect LED blink (Blink for both X=0,1 when Y=T)
        if (table[mode].LED[0][pause] == LED_Blink) {
            //REQUIRED: 1 SECOND blinking period
            led_toggle(LED_PIN);
        }
    }
}

//EXTI interrupt for BUTTON CHANGES MODE & button input: X
void EXTI15_10_IRQHandler(void) {  //careful for pin number
  static uint32_t last_button = 0;
    if(is_pending_EXTI(BUTTON_PIN)){                //check if the EXTI pending flag is set
        clear_pending_EXTI(BUTTON_PIN);  //clear the EXTI pending flag
       //accept button input only when no object is detected
       // debounce interval = 200 ms
        if(timer_check(&last_button, 200) && GPIO_read(proximity) != 0){
            //button event flag is set
            X = 1;

        }               
    }
}




void USART2_IRQHandler(){       // USART2 RX Interrupt 
	if(is_USART2_RXNE()){
		 USART2_read();		// Rread and clear recevied data
		 
  }
}