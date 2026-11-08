

//check the bluetooth connection L이랑 h 를 pc에서 보냄으로써
//connect stepper motor 해서
//J면 cw direction
//k면 ccw direction
//p면 emergency stop
//j 도중에 k가 눌리면 j 멈추고 k 운동
//j 도중에 j 눌리면 현재 멈추고 다시 j 2바퀴 
 
#include "ecSTM32F4v2.h"


//pin name definition
PinName_t pinStepper[4]    = {PB_10,PB_4,PB_5,PB_3};
static volatile uint8_t PC_Data = 0;
static volatile uint8_t BT_Data = 0;
volatile uint32_t steps; //여기서는 STEPS UNIT32 써도 되나
volatile uint32_t right = 0; 
volatile uint32_t left = 0;
volatile uint32_t stop;

volatile uint32_t moving = 0;


//BELOW, NOT USED 
uint8_t PC_string[]="Loop:\r\n";

// Select stepping mode: FULL or HALF
uint32_t mode = FULL;

#define LED_PIN    PA_5 //LD2

void setup(void){
    //PLL 84MHZ
	RCC_PLL_init();
	SysTick_init();
	
	// USART2: USB serial init
	USART2_init();  // default 9600
	//USART2_baud(BAUD_38400);
	
	// USART1: MCU2 serial init 
	USART1_init(); // default 9600
	//USART1_baud(BAUD_38400);

     //LED PIN INTIALIZATION - FOR BLUTOOTH
    GPIO_init(LED_PIN, OUTPUT);    // calls RCC_GPIOB_enable()

	stepper_init(mode, pinStepper); 				// Stepper GPIO pin initialization
	stepper_speed(10);                 		//  set stepper motor speed(based on selected stepping mode

}



int main(void){	
	setup();
    int32_t rev=2;           // one revolution = one round

    //calculate the required steps based on the stepping mode
	if (mode == FULL){
		//Full step = 2048 steps per revolution
		steps = STEP_PER_REV*rev;
	}
	else{
		//half-step : 4096 steps per revolution
		steps=STEP_PER_REV*2*rev;
	}


	printf("MCU Initialized\r\n");	
	while(1){
		// USART Receive: Use Interrupt only
		// USART Transmit: Polling
		//USART2_write(PC_string, 7);

        if(right == 1){
            right = 0;
            moving = 1;
            stepper_step(steps, DIR_CW);  // 이거 실제론 CCW
            moving = 0;
            //right = 0으로 하게된다면 오른쪽돌리는와중에 멈추게 되고 다시
            //J를 누르면 원래는 돌아야하는데 
            //interrupt 끝나고 돌아와서 나머지를 돈 다음에 right가 0이 된다. 
        }
        if(left == 1){
            left = 0;
            moving =1;
            stepper_step(steps, DIR_CCW);  // 이거 실제론 CCW
            moving = 0;
            
        }
         
	}
	
	


}

//내컴퓨터 
void USART2_IRQHandler(){       // USART2 RX Interrupt 
	if(is_USART2_RXNE()){
		PC_Data = USART2_read();		// RX from UART2 (PC)
        //밑에 수행되면 한번더 teraterm에 뜰것같음..
		//USART2_write(&PC_Data,1);		// TX to USART2	 (PC)	 Echo of keyboard typing
        //블루투스도 이게 적히나..??
    }
}
//j누르고 나서 k를 누른다면 즉시바뀌여야 하나
//만약 그렇다면
void USART1_IRQHandler(){          	// USART2 RX Interrupt
	if(is_USART1_RXNE()){          //다른친구가 나한테 보냈어 
		BT_Data = USART1_read();				// RX from other MCU UART1	
        if(BT_Data == 'L'){
            GPIO_write(LED_PIN,LOW);
        }else if(BT_Data == 'H'){
            GPIO_write(LED_PIN,HIGH);
        }else if(BT_Data == 'J'){
            right = 1;
            left = 0;
            if(moving == 1){
                stepper_stop();
            }
        }else if(BT_Data == 'K'){
            //right과 연속으로눌릴경우를 대비해서 left는 2로 준다. 
            left = 1;
            right = 0;
            if(moving ==1){
                stepper_stop();
            }
        }else if(BT_Data == 'P'){
            //stop 안에 step = 0 으로 만들어버림 
            stepper_stop();
            //reset 
            right = 0;
            left = 0;
        } 
        
	}
}

   