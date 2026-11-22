
#include "ecSTM32F4v2.h"
#include <string.h>

//led에서 가렸을때 3000, 안가렸을때 1000
//1000이 촉촉 3000이 마른흙(평소)

#define ventfan           PA_15 //Vent fan
#define window             PC_7  //window
#define LED_panel          PB_7  //led panel
#define photo_block         3600 //바꾸기 4095 가렸을때 
#define photo_unblock        1300 //바꾸기 안가렸을때 1099
#define temp_high          28//정하기 [] //27까지는 안올라가는데 내려가는시간이 너무오래걸림
#define temp_low           26//정하기 [23] - 1분동안 내려감 1도내려가는데 
#define soil_recovery      2100   //다시 촉촉상태로 돌아옴.
#define soil_dry           3000//정하기 100 3200   4095 //촉촉 1900
#define DHT11_PIN          PA_6 //(D_12)-온습도 
#define humid_high          47 //11/15 기준
#define humid_low          45//정하기 [27]
#define button_pin         PC_13  //버튼세팅
#define LED_PIN    PA_5 //LD2

static volatile char BT_Data = 0;  //bt data character 들어오는거
static volatile uint32_t mode; // 지워야함
volatile float saved_duty = 0; //auto에서 사용하는 duty
volatile float prev_duty = 0; //나중에사용하는 previous duty 기억하려고


 volatile uint32_t  bt_flag = 0; // USART6 ISR

volatile uint32_t light = 0;    //ADC value
volatile uint32_t water = 0;    //ADC value
 volatile uint32_t tick_flag = 0;  //500 ms 마다 TIM1 ISR
PinName_t seqCHn[2] = {PA_0, PA_1}; // photo sensor -> pa_0, 수분센서-> pa_1
volatile uint32_t PW = 0;
//volatile uint32_t open_once = 0;
volatile uint32_t window_open = 0 ; // 창문이 열렸는지를 기억하는 flag


volatile uint32_t cont_dry =0;   //DRY 함수 들어갔다 왓냐
volatile uint32_t cont_recovery = 0;  // RECOVERY 한번 들어갔다 왔냐
volatile uint32_t alarm_flag = 0;  // ALARM 나타내는 
volatile uint32_t mode_flag= 1;      //manual로 시작할 수 있게 
volatile uint32_t ack_message =0 ; //K MESSAGE 받았냐
volatile uint32_t  mode_count =0;  //AUTO 들어가기전 5초하려고
volatile  float value = 0 ;        //BLUETOOTH 에서 숫자 다시 변환하려고
volatile uint32_t hystery = 100;   //LED에 쓰이는 경계

volatile uint32_t dht_flag = 0;     //DHT11나중에 ERR 체크하려고
volatile float manual_duty  = 0;    //MANUAL LED DUTY 기억하려고
char lcd_line[20];
volatile uint32_t alarm_blink = 0;      //LCD, LED 1초마다 BLINK

volatile uint32_t vent_flag = 0;          //VENT FAN의 상태

volatile uint32_t led_count = 0;      //최소 ON 해주려고




void AUTO_led(void);

void read_dht11(void);
//void AUTO_led(void);
void AUTO_dht11(void);

//void AUTO_soil
void  auto_soil(void);

//ALARM
void alarm(void);


//bluetooth
void BLUETOOTH();

void print_LCD(void);

void setup(){

    //time쓸때 설정
    RCC_PLL_init();
    SysTick_init();
    
    USART2_init(); //debugging 용
    //printf("A\r\n");
    //timer
    TIM_UI_init(TIM1, 500);           

    //환기팬 (1ms)
    PWM_init(ventfan,1);
    PWM_duty(ventfan, 0.0f);

    //창문서보(20ms)
    PWM_init(window,20);
    PWM_pulsewidth_us(window,1200); //서보모터는 각도이기때문에 초기 값을줘야함.
    
    //led penal
    PWM_init(LED_panel,1);
    PWM_duty(LED_panel, 0.0f);

    //연습용
    // PWM_init(LED_PIN,1);
    // PWM_duty(LED_PIN, 0.0f);

    // JADC Init 
    JADC_init(PA_0);
    JADC_init(PA_1);

    // JADC channel sequence setting
    JADC_sequence(seqCHn, 2);

    // HW trigger : TIM5 TRGO @ 125msec - enables JEOC interrupt
    JADC_trigger_init(TIM5, 125, ADC_RISE);

    //USART1_init();
    
    USART_init(USART6,PA_11,PA_12,9600); //블루투스
    
    
    //온습도
    DHT11_init(DHT11_PIN);	// DATA = PB_12


    //LCD init
    I2C1_init();			// SCL = PB_8,  SDA = PB_9
    LCD_init();

}


#define AUTO    0
#define MANUAL  1
//volatile uint32_t mode = MANUAL; //시작값이 MANUALE?



int main(void){
        setup();

    
    //전원을 키면 디폴트가 led off, 팬 off, 창문닫힘, manual로 시작함.
	printf("MCU Initialized\r\n");	 //display on stm terminal

    uint8_t BTin[]="MCU Initalization\r\n";
    //USART_write(USART6,BTin, sizeof(BTin)-1);// display on bluetooth terminal

    //초기 세팅
    USART_write(USART6, (uint8_t*)"[INITIALLIZATION][MODE = MANUAL]\r\n", strlen("[INITIALLIZATION][MODE = MANUAL]\r\n"));
    USART_write(USART6, (uint8_t*)"[LED = OFF, VENTFAN = STOP, WINDOW = CLOSED]\r\n", strlen("[LED = OFF, VENTFAN = STOP, WINDOW = CLOSED]\r\n"));

    while(1){
        //auto일때 light
        //너무 밝으면 키고 

        if(bt_flag == 1){
            bt_flag = 0;
            BLUETOOTH();
        }

        if(tick_flag == 1){
            tick_flag = 0;
              
            //mode는 안에서 블루투스에서 바꾸는게 젤좋음. 
            read_dht11();   
         
            //IF AUTO  
            if(mode_flag == 0){
                    //5초뒤에 auto
                    if(mode_count<10){
                        mode_count++;
                    }else{
                       

                        AUTO_led();    
                        //환기(팬+창문서보 둘중 하나 상한값 초과)
                        AUTO_dht11();
                  
                    }

             }else{
 
                //manual에서 soil이 나중에 recovery될때 다시 원상복귀 될 수 있도록
                PWM_duty(LED_panel, manual_duty);
             }
        
            auto_soil(); //을 여기다가 추가하는게 맞을듯. 실험으로 확인. 
            print_LCD();
            
        }
    }
}




// //led panel을 led로 바꿔서 확인해보기 포토센서랑 함께 
void AUTO_led(void){
    static uint32_t led_first = 0;    //상한값에서 hysterisis
    static float final_duty = 0;
    static uint32_t led_second = 0;   //하한값 hysterisis
    char led_msg[40];
        
        //첫번째 경계
        if(led_first == 0){
            if(light>photo_unblock + hystery){
                led_first = 1; //led 듀티 계산되야지
            }
        }else{
                if(light<photo_unblock - hystery){
                    led_first = 0; //led 꺼져야지
                }
        }
        
        //두번째 경계
        if(led_second == 0){
            if(light>photo_block+hystery){
                led_second = 1; //완전히 켜져야지
            }
        }else{
            if(light<photo_block - hystery){
                led_second = 0; //duty로 계산되야지.
            }
        }

        if(led_first == 0){
            saved_duty = 0;
        }else if(led_second == 1){
            saved_duty = 1;
        }else{
            //led_on == 1 인상태나 led_full == 0인 상태 
            saved_duty = ((float)light-photo_unblock)/(photo_block - photo_unblock);
         
            if(saved_duty<0.05f){
                //여기서 saved_duty<0로 쓰면 0이 되야하는데 그럼 히스테리시스를 쓰는 의미가없다
                //근데 0.05일이유가 있나
                saved_duty = 0.05f;
            }else if(saved_duty > 1){
                saved_duty = 1;
            }
        }

         //최소 on 시간을 3초로 해보자 
         //켜져있을때 시간재고
         if(prev_duty > 0 && led_count < 6){
            led_count++;
         }
            //켜지는자마자 리셋
            if(prev_duty == 0 && saved_duty > 0){
            led_count = 0;
            }else if(prev_duty > 0 && saved_duty == 0){
                if(led_count<6){
                    saved_duty = prev_duty;
                }
        }
        

         //usart6 출력
         //alarm일때는 나가면 안되니까
         if(alarm_flag == 0){
            if(prev_duty == 0 && saved_duty > 0){

                USART_write(USART6, (uint8_t*)"[AUTO][LED_DUTY = ON]\r\n", strlen("[AUTO][LED_DUTY = ON]\r\n"));
            }else if(prev_duty > 0 && saved_duty == 0){

                USART_write(USART6, (uint8_t*)"[AUTO][LED_DUTY = OFF(0%)]\r\n", strlen("[AUTO][LED_DUTY = OFF(0%)]\r\n"));
            }
            
            //추가로 duty가 바뀔때도 상태변화 프린트 해주면 좋음
            //값이 10%초과할때 뜬다. 
            
            else{
                //이부분이 10확실히 100퍼센트일때 하고 싶어석
                if(saved_duty>=prev_duty+0.1f||saved_duty<prev_duty - 0.1f||(saved_duty == 1 && prev_duty < 1)){
                    sprintf(led_msg,"[AUTO] LED_duty = %d%%\r\n",(int)(saved_duty * 100));
                    USART_write(USART6, (uint8_t*)led_msg, strlen(led_msg));
                }
            }
        }
   
    //디버깅용
    //printf("light=%lu  saved duty=%d %%\r\n", light, (int)(saved_duty * 100));
    
    prev_duty = saved_duty;     
    PWM_duty(LED_panel,saved_duty);
    }






//이건 직접확인해봐야함 
void read_dht11(void){
//2초당
    static int err = 0;
    static uint32_t tick_cnt = 0;
    static uint32_t err_cnt = 0;
    static uint32_t total_error = 0;
    tick_cnt ++;
    //5로 지정한 이유 : DHT11_MIN_PERIOD_MS 가 2000이라서
    if(tick_cnt == 5){

        err = DHT11_read();
        if(err == DHT11_OK){
            err_cnt = 0;
            //28 55
            if(dht_flag == 1){
                dht_flag = 0;

                USART_write(USART6, (uint8_t*)"[DHT11 NO ERROR ANYMORE]\r\n", strlen("[DHT11 NO ERROR ANYMORE]\r\n"));
                }
            }else if(err == DHT11_ERR_TOOSOON){
            }
            //print가 아예안되는것. 
            else{
                err_cnt++;
                total_error++;
                if(err_cnt>5 && dht_flag == 0){
                    //만약에 manual이다. 
                    dht_flag = 1 ;
                    
                    if(mode_flag == 1){
                        USART_write(USART6, (uint8_t*)"[MANUAL][DHT11 ERROR]\r\n", strlen("[MANUAL][DHT11 ERROR]\r\n"));
                    }else{
                        //만약에 auto이면
                        //fan on & window 값이 다시 올때까지 해놓기 
                        USART_write(USART6, (uint8_t*)"[AUTO][DHT11 ERROR]\r\n", strlen("[AUTO][DHT11 ERROR]\r\n"));

                      
                    }
                }
                    printf("[DHT11 ERROR COUNT : %lu]\r\n",total_error); //stm 용
                    char err_msg[40];
                    sprintf(err_msg,"[DHT11 ERROR COUNT : %lu]\r\n", total_error);

                    USART_write(USART6, (uint8_t*)err_msg, strlen(err_msg)); //bluetooth용

                 
            }
   
        tick_cnt = 0;
      
        //printf(" Temperature = %d C, Humidity = %d %%\r\n", DHT11_temperature(), DHT11_humidity());
   
       
    }
}

    

void AUTO_dht11(void){
    //오류가 났었어값이 덮어쓰여지지 않게 pwm 애들을 여기다 놔야돼.
    if(dht_flag == 1){
        //vent flag는 usart6 write를 해주기 위해서
        //한번도 열지 않았다고 가정한다면,
        //이거는 falut일때를 보기 위한 코드 
        if(vent_flag == 0 || window_open ==0){
            USART_write(USART6,(uint8_t*)"[AUTO][DHT11 FAULT] FAN RUN, WINDOW OPEN\r\n",strlen("[AUTO][DHT11 FAULT] FAN RUN, WINDOW OPEN\r\n"));
            }
            PWM_duty(ventfan,1);
            PWM_pulsewidth_us(window,2200);
            vent_flag = 1; 
            window_open = 1;
            return;//추가함 아래에서 다시 정상값으로 할 수 도있음.
    }

    if(temp_high < DHT11_temperature() || humid_high < DHT11_humidity()){
            //DHT11_read() 라는데 이거를 어떻게 각각 temp,humid로 어떻게 읽지
            //팬 on 
                if(window_open == 0||vent_flag ==0){
                    char msg[] = "[AUTO][VENTFAN = RUN] [AUTO][WINDOW = OPEN]\r\n";
                    USART_write(USART6, (uint8_t*) msg, strlen(msg));
                }
                window_open= 1;
                PWM_duty(ventfan,1.0);
                vent_flag = 1;

            //창문열림 - 서보모터
                //PW = 500+(2000 * 150)/180;
                PWM_pulsewidth_us(window, 2200);

                
                
                
            } //48 26
            //vent fan이 닫히고 fan은 멈춰야돼. 
            else if(temp_low > DHT11_temperature() && humid_low > DHT11_humidity()){
                //팬 off
                PWM_duty(ventfan,0.0);
                if(window_open == 1||vent_flag == 1){
                    //한번열렸으면 닫혀주기
                
                     PWM_pulsewidth_us(window,1200);
                     
                    char msg[] = "[AUTO][VENTFAN = STOP] [AUTO][WINDOW = CLOSED]\r\n";
                    USART_write(USART6, (uint8_t*) msg, strlen(msg));
                    
                }else{
                    //디폴트 창문닫힘
                    PWM_pulsewidth_us(window, 1200); // 이값이 위랑 같아야된대 그니까 1100으로 해야된다는거지
                }
                
                //만약 한번 열린적이 있다
                window_open = 0;
                vent_flag = 0;
        
            }
}


// //humid 값은  PA_5      // LED (LD2)
//마른땅은 3200젖은땅은 4095
//3500이랑 4000
void auto_soil(void){

    static uint32_t tick_count = 0;
    static uint32_t dry_count = 0;
    //받아올때 얼마로 받아오는지 확인
    static uint32_t water_count = 0;
    if(water>soil_dry){
       cont_dry=1;
     //cont recovery가 한번 방문됐으면 다시 drycount를 리셋
     //계속해서 카운트를 시키고 이게 10번 채워지면 alarm
        if(cont_recovery == 1){
            dry_count = 0;
            cont_recovery = 0;
        }
            dry_count++;
            if(dry_count>=10){
                if(alarm_flag == 0){
                    USART_write(USART6, (uint8_t*)"[ALARM] DRY\r\n", strlen("[ALARM] DRY\r\n"));
                    tick_count = 0; //  이래야 나중에 add water message가 항상 3초마다 print될수 있음.
                    alarm_blink = 0;//처음 시작
                }
                alarm_flag = 1;

            
        }
           
      //문제점; 계속 alarm이되야하는데 그중간값들에서 꺼져버림
        
    
    }else if(water < soil_recovery){
       //flag 해주기 

        cont_recovery =1;
       //회복임계값 위로 5초 연속 유지 
        //경보해제
       if(cont_dry>=1){
            water_count =0;    
            cont_dry = 0; //--> 리셋을 시켜줘야하나?
       }
       
         water_count++;
            if(water_count>=10){  
                if(alarm_flag == 1){
                    USART_write(USART6, (uint8_t*)"[ALARM] CLEAR\r\n", strlen("[ALARM] CLEAR\r\n"));
                    ack_message =0 ;

                }
                alarm_flag = 0;
                //PWM_duty(LED_panel,0); 알람일때는 그냥 알람상태만 알려주는거니까
            
            }
       }else{
            dry_count = 0;
            water_count = 0;
       }
       
       //ack_message = 1;

    //add water이란 알람을 6초마다 해주기 위해서 
    if(alarm_flag == 1){
        alarm();
        tick_count++;

        if(tick_count>= 2 && ack_message == 0){
                USART_write(USART6, (uint8_t*)"[ALARM] ADD WATER\r\n", strlen("[ALARM] ADD WATER\r\n"));
                //printf("tick_count = %lu\r\n",(unsigned long)tick_count);
                tick_count =0;
        }

    }
      
        //알람이 water_count 되자마자 꺼진다 (5초연속이 아니라)
       //printf("water= %lu, water_count = %lu, dry_count = %lu \r\n", (unsigned long)water, (unsigned long)water_count,(unsigned long)dry_count);
                   
}
        

//volatile로 alarm tick으로 바꿈
void alarm(void){
    //led랑 lcd만 바뀜. 팬 창문은 원래모드 규칙유지
   
     //1초마다 깜빡
    alarm_blink++;

    if(alarm_blink>=2){
        PWM_duty(LED_panel,1);
       
        alarm_blink=0;
    }else{
        PWM_duty(LED_panel,0);
     
    }

   	
    //lcd바뀜
}

// void alarm_off(void){
//     //led랑 lcd만 바뀜. 팬 창문은 원래모드 규칙유지
          
//     PWM_duty(LED_panel,0);

//     //lcd바뀜

// }



// USART2 RX interrupt: Read and discard unused PC input

void USART2_IRQHandler(){       // USART2 RX Interrupt 
	if(is_USART2_RXNE()){
		 USART2_read();		// RX from UART2 (PC)
		 
    }
}

// USART1 RX interrupt: Receive commands from Bluetooth
void USART6_IRQHandler(){         	
	if(is_USART_RXNE(USART6)){          
		// Read a character received through Bluetooth
        char BT = USART_read(USART6);				// RX from other MCU UART1	
        if (BT!='\r' && BT!='\n'){
            BT_Data = BT; 
            bt_flag = 1;
        }
       
    
    }
}



void BLUETOOTH(void){
       // Handle newline characters on both terminals
       if(BT_Data == '\r' || BT_Data == '\n'){
            uint8_t nextline[] = "\r\n";

            // Send newline to Bluetooth terminal
            USART_write(USART6,nextline,2);
            
            //STM창에서도 줄바꿔 
            USART2_write(nextline, 2);
       }
       else{

            //줄바꾸기 전까지 그냥 친거 블루투스에 보여줘
            USART_write(USART6,(uint8_t *)&BT_Data,1);
        // 똑같이 STM에서도 보여주고 
            USART2_write((uint8_t *)&BT_Data, 1);
            //auto일때도 한단어씩 할 수 있게
            USART_write(USART6,(uint8_t *)"\r\n",2);
            
       }
 
       if(BT_Data == 'M'||BT_Data == 'm'){
            mode_flag = 1;
            USART_write(USART6, (uint8_t*)"[MODE][MANUAL]\r\n", strlen("[MODE][MANUAL]\r\n"));

       }
       //MANUAL일때 블루투스에서 받아와야할때
       if(mode_flag == 1){
         // LED control: L = OFF, H = ON
            if(BT_Data == '0'||BT_Data == 'L'){
                if(alarm_flag == 0){
                    PWM_duty(LED_panel,0);
                }
                    USART_write(USART6, (uint8_t*)"[MANUAL][LED DUTY = 0%]\r\n", strlen("[MANUAL][LED DUTY = 0%]\r\n"));
                
                manual_duty = 0;
            }else if(BT_Data == 'H'||BT_Data == 'h'){
                if(alarm_flag == 0){
                    PWM_duty(LED_panel,1.0);
                }
                USART_write(USART6, (uint8_t*)"[MANUAL][LED DUTY = 100%]\r\n", strlen("[MANUAL][LED DUTY = 100%]\r\n"));
                
                manual_duty = 1;
                //아쉬운점: 0.7이라고 넣고 싶은데 7로 넣어야 duty 0.7 이나온다
            }else if(BT_Data>='0' && BT_Data<='9'){
                value = (float)(BT_Data - '0');
                float BT_duty = value*0.1;
                if(alarm_flag == 0){
                    PWM_duty(LED_panel,BT_duty);
                }
                char msg[40];
                sprintf(msg,"[MANUAL] LED_duty = %d%%\r\n", (int)(BT_duty * 100));
                USART_write(USART6, (uint8_t*)msg, strlen(msg));
                
                manual_duty = BT_duty;
            }

            //fan 조작 
            if(BT_Data == 'R'||BT_Data == 'r'){
                PWM_duty(ventfan,1);
                 //printf("[MANUEL][VENTFAN = RUN]");
                USART_write(USART6, (uint8_t*)"[MANUAL][VENTFAN = RUN]\r\n", strlen("[MANUAL][VENTFAN = RUN]\r\n"));
                vent_flag = 1;
            }else if(BT_Data =='S'||BT_Data == 's'){
                PWM_duty(ventfan,0);
                 //printf("[MANUEL][VENTFAN = STOP]");
                USART_write(USART6, (uint8_t*)"[MANUAL][VENTFAN = STOP]\r\n", strlen("[MANUAL][VENTFAN = STOP]\r\n"));
                vent_flag = 0;
            }

            //창문조작
            if(BT_Data == 'O'||BT_Data == 'o'){
                PWM_pulsewidth_us(window,2200);
                window_open = 1;
                USART_write(USART6, (uint8_t*)"[MANUAL][WINDOW = OPEN]\r\n", strlen("[MANUAL][WINDOW = OPEN]\r\n"));

            }else if(BT_Data == 'C'||BT_Data == 'c'){
                PWM_pulsewidth_us(window,1200);
                window_open = 0;
                USART_write(USART6, (uint8_t*)"[MANUAL][WINDOW = CLOSED]\r\n", strlen("[MANUAL][WINDOW = CLOSED]\r\n"));

            }

            //상태가 바뀔때마다 노트북으로 메시지 전송(내용 자유)
    
        }
            //경보확인 명령 --> SOIL은 어디서나 되니까 
            //K를 치면 manuel에서 반복메시지가 멈춘다 
        if(BT_Data == 'A'||BT_Data == 'a'){
            //오토일때도 메세지 현재 반영해서 나갈 수 있도록. 
                if(mode_flag == 1){
                    prev_duty = manual_duty;
                    saved_duty = manual_duty;
                    led_count = 6; //최소 ON 시간 이미 채웠으니까
                }
                mode_flag = 0; // mode가 auto로 간다는 뜻
                mode_count = 0;
                //지우기
                USART_write(USART6, (uint8_t*)"[MODE][AUTO] starts in 5 sec\r\n", strlen("[MODE][AUTO] starts in 5 sec\r\n"));

               
        }
        if((BT_Data == 'K'||BT_Data == 'k')&&alarm_flag==1){
            ack_message = 1;
            USART_write(USART6, (uint8_t*)"[STOP THE ALARM MESSAGE]\r\n", strlen("[STOP THE ALARM MESSAGE]\r\n"));
            //LCD_setCrusor(0,1);
            //LCD_print("[STOP THE ALARM MESSAGE]     ");
        }
            
}


void print_LCD(void){
    char lcd_line[40];

    //상태이름
    //mode flag = 1인게 manual이다. 
        LCD_setCursor(0, 0);
        if(alarm_flag == 1){
            if(ack_message == 0){
                LCD_print("ALARM                ");
            }else{
                LCD_print("ALARM ACK                ");
            }
        }else if(mode_flag==0){
                if(mode_count < 10){
            // 5초 대기 중: 남은 시간 표시 (0.5초 단위)
            sprintf(lcd_line, "AUTO WAIT %lu.%lus ", (10 - mode_count)/2, ((10 - mode_count)%2)*5);
            LCD_print(lcd_line);
    }else{
        LCD_print("AUTO            ");
    }

        }else if(mode_flag==1){
            LCD_print("MANUAL                ");
        }
        LCD_setCursor(0, 1);
            if(alarm_flag == 1){
                 if(alarm_blink>=1){
                    LCD_print("                ");
                                          
                }else{
                   LCD_print("ADD WATER            ");
                 }
            }else if(vent_flag == 1){
                //온도, 습도 vent 
                //DHT11_temperature(), DHT11_humidity() --> %d
                //manual duty, saved_duty 
                sprintf(lcd_line,"%dC %d%% VENT ON  ",DHT11_temperature(),DHT11_humidity());   
                LCD_print(lcd_line);
            }else{
                if(mode_flag == 1){
                    sprintf(lcd_line,"%dC %d%% LED:%d%%   ",DHT11_temperature(),DHT11_humidity(),(int)(manual_duty*100)); 
                    LCD_print(lcd_line);  
                }else if(mode_flag == 0){
                    sprintf(lcd_line,"%dC %d%% LED:%d%%   ",DHT11_temperature(),DHT11_humidity(),(int) (saved_duty*100));   
                    LCD_print(lcd_line);
                }

               }
            }
    






void ADC_IRQHandler(void){
    if(is_ADC_JEOC()){      // one JEOC after the whole sequence
        light = JADC_read(1);
        water = JADC_read(2);
        clear_ADC_JEOC();
    }
}

//default 제어 루프 (500ms)
void TIM1_UP_TIM10_IRQHandler(void) {
    //static uint32_t update_tick = 0;
    if (is_UIF(TIM1)) {
        clear_UIF(TIM1);
        //check FSM LED during PAUSE
        tick_flag = 1 ;
        //지우기
      
    }
}

