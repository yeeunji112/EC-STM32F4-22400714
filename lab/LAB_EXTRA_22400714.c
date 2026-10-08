/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           :  YeeunJi
Created          : 05-03-2021
Modified         : 10-05-2026
Language/ver     : C++ in VS Code

Description      : LAB -  EXTI 7 segment (0~59)
/----------------------------------------------------------------*/


// #include "ecRCC2.h"
// #include "ecGPIO2.h"
// #include "ecSysTick2.h"
// #include "ecEXTI2.h"

#include "ecSTM32F4v2.h"

#define SW2_PIN    PA_4			//EVAL board JKIT SW2

#define  SW1_PIN        PD_2      // sw1, reset 



PinName_t pinFND[8]    = {PB_0, PB_1, PB_2, PB_3, PB_4, PB_5, PB_6, PB_7}; //a,b,c,d,e,f,g,dot
PinName_t selectFND[4]= {PA_10, PA_11, PC_4, PC_3};
uint8_t numDisplay[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

volatile int change = 0 ;
volatile int pause = 0 ;

volatile uint8_t numDisplay1  = 0;
volatile uint8_t numDisplay2  = 0;

// Initialiization 
void setup(void)
{
    // RCC_PLL_init();                 // System Clock = 84MHz
    // SysTick_init();                 // SysTick Timer Initialization

    MCU_init();                        


    //Initialization SW1 
    GPIO_init(SW1_PIN,INPUT);
    GPIO_pupd(SW1_PIN, 1); //pull-up

    GPIO_init(SW2_PIN,INPUT);
    GPIO_pupd(SW2_PIN, 1); // pull - up

    // Initialize 7 semgnet
    FND_display_init(pinFND);

    for(int i = 0 ; i<8; i++)
{
    GPIO_otype(pinFND[i],0); // push pull 
    GPIO_pupd(pinFND[i],0);
    GPIO_ospeed(pinFND[i],1);
}    


    FND_select_init(selectFND);
    for(int i = 0 ; i< 2 ; i++){
        GPIO_otype(selectFND[i],0);
    }

    //intialization for exti PA_4
    EXTI_init(SW2_PIN, FALL, 10);

    //intialization for exti PD_2
    EXTI_init(SW1_PIN, FALL, 10);
}


// MAIN  ----------------------------------------
int main(void) {
 
   //count up at the rate of 1 sec 
	setup();

    //use two digits in the 7 segment display    
    uint8_t selectFND1  = 0;
    uint8_t selectFND2  = 1;
 

	while (1){
        
        //1ms가 두개 즉, 2ms 이니까. 1000ms 만들기 위해 for문은 500까지 돌림 
        //multiplexing 사용 
        for(int i = 0 ; i< 500; i++){
            FND_select(selectFND1); 
            FND_display(numDisplay1);
            delay_ms(1);

            //십의자리 select하고 
            FND_select(selectFND2);
            FND_display(numDisplay2);
            delay_ms(1);
        }
      


        
        //만약 delay 실행중에 interrupt가 발생했다면, 0으로 확실히 reset 되게 하기 
        if(change == 1){
            
            //다시 첨부터 count 하기 위해서 
            change = 0;
            continue; // 이걸쓰면 0으로 바뀐뒤 1초간 시간이 유지됨 
        }


        //sw2를 누르면 pause되야함. 
        if( pause == 1 ){  
            //FND_display(increase);   
            continue; // 밑에줄 increase를 뛰어넘고 계속해서 pause 할 수 있도록 
        }
        
        numDisplay1 = (numDisplay1 + 1) % 10;  //show 일의자리 0부터 9 하고 반복
        if(numDisplay1 == 0){
            numDisplay2 = (numDisplay2+1)%6;   //SHOW 십의자리 0부터 5 하고 반복
        }
    }

}



//PA_4 ( SW2 누르면 PAUSE ) // 여것도 change라는 변수를 써야하나 
void EXTI4_IRQHandler(void) { //EXTI4도 달라짐. PIN 번호가 달라질테니까 //PA_4
  
    if(is_pending_EXTI(SW2_PIN)){                          //is_pending_EXTI == 1 
    //FND_display(increase);//이건 안써도 됨 . 차피 맨 앞에 while이 할거임 
    pause ^= 1;  // pause라는 의미기 때문에, 다시 되돌릴줄도 알아야함. 영영멈추지 않게 하기 위해 토글 
    clear_pending_EXTI(SW2_PIN);
    }               

}

//sw1 == PD_2 ( 누르면 RESET)
void EXTI2_IRQHandler(void){
    if(is_pending_EXTI(SW1_PIN)){
        
        numDisplay1 = 0 ;          // 일의자리 RESET
        numDisplay2 = 0 ;          // 십의자리 RESET 
    
        change = 1;                //전역변수 CHANGE
        clear_pending_EXTI(SW1_PIN);
    }
}


