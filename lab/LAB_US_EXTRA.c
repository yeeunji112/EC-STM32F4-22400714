
/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : Yeeun Ji
Created          : 10-08-2026
Modified         : 10-19-2026
Language/ver     : C in VS Code

Description      : USART communication between two MCUs
                   and remote LED control
/----------------------------------------------------------------*/

//엔터를 상대방이 쳐야 led 가 실행되는ㄴ
#include "ecSTM32F4v2.h"

#define MAX_BUF 20
#define END_CHAR 13

#define LED_PIN PA_5  // LD2

// USART2: PC -> Local MCU
static volatile uint8_t PC_data = 0;

// USART1: Other MCU -> Local MCU
static volatile uint8_t us1_data = 0;
static volatile uint8_t us1_buffer[MAX_BUF] = {0, };
static volatile uint8_t us1_string[MAX_BUF] = {0, };
static volatile uint8_t us1_idx = 0;

static volatile int bReceive = 0;


void setup(void){

    // System Clock: PLL 84 MHz
    RCC_PLL_init();
    SysTick_init();

    // USART2: PC communication
    USART2_init();  // Default 9600 baud

    // USART1: Communication with other MCU
    USART1_init();  // Default 9600 baud

    // LED initialization
    GPIO_init(LED_PIN, OUTPUT);
}


int main(void){

    setup();

    printf("MCU Initialized\r\n");

    while(1){

        // A complete message was received from the other MCU.
        if(bReceive == 1){
            // enter을 상대방이 쳐서 내가 받았을때 
            // Turn ON the local LED when "H" is received.
            if(strcmp((char *)us1_string, "H") == 0){
                GPIO_write(LED_PIN, HIGH);
            }

            // Turn OFF the local LED when "L" is received.
            else if(strcmp((char *)us1_string, "L") == 0){
                GPIO_write(LED_PIN, LOW);
            }

            bReceive = 0;
        }
    }
}

//usart 2 내 컴퓨터 
// USART2: Receive characters from the local PC.
void USART2_IRQHandler(void){

    if(is_USART2_RXNE()){

        PC_data = USART2_read();

        // Optional local echo:
        // USART2_write((uint8_t *)&PC_data, 1);

        // Forward each received character immediately
        // to the other MCU through USART1.
        //
        // This also forwards END_CHAR (Enter).
        // 문자를 받을때마다 상대 mcu로 전송한다. 
        ///usart1 은 친구 컴퓨터 
        USART1_write((uint8_t *)&PC_data, 1);
    }
}


// USART1: Receive characters from the other MCU.
void USART1_IRQHandler(void){

    if(is_USART1_RXNE()){

        us1_data = USART1_read();

        // Display each received character immediately
        // on the local PC through USART2.
        //친구 컴퓨터에서 받은걸 내꺼에 직접 표시한다. 
        USART2_write((uint8_t *)&us1_data, 1);

        // Store received characters until Enter is received.
        if(us1_data != END_CHAR && us1_idx < MAX_BUF - 1){

            us1_buffer[us1_idx] = us1_data;
            us1_idx++;
        }

        // Enter indicates the end of a message.
        else if(us1_data == END_CHAR){

            // Copy the completed message.
            memset((void *)us1_string, 0, MAX_BUF);
            memcpy((void *)us1_string,
                   (const void *)us1_buffer,
                   us1_idx);

            // Reset the receiving buffer.
            memset((void *)us1_buffer, 0, MAX_BUF);
            // 여기서 berecieve가 1이 되고 이걸 main으로 보낸다. 
            us1_idx = 0;
            bReceive = 1;
        }

        // Handle messages exceeding the buffer size.
        else{

            us1_idx = 0;

            memset((void *)us1_string, 0, MAX_BUF);
            memset((void *)us1_buffer, 0, MAX_BUF);

            // Avoid printf inside the interrupt handler.
        }
    }
}
