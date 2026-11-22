/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : YeeunJi
Created          : 05-03-2021
Modified         : 11-05-2026
Language/ver     : C++ in VS Code

Description      : LAB -  timer_icap 
/----------------------------------------------------------------*/
//TIME INTERVAL이 US기 때문에 출력문구가 MS이라면 TIMEINTERVAL/1000을 해줘야 MS이된다

#include "ecSTM32F4v2.h"


#define ICAP_PIN    PB_6        // echo //TIM4_CH1 : capture input
#define PWM_PIN     PA_6        // trigger output(PWM) // TIM3_CH1

uint32_t pwm_period_ms=100; //TRIG period [ms]
uint32_t uTick=10; //ICAP counter tick [us]
uint32_t timer_arr  = 65536;    // ticks per counter wrap (ARR+1) b/c 16bit-> fixed
volatile uint32_t ovf_cnt = 0;      //counts overflows
volatile uint32_t cap_prev   = 0;   // CCR1 value at the rising edge
volatile uint32_t cap_curr    = 0;  // CCR2 value at the falling edge
volatile uint32_t counts = 0;       // ECHO High width [ticks]
volatile float timeInterval = 0;    //ECHO HIGH width [us]
volatile float  distance   = 0;   //measured distance [cm]


void setup(void){
    RCC_PLL_init();        //system clock = 84MHz
    SysTick_init();       //1ms SysTick for delay_ms()
    USART2_init();        // for teraterm            

    //  TRIG : PWM on PA_6 (TIM3_CH1)
    PWM_init(PWM_PIN, pwm_period_ms); // AF mode, TIM3 PWM mode 1
    PWM_pulsewidth_us(PWM_PIN, 10); //10us high

    GPIO_otype(PWM_PIN,0); //push-pull
    GPIO_pupd(PWM_PIN,0); //no pull up/pull down
    GPIO_ospeed(PWM_PIN,2); // fast

    // Input capture on PB_6 (TIM4_CH1)   
    ICAP_init(ICAP_PIN, IC_RISE);   //AF, no pull up pull down
    ICAP_ICn_init(ICAP_PIN, IC_FALL, IC_2);     //TI1 -> IC2, falling
    ICAP_tick(ICAP_PIN, uTick);   //  1 tick = 10us(call LAST)    

    timer_arr=TIM4->ARR + 1; //ticks per overflow, read after ICAP_tick() sets ARR
}

int main(void){
    setup();
        
    printf("Start measuring the Distance\r\n");

    while(1){
        
        if(distance < 2.0f || distance > 400.0f){
            printf("Distance : %.2f [cm] (out of range)\r\n", distance);
        }else{
            printf("Distance : %.2f [cm]\r\n", distance);  //print latest distance
        }
        delay_ms(500);                                //print every 500ms
    }
}

//UIF역할: 카운터가 ARR에서 0으로 돌아갈때
//CCIF 역할: 입력 핀에 설정한 EDGE 가 들어와서 CCR에 값이 저장될때 

//pin: PB6 --> use TIM4
//TIM4 interrupt: overflow + rising/falling edge capture
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
        timeInterval=counts*uTick;                  //[us]
        distance = timeInterval/58.0f;              //[cm]
        clear_CCIF(TIM4, IC_2);
    }   
}