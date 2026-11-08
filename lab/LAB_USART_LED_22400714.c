/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : Yeeun Ji
Created          : 10-08-2026
Modified         : 10-08-2026
Language/ver     : C in VS Code

Description      : turn on led with other mcu
/----------------------------------------------------------------*/


//USART2
//USART1


#include "ecSTM32F4v2.h"// this should include #include "ecUART2.h" 
// //#include "ecUART2.h" 


 #define MAX_BUF 30  // 
 #define END_CHAR 13     //(ENTER)
 #define LED_PIN    PA_5 //LD2

 //USART2: data rceived from the local pc
 static volatile uint8_t PC_data = 0;

 //flag indicating a complete message has been received
 static volatile int bReceive =0;



// USART1 : Data received from the other MCU
static volatile uint8_t us1_data = 0;
 static volatile uint8_t us1_buffer[MAX_BUF]={0, };
 static volatile uint8_t us1_string[MAX_BUF]={0, };
 static volatile uint8_t us1_idx = 0;


//
void setup(void){

    //Initialize system clock and Sytick 
	RCC_PLL_init();
	SysTick_init();
	
	// USART2: for PC COMMUNICATION
	USART2_init();  // default 9600
	//USART2_baud(BAUD_38400);
	
	// USART1: for communication with the other MCU
	USART1_init(); // default 9600
	//USART1_baud(BAUD_38400);

    //LED PIN INTIALIZATION
    GPIO_init(LED_PIN, OUTPUT);    // calls RCC_GPIOB_enable()

}

int main(void){
    setup();

    //display initialization message on the pc 
    printf("MCU Initialized\r\n");

    while(1){
      
          if(bReceive == 1){
            // Display the received message after Enter is detected
            printf("Received: %s\r\n", (char *)us1_string);

            //clear the receive flag
             bReceive = 0;
           }
        }
    }



// USART2 RX interrupt: Receive data from the local PC
void USART2_IRQHandler(){       // USART2 RX Interrupt 
	if(is_USART2_RXNE()){

        //read a character from the local pc 
		PC_data = USART2_read();    
        

       //send each character immdeiately to the other MCU
       
       USART1_write((uint8_t *)&PC_data, 1);


        //echo the character --> this should allow to show what is been written
        if(PC_data != END_CHAR){
        USART2_write((uint8_t *)&PC_data, 1);
           }   
       
	
        
        else{      
             // move to the next line when enter is perssed 
            uint8_t nextline[] = "\r\n";
            USART2_write(nextline, 2);
        }
    }
}



//Usart1 RX intterupt: receive data from the other mcu
void USART1_IRQHandler(){
    
    if(is_USART1_RXNE()){
        //read a character from the other mcu
        us1_data = USART1_read();
        
       
        // for ignoring NULL and Line feed character 
        if(us1_data == '\0' || us1_data == '\n') return;
        
        //control LD2 based on the receved character
        if(us1_data == 'H'){
            GPIO_write(LED_PIN, HIGH);
        }
        else if(us1_data == 'L'){
            GPIO_write(LED_PIN, LOW);
        }

        // store received characters until Enter is dected
        if(us1_data != END_CHAR && (us1_idx < MAX_BUF-1)){
            us1_buffer[us1_idx] = us1_data;
            us1_idx++;
        }
        else if (us1_data == END_CHAR){
            // Copy the complete message to the display buffer
            memset((void *)us1_string, 0, sizeof(us1_string));
            memcpy((void *)us1_string, (const void *)us1_buffer, us1_idx);
           
            //clear the recieve buffer for the next message 
            memset((void *)us1_buffer, 0, sizeof(us1_buffer));

           //notify main () that the complete message is available
            bReceive = 1;
            us1_idx = 0;
        }
        else{
            //reset the receive buffer if the message exceeds MAX_BUF
            us1_idx = 0;
            memset((void *)us1_buffer, 0, sizeof(us1_buffer));
           
        }
    }
}
