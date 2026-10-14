/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : YeeunJi
Created          : 05-03-2021
Modified         : 09-28-2026 
Language/ver     : C++ in VS Code

Description      : SysTick Library. Distributed to Students for LAB_SYSTICK
/----------------------------------------------------------------*/


// #include "ecSysTick2.h"
#include "ecSysTick2.h"


volatile uint32_t msTicks;

// Initialize 1 ms tick of SysTick timer. 
// Must call AFTER RCC_HSI_init() (16 MHz) or RCC_PLL_init() (84 MHz)
// Updates  EC_SYSCLK either 84 MHz or 16 MHz
void SysTick_init(void){
	//  SysTick Control and Status Register
	// Disable SysTick IRQ and SysTick Counter
	SysTick->CTRL = 0;											
	
	// Select processor clock
	// 1 = processor clock;  0 = external clock
	SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;

	// SysTick Reload Value Register : (SYSCLK / 1000) - 1  ->  1 ms
	//   HSI 16 MHz :  15999      PLL 84 MHz : 83999 
	SysTick->LOAD = (uint32_t)EC_SYSCLK *1UL/ 1000UL - 1UL;   // 1[ms] = 1/1000[s] 

	// SysTick Current Value Register : Reset counter value to 0
	SysTick->VAL = 0;

	// Enables SysTick exception request
	// 1 = counting down to zero --> SysTick exception request
	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
	
	// Enable SysTick IRQ and SysTick Timer
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
		
	// Enable interrupt in NVIC
	NVIC_SetPriority(SysTick_IRQn, 15);		// Set Priority to 1
	NVIC_EnableIRQ(SysTick_IRQn);			// Enable interrupt in NVIC
}


// SysTick_Handler() is called every 1 ms by the SysTick IRQ
void SysTick_Handler(void){
	SysTick_counter();	
}

// Increment msTicks every 1 ms
void SysTick_counter(void){
	msTicks++;
}	

// Blocking delay in milliseconds
void delay_ms (uint32_t msec){
  	uint32_t curTicks;
  	curTicks = msTicks;
	while ((msTicks - curTicks) < msec){;}		// unsigned math: safe across wrap-around
	//while문이 내가 원하는 카운트 만큼 돌아가는 것. 
	//dealy 호출 할때 그 순간 msTick 값을 cur에 저장하고 msTick은 계속 증가하는중. 
	//그래서 while에서 msec보다 언젠간 클 수 밖에 없음. 내가 원하는 차이만큼 발생하면 나와라. 
}


// Reset the SysTick counter to 0
void SysTick_reset(void)
{
	SysTick->VAL = 0;
}

// Return the current value of the SysTick counter
uint32_t SysTick_val(void) {
	return SysTick->VAL;
}


// Blocking delay in microseconds
// void delay_us(uint32_t usec)
// {
//     uint32_t ticksPerUs = (uint32_t)EC_SYSCLK / 1000000UL;
//     uint32_t reload = SysTick->LOAD + 1UL;
//     uint32_t previous = SysTick->VAL;
//     uint64_t elapsedTicks = 0;
//     uint64_t targetTicks = (uint64_t)usec * ticksPerUs;

//     while (elapsedTicks < targetTicks)
//     {
//         uint32_t current = SysTick->VAL;

//         if (previous >= current)
//             elapsedTicks += previous - current;
//         else
//             elapsedTicks += previous + reload - current;

//         previous = current;
//     }
// }
