

/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : YeeunJi
Student ID       : 22400714
Created          : 19-11-2026
Language/ver     : C in VS Code

Description      : EC PROJECT 1 - SMARTFARM                               
/----------------------------------------------------------------*/



#include "ecSTM32F4v2.h"
#include <string.h>




#define ventfan           PA_15 //pwm
#define window             PC_7 //window servo motor(PWM)
#define LED_panel          PB_7 //Gro=light LED panel(PWM)
#define LED_PIN    PA_5 //LD2
#define button_pin         PC_13 //User button (B1)
#define DHT11_PIN          PA_6 //On-board LED(LD2)

#define photo_block         3600 // WHEN SENSOR IS COVERED(dark)
#define photo_unblock        1300 // when sensor is not covered (bright)
#define temp_high          29//upper temperature limit
#define temp_low           27 // lower temperature limi
#define soil_recovery      2100  //below this, soil is considered wet
#define soil_dry           3000//upper this, soil is dry
#define humid_high          47 //upper humid
#define humid_low          44//lower humid




// GLOABL VAIRABLES
static volatile char BT_Data = 0; //last character recived over bluetooth
volatile float saved_duty = 0;  //computed in AUTO mode
volatile float prev_duty = 0;  //LED duty from the previous control cycle


volatile uint32_t  bt_flag = 0;  // set by USART6 ISR when a new character arrives 

volatile uint32_t light = 0;  //latest photo sensor ADC vlaue
volatile uint32_t water = 0;  //Latest soil moisture ADC value
 volatile uint32_t tick_flag = 0; //set every 500 ms by TIM1 ISR
PinName_t seqCHn[2] = {PA_0, PA_1}; // photo sensor -> pa_0, soil sensor-> pa_1

volatile uint32_t window_open = 0 ; // 1 = window is opened, 0 = closed


volatile uint32_t cont_dry =0;  //1 = soil was dry in the latest reading
volatile uint32_t cont_recovery = 0; //1 = soil was in the wet range 
volatile uint32_t alarm_flag = 0; //1 = soild dry alarm is active
volatile uint32_t mode_flag= 1;      //to start with manual, 0 is AUTO
volatile uint32_t ack_message =0 ; // for ('k') to know the alarm and stop message
volatile uint32_t  mode_count =0;  //counting from MANUAL to AUTO (5 SEC)
volatile  float value = 0 ;    //value in BLuetooth digit
volatile uint32_t hystery = 100; //for LED, photo sensor thresholds

volatile uint32_t dht_flag = 0;   //checking if it is a fault state
volatile float manual_duty  = 0;  //led duty set by manual mode
char lcd_line[20];              
volatile uint32_t alarm_blink = 0;   //blink for the alarm (used in led, lcd)

volatile uint32_t vent_flag = 0;      //vent is running or not
        
volatile uint32_t led_count = 0;       //for minimum led on time
 


//led in AUTO
void AUTO_led(void);

//only read every 2.5sec
void read_dht11(void);
//dht11 when auto
void AUTO_dht11(void);

//relating to moisture 
void  auto_soil(void);

//ALARM
void alarm(void);

//bluetooth command
void BLUETOOTH();

//relating to LCD 
void print_LCD(void);

void setup(){

    //system clock and systick
    RCC_PLL_init();
    SysTick_init();
    
    USART2_init(); //only just for debugging
    
    //timer - main control loop for 500ms
    TIM_UI_init(TIM1, 500);           

    //ventfan (1ms) , closed
    PWM_init(ventfan,1);
    PWM_duty(ventfan, 0.0f);

    //windo servo motor(20ms)
    PWM_init(window,20);
    PWM_pulsewidth_us(window,1200); //poisition-controlled, needs initial angle
    
    //led penal
    PWM_init(LED_panel,1); //led panel pwm (period 1 ms), initially off
    PWM_duty(LED_panel, 0.0f); 

    
    // injected ADC init
    JADC_init(PA_0);
    JADC_init(PA_1);

    // JADC channel sequence setting
    JADC_sequence(seqCHn, 2);

    // TIM5 TRGO @ 125msec - enables JEOC interrupt
    JADC_trigger_init(TIM5, 125, ADC_RISE);

    
    USART_init(USART6,PA_11,PA_12,9600); //bluetooth module(9600 baud)
    
    
    //temperature, humidity
    DHT11_init(DHT11_PIN);	// DATA = PB_12


    //LCD init(I2C)
    I2C1_init();			// SCL = PB_8,  SDA = PB_9
    LCD_init();

}


#define AUTO    0
#define MANUAL  1



int main(void){
        setup();

    
    //Default state at power-on : LED off, fan off ,window closed, manual mode
	printf("MCU Initialized\r\n");	 //display on stm terminal

    uint8_t BTin[]="MCU Initalization\r\n";

    //send the initial state to the Bluetooth terminal
    USART_write(USART6, (uint8_t*)"[INITIALLIZATION][MODE = MANUAL]\r\n", strlen("[INITIALLIZATION][MODE = MANUAL]\r\n"));
    USART_write(USART6, (uint8_t*)"[LED = OFF, VENTFAN = STOP, WINDOW = CLOSED]\r\n", strlen("[LED = OFF, VENTFAN = STOP, WINDOW = CLOSED]\r\n"));

    while(1){
        
        //handle a new bluetooth command(flag set in USART6 ISR)
        if(bt_flag == 1){
            bt_flag = 0;
            BLUETOOTH();
        }

        //main control loop: runs every 500 ms
        if(tick_flag == 1){
            tick_flag = 0;
            //mode itself is changed inside the bluetooth handler
            read_dht11();   
         
            //IF AUTO  
            if(mode_flag == 0){
                    //AFTER 5 sec, auto
                    if(mode_count<10){
                        mode_count++;
                    }else{
                       
                        AUTO_led();    
                        //led panel from light level
                        AUTO_dht11(); //vent(fan+window)
                  
                    }

             }else{
 
                //manual mode: re-aply the user's led duty every cycle
                //so the led returns to the manual setting after a soil alarm clears
                PWM_duty(LED_panel, manual_duty);
             }
        
            auto_soil(); //soil checks run in both modes 
            print_LCD();
            printf("[%s] light=%lu\r\n",
       (mode_flag == 0) ? "AUTO" : "MANUAL",
       light);
            
        }
    }
}




// auto mode led control
void AUTO_led(void){
    static uint32_t led_first = 0;    //hysteresis for the lower threshold
    static float final_duty = 0;
    static uint32_t led_second = 0;   // hysterisis for the higher threshold
    char led_msg[40];
        
        //FIRST BOUNDARY 
        if(led_first == 0){
            if(light>photo_unblock + hystery){
                led_first = 1; //led duty calculation
            }
        }else{
                if(light<photo_unblock - hystery){
                    led_first = 0; //led is off
                }
        }
        
        //full brightness
        if(led_second == 0){
            if(light>photo_block+hystery){
                led_second = 1; //fully LED ON
            }
        }else{
            if(light<photo_block - hystery){
                led_second = 0; //BACK TO DUTY 
            }
        }

        if(led_first == 0){
            saved_duty = 0;
        }else if(led_second == 1){
            saved_duty = 1;
        }else{
            //led_on == 1 and led_second == 0
            saved_duty = ((float)light-photo_unblock)/(photo_block - photo_unblock);
         
            if(saved_duty<0.05f){
                //If we clapmed to 0 here, the LED would turn off inside the hystersis band,
                //which defeats the purpose of hystersis. So keep a minimum duty of 5%              
                saved_duty = 0.05f;
            }else if(saved_duty > 1){
                saved_duty = 1;
            }
        }

         //Miniimum LED ON TIME: 3sec 
         //Count how long the LED has been on
         if(prev_duty > 0 && led_count < 6){
            led_count++;
         }
            //Reset the counter as soon as the LED turns on
            if(prev_duty == 0 && saved_duty > 0){
            led_count = 0;
            }else if(prev_duty > 0 && saved_duty == 0){
                //trying to turn off before 3 sec
                if(led_count<6){
                    saved_duty = prev_duty;
                }
        }
        

         //bluetooth status message
         //surpressed while the alarm is active
         if(alarm_flag == 0){
            if(prev_duty == 0 && saved_duty > 0){

                USART_write(USART6, (uint8_t*)"[AUTO][LED_DUTY = ON]\r\n", strlen("[AUTO][LED_DUTY = ON]\r\n"));
            }else if(prev_duty > 0 && saved_duty == 0){

                USART_write(USART6, (uint8_t*)"[AUTO][LED_DUTY = OFF(0%)]\r\n", strlen("[AUTO][LED_DUTY = OFF(0%)]\r\n"));
            }
            
            //also report duty changes:
            //print when the duty changes by 10% or more
            //and always print when it reaches exactly 100%
            
            else{
                //이부분이 10확실히 100퍼센트일때 하고 싶어석
                if(saved_duty>=prev_duty+0.1f||saved_duty<prev_duty - 0.1f||(saved_duty == 1 && prev_duty < 1)){
                    sprintf(led_msg,"[AUTO] LED_duty = %d%%\r\n",(int)(saved_duty * 100));
                    USART_write(USART6, (uint8_t*)led_msg, strlen(led_msg));
                }
            }
        }
   
    //just for debugging
    //printf("light=%lu  saved duty=%d %%\r\n", light, (int)(saved_duty * 100));
    
    prev_duty = saved_duty;     
    PWM_duty(LED_panel,saved_duty);
    }






//Read the DHT11 sensor
//called every 500 ms but actaully read as 2.5 ms
void read_dht11(void){
//2초당
    static int err = 0;
    static uint32_t tick_cnt = 0;
    static uint32_t err_cnt = 0;         //error count
    static uint32_t total_error = 0;     //total error count since power-on
    tick_cnt ++;
    //5 ticks are used because DHT11_MIN_PERIOD_MS is 2000ms
    if(tick_cnt == 5){

        err = DHT11_read();
        if(err == DHT11_OK){
            err_cnt = 0;
            //sensor recovered from a fault
            if(dht_flag == 1){
                dht_flag = 0;

                USART_write(USART6, (uint8_t*)"[DHT11 NO ERROR ANYMORE]\r\n", strlen("[DHT11 NO ERROR ANYMORE]\r\n"));
                }
            }else if(err == DHT11_ERR_TOOSOON){
            }
            //read request too early
            else{
                err_cnt++;
                total_error++;
                if(err_cnt>5 && dht_flag == 0){
                    //6 consecutive error-> declare a DHT11 fault 
                    dht_flag = 1 ;
                    
                    if(mode_flag == 1){
                        //manual mode
                        USART_write(USART6, (uint8_t*)"[MANUAL][DHT11 ERROR]\r\n", strlen("[MANUAL][DHT11 ERROR]\r\n"));
                    }else{
                        //auto mode
                        //fan on & window until valid data comes back
                        USART_write(USART6, (uint8_t*)"[AUTO][DHT11 ERROR]\r\n", strlen("[AUTO][DHT11 ERROR]\r\n"));

                      
                    }
                }
                    printf("[DHT11 ERROR COUNT : %lu]\r\n",total_error); //for stm
                    char err_msg[40];
                    sprintf(err_msg,"[DHT11 ERROR COUNT : %lu]\r\n", total_error);

                    USART_write(USART6, (uint8_t*)err_msg, strlen(err_msg)); //for bluetooth

                 
            }
   
        tick_cnt = 0;
      
        //printf(" Temperature = %d C, Humidity = %d %%\r\n", DHT11_temperature(), DHT11_humidity());
   
       
    }
}

    
//auto --> dht11
void AUTO_dht11(void){
    //fault handling: the pwm outputs are set here so they are not overwritten below
    if(dht_flag == 1){
        //vent flag/ window_open are used so the USART6 message is sent only once
        //when the fan or window was not already on/open 
        if(vent_flag == 0 || window_open ==0){
            USART_write(USART6,(uint8_t*)"[AUTO][DHT11 FAULT] FAN RUN, WINDOW OPEN\r\n",strlen("[AUTO][DHT11 FAULT] FAN RUN, WINDOW OPEN\r\n"));
            }
            //fai-safe: ventilate while sensor data is unavailable
            PWM_duty(ventfan,1);
            PWM_pulsewidth_us(window,2200);
            vent_flag = 1; 
            window_open = 1;
            return;//this should be addedso the stale sensor value below cannot change the output
    }

    if(temp_high < DHT11_temperature() || humid_high < DHT11_humidity()){
            //fan on
                if(window_open == 0||vent_flag ==0){
                    char msg[] = "[AUTO][VENTFAN = RUN] [AUTO][WINDOW = OPEN]\r\n";
                    USART_write(USART6, (uint8_t*) msg, strlen(msg));
                }
                window_open= 1;
                PWM_duty(ventfan,1.0);
                vent_flag = 1;

            //open the windeow - servo motor
               
                PWM_pulsewidth_us(window, 2200);

                
                
                
            } 
            //close the window and stop the fan
            else if(temp_low > DHT11_temperature() && humid_low > DHT11_humidity()){
                //fan off
                PWM_duty(ventfan,0.0);
                if(window_open == 1||vent_flag == 1){
                    //if it is opened before, close it and report the change
                
                     PWM_pulsewidth_us(window,1200);
                     
                    char msg[] = "[AUTO][VENTFAN = STOP] [AUTO][WINDOW = CLOSED]\r\n";
                    USART_write(USART6, (uint8_t*) msg, strlen(msg));
                    
                }else{
                    //default: window closed
                    PWM_pulsewidth_us(window, 1200); 
                }
                
                //clear the open/running state
                window_open = 0;
                vent_flag = 0;
        
            }
}


// soil moisture monitoring (runs in both AUTO and MANUAL mode)
void auto_soil(void){

    static uint32_t tick_count = 0; // since the last water message
    static uint32_t dry_count = 0; //consecutive dry ticks
    static uint32_t water_count = 0;  // consecutive recovered ticks
    if(water>soil_dry){
       cont_dry=1;
     //If we passed through the recoverd the state, restart the dry count.
     //keep counting : once it reaches 10 ticks, raise the alarm
        if(cont_recovery == 1){
            dry_count = 0;
            cont_recovery = 0;
        }
            dry_count++;
            if(dry_count>=10){
                if(alarm_flag == 0){
                    USART_write(USART6, (uint8_t*)"[ALARM] DRY\r\n", strlen("[ALARM] DRY\r\n"));
                    tick_count = 0; //  so that "add water" is always printed on a fixed interval
                    alarm_blink = 0;// start of the blink sequence
                }
                alarm_flag = 1;

            
        }
           
    
        
    
    }else if(water < soil_recovery){
       //soil is wet again

        cont_recovery =1;
       //for 5 sec --> clar the alarm
       if(cont_dry>=1){
            water_count =0;    
            cont_dry = 0; 
       }
       
         water_count++;
            if(water_count>=10){  
                if(alarm_flag == 1){
                    USART_write(USART6, (uint8_t*)"[ALARM] CLEAR\r\n", strlen("[ALARM] CLEAR\r\n"));
                    ack_message =0 ;

                }
                alarm_flag = 0;
                // the led is off here.
            }
       }else{
            //reset both counters
            dry_count = 0;
            water_count = 0;
       }
       

    //message "add water" 
    if(alarm_flag == 1){
        alarm();
        tick_count++;
        //repeat every 2 ticks
        if(tick_count>= 2 && ack_message == 0){
                USART_write(USART6, (uint8_t*)"[ALARM] ADD WATER\r\n", strlen("[ALARM] ADD WATER\r\n"));
                tick_count =0;
        }

    }
      
        
       //printf("water= %lu, water_count = %lu, dry_count = %lu \r\n", (unsigned long)water, (unsigned long)water_count,(unsigned long)dry_count);
                   
}
        

//alarm output
//only the led panel and LCD change
void alarm(void){
   
     //toglgle the led panel
    alarm_blink++;

    if(alarm_blink>=2){
        PWM_duty(LED_panel,1);
       
        alarm_blink=0;
    }else{
        PWM_duty(LED_panel,0);
     
    }

   	
    
}





// USART2 RX interrupt: Read and discard unused PC input
void USART2_IRQHandler(){       
	if(is_USART2_RXNE()){
		 USART2_read();		// RX from UART2 (PC)
		 
    }
}

// USART RX interrupt: Receive commands from Bluetooth
void USART6_IRQHandler(){         	
	if(is_USART_RXNE(USART6)){          
		// Read a character received through Bluetooth
        char BT = USART_read(USART6);	
        //only for real command characters			
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
            
            //also send newline to the STM terminal
            USART2_write(nextline, 2);
       }
       else{

            //echo the typed character to the bluetooth terminal
            USART_write(USART6,(uint8_t *)&BT_Data,1);
        // echo it to the stm terminal as well
            USART2_write((uint8_t *)&BT_Data, 1);
            //new line after each character
            USART_write(USART6,(uint8_t *)"\r\n",2);
            
       }
 
       //switch to manual
       if(BT_Data == 'M'||BT_Data == 'm'){
            mode_flag = 1;
            USART_write(USART6, (uint8_t*)"[MODE][MANUAL]\r\n", strlen("[MODE][MANUAL]\r\n"));

       }
       //MANUal - commands
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
                //when 7 is pressed, to make decimal, 0.1 is multiplied 
            }else if(BT_Data>='0' && BT_Data<='9'){
                value = (float)(BT_Data - '0');
                float BT_duty = value*0.1; // to make value float 
                if(alarm_flag == 0){
                    PWM_duty(LED_panel,BT_duty);
                }
                char msg[40];
                sprintf(msg,"[MANUAL] LED_duty = %d%%\r\n", (int)(BT_duty * 100));
                USART_write(USART6, (uint8_t*)msg, strlen(msg));
                
                manual_duty = BT_duty;
            }

            //fan control
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

            //wiindow control
            if(BT_Data == 'O'||BT_Data == 'o'){
                PWM_pulsewidth_us(window,2200);
                window_open = 1;
                USART_write(USART6, (uint8_t*)"[MANUAL][WINDOW = OPEN]\r\n", strlen("[MANUAL][WINDOW = OPEN]\r\n"));

            }else if(BT_Data == 'C'||BT_Data == 'c'){
                PWM_pulsewidth_us(window,1200);
                window_open = 0;
                USART_write(USART6, (uint8_t*)"[MANUAL][WINDOW = CLOSED]\r\n", strlen("[MANUAL][WINDOW = CLOSED]\r\n"));

            }

    
        }
            
            //switch to AUTO mode 
        if(BT_Data == 'A'||BT_Data == 'a'){
                if(mode_flag == 1){
                    prev_duty = manual_duty;
                    saved_duty = manual_duty;
                    led_count = 6; //Minimum on time is treated as already satisfied
                }
                mode_flag = 0; // go to auto mode
                mode_count = 0; //restart 5sec after
                
                USART_write(USART6, (uint8_t*)"[MODE][AUTO] starts in 5 sec\r\n", strlen("[MODE][AUTO] starts in 5 sec\r\n"));

               
        }
        //alarm acknowledge
        // k stops the repeated "add water" message
        if((BT_Data == 'K'||BT_Data == 'k')&&alarm_flag==1){
            ack_message = 1;
            USART_write(USART6, (uint8_t*)"[STOP THE ALARM MESSAGE]\r\n", strlen("[STOP THE ALARM MESSAGE]\r\n"));
         
        }
            
}


//lcd display
void print_LCD(void){
    char lcd_line[40];

   //line 1: current state
        LCD_setCursor(0, 0);
        if(alarm_flag == 1){
            if(ack_message == 0){
                LCD_print("ALARM                ");
            }else{
                LCD_print("ALARM ACK                ");
            }
        }else if(mode_flag==0){
                if(mode_count < 10){
            // to show 5 sec in LCD
            sprintf(lcd_line, "AUTO WAIT %lu.%lus ", (10 - mode_count)/2, ((10 - mode_count)%2)*5);
            LCD_print(lcd_line);
            }else{
                LCD_print("AUTO            ");
            }

        }else if(mode_flag==1){
            LCD_print("MANUAL                ");
        }
        //LINE 2 : DETAILS
        LCD_setCursor(0, 1);
            if(alarm_flag == 1){
                 if(alarm_blink>=1){
                    LCD_print("                ");
                   //  blink "Add water" for one sec                       
                }else{
                   LCD_print("ADD WATER            ");
                 }
            }else if(vent_flag == 1){
                //temperature, humidity  vent 
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
    





//every 125 ms --> ADC interrupt
void ADC_IRQHandler(void){
    if(is_ADC_JEOC()){      // one JEOC after the whole sequence
        light = JADC_read(1); //channel 1 : photo sensor
        water = JADC_read(2); //channel 2: moisture sensor
        clear_ADC_JEOC();
    }
}

//TIM1 update interrupt (500ms)
void TIM1_UP_TIM10_IRQHandler(void) {
    //static uint32_t update_tick = 0;
    if (is_UIF(TIM1)) {
        clear_UIF(TIM1);
        
        tick_flag = 1 ;       //run the control loop in main()
    
      
    }
}

