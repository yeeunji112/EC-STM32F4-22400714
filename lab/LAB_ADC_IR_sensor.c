/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : YeeunJi
Created          : 11-21-2026
Modified         : 11-21-2026
Language/ver     : C in VS Code

Description      : LAB - ADC IR Reflective Sensor (TCRT5000 x2), line tracing
                   - Regular group, 2 channels (scan): PB_0 = ADC1_IN8, PB_1 = ADC1_IN9
                   - TIM3 TRGO (update event) every 1 ms -> 1 kHz sequence
                   - EOC interrupt after EACH conversion -> read IR1, then IR2
                   - Print IR1, IR2 and GO LEFT / GO RIGHT every 1 sec
/----------------------------------------------------------------*/

//ADC란 
//. 예를 들어 TIM3를 쓰라고 하면 Regular만 가능하고, TIM1/4/5를 쓰라고 하면 Injected만 가능해요.
//Regular는 채널 많고 결과 레지스터가 하나인 기본 변환, Injected는 최대 4채널에 결과 레지스터가 따로 있고 우선순위가 높은 끼어들기 변환이에요.
//IR 반사 센서(TCRT5000) 2개를 ADC로 읽어서, 바닥의 검은 선이 두 센서 사이에 있도록 왼쪽·오른쪽 방향을 판단하는 거예요. 이 LAB은 나중에 라인 트레이싱 카 프로젝트로 확장돼요.

#include "ecSTM32F4v2.h"

// ---------------------------------------------------------------
// Pin / parameter
// ---------------------------------------------------------------
#define IR1_PIN     PB_0        // left sensor  : ADC1_IN8  (rank 1)
#define IR2_PIN     PB_1        // right sensor : ADC1_IN9  (rank 2)

// Measure first, then set this value.
// TCRT5000 module : white (strong reflection) -> small value
//                   black (weak reflection)   -> large value
// Put the sensor on white and on black, print the values, take the middle.
#define THRESHOLD   1500

PinName_t seqCHn[2] = {IR1_PIN, IR2_PIN};   // conversion order : IR1 -> IR2

volatile uint32_t IR1 = 0;      // latest left value
volatile uint32_t IR2 = 0;      // latest right value
volatile uint32_t seq = 0;      // 0 : next EOC is IR1, 1 : next EOC is IR2


void setup(void){
    RCC_PLL_init();             // 84 MHz
    SysTick_init();             // delay_ms
    USART2_init();              // printf -> Tera Term (9600)

    // 1. Analog pins + ADC clock / 12-bit / sampling time (inside ADC_init)
    ADC_init(IR1_PIN);
    ADC_init(IR2_PIN);

    // 2. Two-channel scan sequence : SQ1 = IN8, SQ2 = IN9, L = 1, SCAN = 1
    ADC_sequence(seqCHn, 2);

    // 3. HW trigger : TIM3 update event (TRGO) every 1 ms, rising edge
    //    -> EXTSEL = 1000 (TIM3 TRGO), EXTEN = 01, EOC interrupt enabled
    ADC_trigger_init(TIM3, 1, ADC_RISE);
}


int main(void){
    setup();
    printf("LAB ADC IR start\r\n");

    while(1){
        // copy once so both values come from the same moment
        uint32_t left  = IR1;
        uint32_t right = IR2;

        printf("IR1 = %lu   IR2 = %lu\r\n", left, right);

        // The line should stay BETWEEN the two sensors.
        // If one sensor sees the black line, the car drifted -> steer toward that side.
        if(left > THRESHOLD && right <= THRESHOLD){
            printf("GO LEFT\r\n");          // line is under the left sensor
        }
        else if(right > THRESHOLD && left <= THRESHOLD){
            printf("GO RIGHT\r\n");         // line is under the right sensor
        }
        else if(left <= THRESHOLD && right <= THRESHOLD){
            printf("GO STRAIGHT\r\n");      // both white : line is between
        }
        else{
            printf("STOP (both black)\r\n");
        }
        printf("\r\n");

        delay_ms(1000);                     // print every 1 sec
    }
}


// One trigger -> 2 conversions -> 2 EOC interrupts (EOCS = 1)
// Regular group has ONE data register (DR), so read it after every EOC.
void ADC_IRQHandler(void){
    if(is_ADC_OVR()){               // a result was not read in time
        clear_ADC_OVR();            // must clear, or the ADC stops
        seq = 0;                    // restart the order from IR1
    }

    if(is_ADC_EOC()){
        if(seq == 0){
            IR1 = ADC_read();       // reading DR clears EOC
            seq = 1;
        }else{
            IR2 = ADC_read();
            seq = 0;
        }
    }
}
