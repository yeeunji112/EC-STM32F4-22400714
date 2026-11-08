/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : YeeunJi
Created          : 05-03-2021
Modified         : 10-05-2026
Language/ver     : C++ in VS Code

Description      : LAB -  PWM DC (FAN CONTROLLING)
/----------------------------------------------------------------*/

#include "ecSTM32F4v2.h"

#define NUM                 
#define BUTTON_PIN          PC_13        

#define Direction_Pin        PC_2
#define proximity            PA_4      //내가 지정
#define LED_PIN              PA_5      // LD2 + 내가 지정



//state defnition
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
    //BUTTON PRESSED / OBJECT DETECTED 
    uint32_t next[2][2];  //NEXT STATE:
    float dutyratio[2][2];   //PWM DUTY
    uint32_t LED[2][2];      // LED OUTPUT
}state_table;

state_table table[3]={

    //[0] - BUTTON NOT PRESSED [1] - BUTTON PRESSED & OBJECTED NOT DETECTED/DETECTED
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
    GPIO_write(Direction_Pin, LOW);      //direction is not requred in this case

    //intialization for exti PA_4
    EXTI_init(BUTTON_PIN, FALL, 10); 

    //IR proximity initialization
    GPIO_init(proximity,INPUT);
    GPIO_pupd(proximity, 1); //pull-up

    //LED initialization
    GPIO_init(LED_PIN, OUTPUT);
    

    //TIM2 Update-Event Interrupt every 1 msec
    TIM_UI_init(TIM3, 500);           // UEV every 1 msec
    TIM_UI_enable(TIM3);
    //TIM_UI_init_us(TIM2, 1000);   // UEV every 1000usec=1msec

    //pwm initialization with 20msec
    PWM_init(PA_0,1);           //TIM2_CH1 (1msec)


    //USART2 initialization
    USART2_init(); 

}

int main(){

    uint32_t last_print = 0;

    setup();

    while(1){


        //X는 B1 버튼, Y = IR SENSOR (PAUSE)
        //proximity가 실제로 active low인지 확인해보기 
        if(GPIO_read(proximity) == 0){
            pause = 1; 

            X = 0;         //MODE를 바꾸지 않는다. - BUTTON PRESSED가 안된걸로 간주.
            PWM_duty(PA_0,table[mode].dutyratio[X][pause]); // pause주기 
        }else{
            //물체가 더이상 감지되지 않아서 다시 팬이돈다
            pause = 0;
            
            //pwm 속도 출력
            PWM_duty(PA_0,table[mode].dutyratio[X][pause]);
           
            if(table[mode].LED[X][pause]==LED_ON){
                GPIO_write(LED_PIN,HIGH);
            }else{
                //state table에서 led low라고 써져있다면
                GPIO_write(LED_PIN,LOW);
            }

            //모드는 S0->S1->S2 이런걸 유지하되,
            mode = table[mode].next[X][pause];
            //BUTTON은 RESET을 시켜야함
            X = 0;
        }

    
        


        uint32_t printduty = (uint32_t)(table[mode].dutyratio[0][pause] * 100);

        
         if( table[mode].LED[0][pause] == LED_OFF){
            if(timer_check(&last_print, 2000)){
                printf("The Mode : %s, Duty Ratio : %lu%%\r\n",modeName[mode],(unsigned long)printduty);
                printf("LED OFF\r\n");
            }
        }else if(table[mode].LED[0][pause] == LED_ON){
            if(timer_check(&last_print, 2000)){
                printf("The Mode : %s, Duty Ratio : %lu%%\r\n",modeName[mode],(unsigned long)printduty);
                printf("LED ON\r\n");
            }
        }
        else{
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
// LED[0][pause] : button input does not affect LED blink (Blink for both X=0,1 when Y=T)
        if (table[mode].LED[0][pause] == LED_Blink) {
            led_toggle(LED_PIN);
        }
    }
}

//BUTTON CHANGES MODE
void EXTI15_10_IRQHandler(void) {  //careful for pin number
  static uint32_t last_button = 0;
    if(is_pending_EXTI(BUTTON_PIN)){                //check if the EXTI pending flag is set
        clear_pending_EXTI(BUTTON_PIN);  //clear the EXTI pending flag
       //물체가 탐지가 안될때만 버튼모드가 발생할 수 있도록 
        if(timer_check(&last_button, 200) && GPIO_read(proximity) == HIGH){
            //BUTTON PRESSED
            X = 1;

        }               
    }
}




void USART2_IRQHandler(){       // USART2 RX Interrupt 
	if(is_USART2_RXNE()){
		 USART2_read();		// RX from UART2 (PC)
		 
  }
}