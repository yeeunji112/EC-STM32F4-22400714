#include "ecSTM32F4v2.h"

#define IR1_PIN       PB_0      // left sensor  : ADC1_IN8 (rank 1)
#define IR2_PIN       PB_1      // right sensor : ADC1_IN9 (rank 2)
#define DCMOTOR       PA_0
#define Direction_Pin PC_2
#define LED_PIN       PA_5
#define THRESHOLD     1500      // value >= THRESHOLD : black

//IR 반사 센서 2개로 검은 선의 위치를 판단하고,
//DC 모터 1개의 속도를 바꾼다. 직선에서는 빠르게, 커브에서는 느리게, 선이 끝나면 멈춘다.
//PC 키보드 명령으로 출발·정지한다.

volatile char     cmd_data   = 0;   // last command character from PC
volatile uint32_t cmd_flag   = 0;   // set by USART2 ISR when a new command arrives
volatile uint32_t IR1 = 0;          // latest left value
volatile uint32_t IR2 = 0;          // latest right value
volatile uint32_t seq = 0;          // 0 : next EOC is IR1, 1 : next EOC is IR2
volatile uint32_t mode = 1;         // 0 : RUN, 1 : STOP (start stopped)
volatile uint32_t last_print = 0;

PinName_t seqCHn[2] = {IR1_PIN, IR2_PIN};   // conversion order : IR1 -> IR2

void command(void);

void setup(){
    RCC_PLL_init();             // 84 MHz
    SysTick_init();
    USART2_init();

    ADC_init(IR1_PIN);
    ADC_init(IR2_PIN);
    ADC_sequence(seqCHn, 2);
    ADC_trigger_init(TIM3, 1, ADC_RISE);

    // DC motor
    PWM_init(DCMOTOR, 1);
    PWM_duty(DCMOTOR, 0);

    // Direction pin : fixed
    GPIO_init(Direction_Pin, OUTPUT);
    GPIO_otype(Direction_Pin, 0);       // push-pull
    GPIO_write(Direction_Pin, LOW);

    // LED : ON = RUN, OFF = STOP
    GPIO_init(LED_PIN, OUTPUT);
}

int main(){
    setup();
    while(1){
        command();

        uint32_t left  = IR1;
        uint32_t right = IR2;
        uint32_t Lb = (left  >= THRESHOLD);   // 1 = black
        uint32_t Rb = (right >= THRESHOLD);

        if(mode == 0){                          // RUN
            GPIO_write(LED_PIN, HIGH);

            if(!Lb && !Rb){                     // both white : line between -> straight, fast
                PWM_duty(DCMOTOR, 0.8f);
                if(timer_check(&last_print, 500)) printf("L=%d R=%d  STRAIGHT 80%%\r\n", (int)left, (int)right);
            }
            else if(Lb && !Rb){                 // line under left sensor -> curve, slow
                PWM_duty(DCMOTOR, 0.4f);
                if(timer_check(&last_print, 500)) printf("L=%d R=%d  CURVE LEFT 40%%\r\n", (int)left, (int)right);
            }
            else if(!Lb && Rb){                 // line under right sensor -> curve, slow
                PWM_duty(DCMOTOR, 0.4f);
                if(timer_check(&last_print, 500)) printf("L=%d R=%d  CURVE RIGHT 40%%\r\n", (int)left, (int)right);
            }
            else{                               // both black : line end -> stop
                PWM_duty(DCMOTOR, 0.0f);
                if(timer_check(&last_print, 500)) printf("L=%d R=%d  LINE END 0%%\r\n", (int)left, (int)right);
            }
        }
        else{                                   // STOP
            GPIO_write(LED_PIN, LOW);
            PWM_duty(DCMOTOR, 0.0f);
        }
    }
}

// process PC keyboard command : G = run, S = stop
void command(void){
    if(cmd_flag == 0) return;
    cmd_flag = 0;

    if(cmd_data == 'G' || cmd_data == 'g'){
        mode = 0;
        printf("[RUN]\r\n");
    }
    else if(cmd_data == 'S' || cmd_data == 's'){
        mode = 1;
        printf("[STOP]\r\n");
    }
}

void ADC_IRQHandler(void){
    if(is_ADC_OVR()){               // a result was not read in time
        clear_ADC_OVR();            // must clear, or the ADC stops
        seq = 0;                    // restart the order from IR1
    }
    if(is_ADC_EOC()){
        if(seq == 0){ IR1 = ADC_read(); seq = 1; }
        else        { IR2 = ADC_read(); seq = 0; }
    }
}

// USART2 RX interrupt : receive command from PC keyboard
void USART2_IRQHandler(void){
    if(is_USART2_RXNE()){
        char c = USART2_read();
        if(c != '\r' && c != '\n'){
            cmd_data = c;
            cmd_flag = 1;
        }
    }
}