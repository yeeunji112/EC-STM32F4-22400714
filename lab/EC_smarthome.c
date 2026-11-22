/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : YeeunJi
Created          : 11-21-2026
Language/ver     : C in VS Code

Description      : 스마트홈 관리 시스템 (공부용, 한국어 주석)
                   HOME  : PIR -> 노란불 3초, 어두우면 흰불, 온습도 높으면 환기
                   AWAY  : 5초 외출 카운트다운 후 PIR -> 침입 알람
                   ALARM : 빨간불 0.5초 깜빡임, LCD "INTRUDER" 깜빡임, BT 'K'로 해제
/----------------------------------------------------------------*/

#include "ecSTM32F4v2.h"
#include <string.h>

// ---------------------------------------------------------------
// 핀 정의 (문제에서 주어진 핀, 바꾸면 안 됨)
// ---------------------------------------------------------------
#define FAN_PIN     PB_7        // 환기 팬 : TIM4_CH2 PWM, 주기 1ms
#define SERVO_PIN   PC_7        // 창문 서보 : TIM3_CH2 PWM, 주기 20ms
#define LED_R       PB_13       // RGB LED 빨강 (디지털 출력)
#define LED_G       PB_14       // RGB LED 초록
#define LED_B       PB_15       // RGB LED 파랑
#define BTN_PIN     PA_10       // 버튼 : EXTI10 -> EXTI15_10_IRQHandler
#define PIR_PIN     PB_3        // PIR 센서 : EXTI3 -> EXTI3_IRQHandler
#define DHT_PIN     PA_6        // 온습도 센서 DHT11
PinName_t seqCHn[2] = {PA_0, PC_1};   // JADC 순서 : JDR1 = 빛 센서, JDR2 = 가변저항

// ---------------------------------------------------------------
// 기준값과 시간 상수 (1 tick = 250ms)
// ---------------------------------------------------------------
#define TEMP_HIGH   29          // 이 온도를 넘으면 환기 시작
#define TEMP_LOW    27          // 이 온도 아래로 내려가야 환기 정지
#define HUMID_HIGH  60          // 이 습도를 넘으면 환기 시작
#define HUMID_LOW   55          // 이 습도 아래로 내려가야 환기 정지
#define LIGHT_HYS   100         // 밝기 판단 히스테리시스 (가변저항 값 기준 ±100)

#define ENTRY_TICKS     12      // 현관 노란불 3초  = 12 x 250ms
#define COUNT_TICKS     20      // 외출 카운트다운 5초 = 20 x 250ms
#define DHT_TICKS       8       // DHT11 2초마다 1번 = 8 x 250ms

#define SERVO_CLOSE     1200    // 창문 닫힘 펄스폭 [us]
#define SERVO_OPEN      2200    // 창문 열림 펄스폭 [us]

// ---------------------------------------------------------------
// 상태 정의
// ---------------------------------------------------------------
#define HOME        0           // 집에 있음
#define AWAY_COUNT  1           // 외출 카운트다운 (5초)
#define AWAY        2           // 외출 중, 침입 감시
#define ALARM       3           // 침입 알람
const char *modeName[4] = {"HOME", "AWAY", "AWAY", "ALARM"};   // LCD에 표시할 이름

// ---------------------------------------------------------------
// ISR에서 바꾸는 변수 (반드시 volatile)
// ISR은 flag만 세우고, 실제 처리는 main에서 함
// ---------------------------------------------------------------
volatile uint32_t tick_flag = 0;    // TIM1이 250ms마다 1로 만듦
volatile uint32_t btn_flag  = 0;    // 버튼이 눌리면 1
volatile uint32_t pir_flag  = 0;    // 움직임이 감지되면 1
volatile uint32_t bt_flag   = 0;    // 블루투스로 글자가 들어오면 1
volatile char     bt_data   = 0;    // 들어온 글자
volatile uint32_t light = 0, pot = 0;   // JADC 결과 (빛, 가변저항)

// ---------------------------------------------------------------
// main에서만 쓰는 변수
// ---------------------------------------------------------------
uint32_t mode       = HOME;     // 현재 상태
uint32_t prev_mode  = AWAY;     // ALARM이 해제되면 돌아갈 상태
uint32_t count      = 0;        // 외출 카운트다운 tick 수
uint32_t entry_cnt  = 0;        // 현관 노란불 남은 tick 수 (0이면 꺼짐)
uint32_t dark       = 0;        // 1 = 거실이 어두움
uint32_t vent_on    = 0;        // 1 = 팬 켜짐 + 창문 열림
uint32_t blink_cnt  = 0;        // 깜빡임용 tick 카운터
int temp = 0, humid = 0;        // 마지막으로 읽은 정상 온도, 습도
uint32_t dht_err    = 0;        // DHT11 연속 오류 횟수


// 블루투스로 문자열 보내기
void BT_send(const char *msg){
    USART_write(USART6, (uint8_t *)msg, strlen(msg));
}

// RGB LED 색 정하기
// Common CATHODE 기준 : HIGH = 켜짐
// Common ANODE라면 GPIO_write(LED_R, !r) 처럼 세 줄 모두 반대로 바꾸면 됨
void rgb(int r, int g, int b){
    GPIO_write(LED_R, r);
    GPIO_write(LED_G, g);
    GPIO_write(LED_B, b);
}

// 상태를 바꾸고 블루투스로 알림
void set_mode(uint32_t new_mode){
    char msg[40];
    mode = new_mode;
    sprintf(msg, "[MODE] %s\r\n", (new_mode == AWAY_COUNT) ? "AWAY (5s countdown)" : modeName[new_mode]);
    BT_send(msg);
}

// 환기 켜기/끄기 : 팬과 창문을 같이 움직임
// 상태가 바뀔 때만 실행하므로 BT 메시지도 바뀔 때 한 번만 나감
void vent(uint32_t on){
    if(on == vent_on) return;               // 이미 그 상태면 아무것도 안 함
    vent_on = on;
    PWM_duty(FAN_PIN, on ? 1.0f : 0.0f);                            // 팬 100% 또는 정지
    PWM_pulsewidth_us(SERVO_PIN, on ? SERVO_OPEN : SERVO_CLOSE);    // 창문 열기 또는 닫기
    BT_send(on ? "[VENT] FAN RUN, WINDOW OPEN\r\n" : "[VENT] FAN STOP, WINDOW CLOSED\r\n");
}


// ---------------------------------------------------------------
// 초기화
// ---------------------------------------------------------------
void setup(void){
    RCC_PLL_init();                             // 시스템 클럭 84MHz
    SysTick_init();                             // 1ms SysTick (timer_check, DHT11에 필요)
    USART2_init();                              // 디버그용 (STM 창)
    USART_init(USART6, PA_11, PA_12, 9600);     // 블루투스 (TX = PA_11, RX = PA_12)

    // RGB LED : 디지털 출력 3개, 처음엔 모두 끔
    GPIO_init(LED_R, OUTPUT);
    GPIO_init(LED_G, OUTPUT);
    GPIO_init(LED_B, OUTPUT);
    rgb(0, 0, 0);

    // 팬 : PWM 1ms, 처음엔 정지
    PWM_init(FAN_PIN, 1);
    PWM_duty(FAN_PIN, 0.0f);
    // 창문 서보 : PWM 20ms, 처음엔 닫힘
    PWM_init(SERVO_PIN, 20);
    PWM_pulsewidth_us(SERVO_PIN, SERVO_CLOSE);

    // 버튼 : EXTI_init은 GPIO를 초기화하지 않으므로 GPIO_init 먼저
    GPIO_init(BTN_PIN, INPUT);
    GPIO_pupd(BTN_PIN, 1);          // pull-up : 평소 HIGH, 누르면 LOW
    EXTI_init(BTN_PIN, FALL, 3);    // 누르는 순간(falling edge)에 인터럽트

    // PIR : 움직임이 있으면 HIGH가 됨
    //passive infrared. 사람이 움직이는것을 감지하는 센서 
    GPIO_init(PIR_PIN, INPUT);
    GPIO_pupd(PIR_PIN, 2);          // pull-down : 평소 LOW
    EXTI_init(PIR_PIN, RISE, 3);    // 감지되는 순간(rising edge)에 인터럽트

    // JADC : 빛(JDR1) + 가변저항(JDR2), TIM5 TRGO로 125ms마다 변환
    JADC_init(PA_0);
    JADC_init(PC_1);
    JADC_sequence(seqCHn, 2);
    JADC_trigger_init(TIM5, 125, ADC_RISE);     // JEOC 인터럽트도 여기서 켜짐

    DHT11_init(DHT_PIN);            // 온습도 센서
    I2C1_init();                    // LCD용 I2C (SCL = PB_8, SDA = PB_9)
    LCD_init();

    // 제어 루프 : TIM1 update 인터럽트 250ms마다 (마지막에 켜기)
    TIM_UI_init(TIM1, 250);
}


// ---------------------------------------------------------------
// 1) 인터럽트로 들어온 입력 처리 (버튼, PIR, 블루투스)
// ---------------------------------------------------------------
void handle_events(void){
    // 버튼 : HOME <-> AWAY 전환
    if(btn_flag){
        btn_flag = 0;
        if(mode == HOME){
            count = 0;              // 카운트다운 처음부터
            vent(0);                // 환기는 HOME에서만 하므로 끔
            set_mode(AWAY_COUNT);
        }else if(mode == AWAY_COUNT || mode == AWAY){
            set_mode(HOME);         // 외출 취소 또는 귀가
        }
        // ALARM 중에는 버튼 무시 (해제는 블루투스로만)
    }

    // PIR : HOME이면 현관 조명, AWAY면 침입
    if(pir_flag){
        pir_flag = 0;
        if(mode == HOME){
            entry_cnt = ENTRY_TICKS;            // 노란불 3초 시작
            BT_send("[HOME] MOTION AT ENTRANCE\r\n");
        }else if(mode == AWAY){
            prev_mode = AWAY;                   // 해제 후 돌아갈 상태 저장
            set_mode(ALARM);
            BT_send("[ALARM] INTRUDER DETECTED! send 'K' to clear\r\n");
        }
        // AWAY_COUNT : 사용자가 나가는 중이므로 움직임 무시
    }

    // 블루투스 'K' : 알람 해제 -> 이전 상태로 복귀
    if(bt_flag){
        bt_flag = 0;
        if((bt_data == 'K' || bt_data == 'k') && mode == ALARM){
            BT_send("[ALARM] CLEARED\r\n");
            set_mode(prev_mode);
        }
    }
}


// ---------------------------------------------------------------
// 2) 센서 읽기 (매 tick)
// ---------------------------------------------------------------
void read_sensors(void){
    static uint32_t dht_tick = 0;
    char msg[40];

    // 밝기 판단 : 고정값 대신 가변저항 값과 비교 (값이 클수록 어두움)
    // 히스테리시스로 경계에서 깜빡이지 않게 함
    if(dark == 0 && light > pot + LIGHT_HYS){
        dark = 1;
        BT_send("[LIGHT] DARK\r\n");
    }
    else if(dark == 1 && light + LIGHT_HYS < pot){     // unsigned 뺄셈을 피하려고 덧셈으로 씀
        dark = 0;
        BT_send("[LIGHT] BRIGHT\r\n");
    }

    // DHT11은 2초에 한 번만 읽음 (8 tick)
    if(++dht_tick >= DHT_TICKS){
        dht_tick = 0;
        if(DHT11_read() == DHT11_OK){
            int t = DHT11_temperature(), h = DHT11_humidity();
            // DHT11 측정 범위 안의 값만 정상으로 인정
            if(t >= 0 && t <= 50 && h >= 20 && h <= 95){
                temp = t;
                humid = h;
                dht_err = 0;
                return;
            }
        }
        // 읽기 실패 또는 범위 밖 값 : 블루투스로 보고, 이전 값 유지
        dht_err++;
        sprintf(msg, "[DHT11] INVALID READING (%lu)\r\n", dht_err);
        BT_send(msg);
    }
}


// ---------------------------------------------------------------
// 3) 환기 : HOME에서만, HIGH / LOW 히스테리시스
// ---------------------------------------------------------------
void control_vent(void){
    if(mode != HOME){               // AWAY, ALARM에서는 무조건 끔
        vent(0);
        return;
    }
    if(temp > TEMP_HIGH || humid > HUMID_HIGH)       vent(1);   // 하나라도 넘으면 켬
    else if(temp < TEMP_LOW && humid < HUMID_LOW)    vent(0);   // 둘 다 내려가야 끔
    // 그 사이 값 : 현재 상태 유지
}


// ---------------------------------------------------------------
// 4) RGB LED : if / else if 순서가 우선순위
//    ALARM 빨강 > 카운트다운 노랑 > 현관 노랑 > 환기 초록 > 어두움 흰색 > 끔
//    이유 : 안전(침입) > 사용자 행동(외출, 방문) > 상태 표시(환기) > 일상 조명
// ---------------------------------------------------------------
void control_led(void){
    blink_cnt++;

    if(mode == ALARM){
        rgb(blink_cnt % 2, 0, 0);                       // 빨강, 250ms마다 토글 = 0.5s 주기
    }
    else if(mode == AWAY_COUNT){
        rgb(count < ENTRY_TICKS, count < ENTRY_TICKS, 0);   // 카운트다운 처음 3초 노랑
    }
    else if(mode == AWAY){
        rgb(0, 0, 0);                                   // 외출 중 조명 꺼짐
    }
    else if(entry_cnt > 0){
        entry_cnt--;                                    // 남은 시간 줄이기
        rgb(1, 1, 0);                                   // 노랑 3초
    }
    else if(vent_on){
        rgb(0, (blink_cnt / 2) % 2, 0);                 // 초록, 500ms마다 토글 = 1s 주기
    }
    else if(dark){
        rgb(1, 1, 1);                                   // 흰색
    }
    else{
        rgb(0, 0, 0);                                   // 끔
    }
}


// ---------------------------------------------------------------
// 5) 외출 카운트다운 (5초)
// ---------------------------------------------------------------
void control_countdown(void){
    if(mode != AWAY_COUNT) return;
    if(++count >= COUNT_TICKS){     // 20 tick = 5초 지남
        set_mode(AWAY);             // 이제부터 침입 감시
        pir_flag = 0;               // 카운트다운 중 들어온 움직임은 버림
    }
}


// ---------------------------------------------------------------
// 6) LCD (16 x 2)
//    1줄 : 상태 이름 / 2줄 : 상태별 정보
// ---------------------------------------------------------------
void print_LCD(void){
    char line[20];

    // 1줄 : HOME, AWAY, ALARM
    LCD_setCursor(0, 0);
    sprintf(line, "%-16s", modeName[mode]);     // 16칸을 채워서 이전 글자 지우기
    LCD_print(line);

    // 2줄
    LCD_setCursor(0, 1);
    if(mode == HOME && vent_on)       sprintf(line, "%2dC %2d%% VENT    ", temp, humid);
    else if(mode == HOME)             sprintf(line, "%2dC %2d%%         ", temp, humid);
    else if(mode == AWAY_COUNT){
        uint32_t left = COUNT_TICKS - count;               // 남은 tick (4 tick = 1초)
        sprintf(line, "COUNTDOWN %lus    ", (left + 3) / 4);   // 5,4,3,2,1 표시
    }
    else if(mode == AWAY)             sprintf(line, "MONITORING      ");
    else                              sprintf(line, "%-16s", ((blink_cnt / 4) % 2) ? "" : "INTRUDER");   // 1초 간격 깜빡임
    LCD_print(line);
}


// ---------------------------------------------------------------
// main : 250ms마다 한 번씩 전체 처리
// ---------------------------------------------------------------
int main(void){
    setup();
    BT_send("[INIT] MODE = HOME\r\n");

    while(1){
        if(tick_flag){
            tick_flag = 0;
            handle_events();        // 1) 버튼, PIR, 블루투스
            read_sensors();         // 2) 빛, DHT11
            control_countdown();    // 5) 외출 카운트다운
            control_vent();         // 3) 환기
            control_led();          // 4) RGB LED
            print_LCD();            // 6) LCD
        }
    }
}


// ===============================================================
// 인터럽트 서비스 루틴 (ISR)
// flag 세우기, 값 복사만 함 (delay, printf, LCD, 모터 제어 금지)
// ===============================================================

// TIM1 update : 250ms마다 제어 루프 실행 신호
void TIM1_UP_TIM10_IRQHandler(void){
    if(is_UIF(TIM1)){
        tick_flag = 1;
        clear_UIF(TIM1);
    }
}

// 버튼 PA_10 : 핀 10~15는 EXTI15_10_IRQHandler
void EXTI15_10_IRQHandler(void){
    static uint32_t last = 0;
    if(is_pending_EXTI(BTN_PIN)){
        if(timer_check(&last, 200)) btn_flag = 1;   // 200ms 안에 다시 오면 무시 (디바운스)
        clear_pending_EXTI(BTN_PIN);
    }
}

// PIR PB_3 : 핀 0~4는 번호별 ISR (EXTI3_IRQHandler)
void EXTI3_IRQHandler(void){
    if(is_pending_EXTI(PIR_PIN)){
        pir_flag = 1;
        clear_pending_EXTI(PIR_PIN);
    }
}

// JADC : 125ms마다 두 채널 변환이 끝나면 한 번 들어옴
void ADC_IRQHandler(void){
    if(is_ADC_JEOC()){
        light = JADC_read(1);       // JDR1 : 빛 센서
        pot   = JADC_read(2);       // JDR2 : 가변저항
        clear_ADC_JEOC();           // JEOC는 직접 지워야 함
    }
}

// 블루투스 USART6 : 한 글자 받기 (엔터, 줄바꿈은 무시)
void USART6_IRQHandler(void){
    if(is_USART_RXNE(USART6)){
        char c = USART_read(USART6);
        if(c != '\r' && c != '\n'){
            bt_data = c;
            bt_flag = 1;
        }
    }
}

// 디버그 USART2 : 들어온 글자는 읽고 버림
void USART2_IRQHandler(void){
    if(is_USART2_RXNE()) USART2_read();
}
