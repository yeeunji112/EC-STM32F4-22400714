
#include "ecSTM32F4v2.h"


//초음파 센서로 사람이 얼마나 가까이 있는지 재고, 
//가까울수록 팬을 세게 돌린다. 
//버튼으로 전체 동작을 멈추거나 다시 시작한다.

#define FAN PA_0
#define Direction_Pin PC_2
#define Button_pin  PC_13
#define LED_PIN     PA_5
#define PWM_PIN     PA_6
#define ICAP_PIN    PB_6

#define FAN_OFF 0
#define FAN_MID 1
#define FAN_HIGH 2

const char *modeName[3] = {"OFF", "MID", "HIGH"};
const char *RUN[2] = {"RUN", "STOP"};

volatile uint32_t mode = 0;
volatile uint32_t ovf_cnt = 0;
volatile uint32_t cap_prev = 0;
volatile uint32_t cap_curr = 0;
volatile uint32_t counts = 0;
volatile float timeInterval = 0;
volatile float distance = 0;
volatile uint32_t pause = 0;
volatile uint32_t last_print = 0;
uint32_t timer_arr  = 65536;    // ticks per counter wrap (ARR+1) b/c 16bit-> fixed

void setup(){
    RCC_PLL_init();        //system clock = 84MHz
    SysTick_init(); 

    PWM_init(PWM_PIN, 100); // AF mode, TIM3 PWM mode 1
    PWM_pulsewidth_us(PWM_PIN, 10); //10us high


   // Input capture on PB_6 (TIM4_CH1)   
    ICAP_init(ICAP_PIN, IC_RISE);   //AF, no pull up pull down
    ICAP_ICn_init(ICAP_PIN, IC_FALL, IC_2);     //TI1 -> IC2, falling
    ICAP_tick(ICAP_PIN, 10);   //  1 tick = 10us(call LAST)    
    timer_arr=TIM4->ARR + 1; //ticks per overflow, read after ICAP_tick() sets ARR

    PWM_init(FAN,1);
    PWM_duty(FAN,0.0f);
    
    GPIO_init(Direction_Pin,OUTPUT);
    GPIO_otype(Direction_Pin, 0);           //push-pull
    GPIO_write(Direction_Pin, LOW);   


    EXTI_init(Button_pin,FALL,10);

    GPIO_init(LED_PIN,OUTPUT);
    TIM_init(TIM5,500);

    USART2_init();
}


int main(){
    setup();
    while(1){

      
        if(pause == 1){
            PWM_duty(FAN,0);
        }else{

            if(distance>=2&&distance<10){
                PWM_duty(FAN,1.0);
                mode = FAN_HIGH;
            }else if(distance>=10&&distance<30){
                PWM_duty(FAN,0.5);
                mode = FAN_MID;
            }else if(distance>=30&&distance<=400){
                PWM_duty(FAN,0.0);
                mode = FAN_OFF;
            }else{
                PWM_duty(FAN,0.0);
                printf("out of range\r\n");
            }
        }
            if(timer_check(&last_print, 2000)){
                //use %s for string
                if(pause == 0){
                 printf("Distance: %.2f, RUN, modeName[mode]\r\n",distance);

                }else{
                    printf("Distance: %.2f, STOP, (0%%)\r\n",distance);

                }
               
            }
    }
}





void USART2_IRQHandler(){       // USART2 RX Interrupt 
	if(is_USART2_RXNE()){
		 USART2_read();		// Rread and clear recevied data
		 
  }
}

void EXTI15_10_IRQHandler(void) {  //careful for pin number
  static uint32_t last_button = 0;
    if(is_pending_EXTI(Button_pin)){                //check if the EXTI pending flag is set
        clear_pending_EXTI(Button_pin);  //clear the EXTI pending flag
        pause^= 1;

    }               
}

void TIM4_IRQHandler(void){
    // Counter overflow 
    if(is_UIF(TIM4)){
        ovf_cnt++;                 //count overflows during ECHO HIGH
        clear_UIF(TIM4);
    }

    //RISING이 문제에 되어있기때문에 OVF_CNT를 RESET해주는곳은 여기다.
    // Rising edge captured on IC1
    if(is_CCIF(TIM4, IC_1)){                    // IC1 : rising edge, SAME pin (TI1)
        cap_prev = ICAP_read(ICAP_PIN, IC_1);  //Save start time(CCR1)
        ovf_cnt = 0 ;                           //start counting overflows from here(rising)
        clear_CCIF(TIM4,IC_1);                                
    }
    // Falling edge captured on IC2 :ECHO pulse ends
    if(is_CCIF(TIM4, IC_2)){                // IC2 : falling edge, SAME pin (TI1)       
        cap_curr = ICAP_read(ICAP_PIN, IC_2);  //save end time(CCR2)

        // Ticks between the two edges, then scaled to usec.        
        counts = (cap_curr-cap_prev) + (ovf_cnt*timer_arr);     //[ticks]
        timeInterval=counts*10;                  //[us]
        distance = timeInterval/58.0f;              //[cm]
        clear_CCIF(TIM4, IC_2);
    }   
}

void TIM5_IRQHandler(void) {
    if (is_UIF(TIM5)) {
        clear_UIF(TIM5);
        //check FSM LED during PAUSE
// LED[0][pause] : button input does not affect LED blink (Blink for both X=0,1 when Y=T)
        if (pause ==0){
            led_toggle(LED_PIN);
        }else{
            GPIO_write(LED_PIN,HIGH);
        }
}
}