//DISTANCE연습하기 

#include "ecSTM32F4v2.h"


volatile uint32_t timer_arr  = 65536;    // ticks per counter wrap (ARR+1)
volatile float distance = 0;
volatile uint32_t cap_prev = 0;
volatile uint32_t cap_curr = 0;
volatile uint32_t counts = 0;
volatile float TimeInterval = 0;
volatile uint32_t uTick = 10;
volatile uint32_t ovf_cnt = 0 ;
volatile uint32_t tim_arr = 0;
void setup(void){
	
	RCC_PLL_init();
    SysTick_init();
    USART2_init();        

	PWM_init(PA_6,100);
	PWM_pulsewidth_us(PA_6,10);

	ICAP_init(PB_6, IC_RISE);
	//rise만이 아니라 fall도해줘야함
	ICAP_ICn_init(PB_6,IC_FALL,IC_2);   
    ICAP_tick(PB_6, 10);         

    timer_arr=TIM4->ARR + 1;

}


int main(){
	setup();
	printf("start distance\r\n");
	while(1){
		printf("Distance : %.2f\r\n",distance);
		delay_ms(1000);
	}

}

void TIM4_IRQHandler(void){
	if(is_UIF(TIM4)){
		ovf_cnt++;
		clear_UIF(TIM4);
	}

	if(is_CCIF(TIM4,IC_1)){
		cap_prev = ICAP_read(PB_6,IC_1);
		ovf_cnt = 0;
		clear_CCIF(TIM4,IC_1);
	}
	if(is_CCIF(TIM4,IC_2)){
		cap_curr = ICAP_read(PB_6,IC_2);

		counts = (cap_curr - cap_prev) + (ovf_cnt * tim_arr);
		TimeInterval = counts * uTick;
		distance = TimeInterval / 58.0f;
		clear_CCIF(TIM4,IC_2);
	}
}


