

//미니 스마트홈하기 
#include "ecSTM32F4v2.h"

#define button_pin  PA_10
#define PIR_PIN         PB_3
#define photosensor       PA_0
#define resistor        PC_1
#define FAN_PIN            PB_7
PinName_t seqCHn[2] = {PA_0, PC_1};   // JADC 순서 : JDR1 = 빛 센서, JDR2 = 가변저항


PinName_t LED_PIN[] = {PB_12, PB_13, PB_14, PB_15}; // LD0, LD1, LD2, LD3

void setup(){
    RCC_PLL_init();                             // 시스템 클럭 84MHz
    SysTick_init();                             // 1ms SysTick (timer_check, DHT11에 필요)
    USART2_init();
    
    GPIO_init(button_pin,INPUT);
    GPIO_pupd(button_pin,1);
    EXTI_init(button_pin,FALL,10);

    GPIO_init(PIR_PIN,INPUT);
    GPIO_pupd(PIR_PIN, 2);          // pull-down : 평소 LOW
    EXTI_init(PIR_PIN, RISE, 3);  
    
    JADC_init(PA_0);
    JADC_init(PC_1);
    JADC_sequence(seqCHn, 2);
    JADC_trigger_init(TIM5, 125, ADC_RISE);     // JEOC 인터럽트도 여기서 켜짐

        for(int i = 0; i < 4; i++) {

        GPIO_init(LED_PIN[i], OUTPUT);   // Calls RCC_GPIOB_enable()
        GPIO_pupd(LED_PIN[i], 1);        // Pull-up
        GPIO_otype(LED_PIN[i], 1);       // Open-drain
        GPIO_ospeed(LED_PIN[i], 1);      // Medium speed
    }

    PWM_init(FAN_PIN,1);
    USART6_init();

    TIM_UI_init(TIM1,250);

}

volatile uint32_t mode = 0;
volatile uint32_t tick_flag = 0;

int main(){
    setup();

    while(1){//0이 HOME
        if(mode == 0){

        }
    }
}

void led1(void){

}

void TIM1_UP_TIM10_IRQHandler(void){
    if(is_UIF(TIM1)){
        tick_flag = 1;
        clear_UIF(TIM1);
    }
}

void ADC_IRQHandler(void){
    if(is_ADC_JEOC()){
        light = JADC_read(1);       // JDR1 : 빛 센서
        pot   = JADC_read(2);       // JDR2 : 가변저항
        clear_ADC_JEOC();           // JEOC는 직접 지워야 함
    }
}

void USART6_IRQHandler(void){
    if(is_USART_RXNE(USART6)){
        char c = USART_read(USART6);
        if(c != '\r' && c != '\n'){
            bt_data = c;
            bt_flag = 1;
        }
    }
}