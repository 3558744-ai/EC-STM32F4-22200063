/*
 * 1. 밝기 관련 코드 - AUTO 조명 단독 시험용
 * 원본: TU_DHT11_AUTO.c의 밝기 제어 부분을 분리.
 *
 * 연결: 조도 AO -> PA0(A0), LED 패널 PWM 입력 -> PB7,
 *       시작 버튼 -> 보드 B1 USER(PC13), 장치 GND -> 공통 GND.
 * 흐름: 버튼 누름 -> 5초 대기 -> 조도가 낮을수록 LED 밝게.
 *       밝기 경계 히스테리시스와 최소 ON 3초 적용.
 *
 * 사용: 이 파일 하나만 기존 EC 프로젝트의 src/main.c로 사용한다.
 * 다른 main.c 또는 02_AUTO_Temperature_Humidity.c와 동시에 빌드하지 않는다.
 * 완성된 EC 드라이버가 필요하며 _student 빈칸 파일로는 동작하지 않는다.
 * 센서 기준값은 실측 전 예시다. 패널 빛이 조도 센서에 직접 닿지 않게 한다.
 * 범위: 조도/LED/버튼/USB 출력. 토양 경보·LCD·BT·온습도는 포함하지 않는다.
 */
#include "ecSTM32F4v2.h"
#include "ecPWM2.h"
#include "ecTIM2.h"
#include <stdint.h>
#include <stdio.h>

#include "ecADC2.h"
#include "ecEXTI2.h"

/* 핀 및 시간: TIM5=센서 125ms, TIM1=제어 500ms. */
#define CDS_PIN              PA_0
#define PANEL_PIN            PB_7       // TIM4 CH2, 1 kHz
#define START_BUTTON         PC_13      // 선택: Nucleo 사용자 버튼, 누르면 LOW
#define CONTROL_MS           500U
#define LIGHT_WAIT_MS        5000U
#define BUTTON_DEBOUNCE_MS   50U
#define ADC_STALE_MS         500U

/* 예시 보정값: 밝을수록 raw가 감소하면 HIGH_IS_BRIGHT를 0으로 바꾼다.
 * ADC값은 lux가 아니다. 실제 어둠/밝음에서 측정해 기준을 정한다.
 */
#define LIGHT_RAW_HIGH_IS_BRIGHT  1
#define LIGHT_DARK           1000U
#define LIGHT_BRIGHT         3000U
#define LIGHT_HYST           100U
#define LIGHT_MIN_ON_MS      3000U

/* ISR이 갱신하는 값은 volatile. 조도 한 채널만 변환한다. */
typedef struct {
    uint32_t light, sampled_at;
    uint8_t valid;
} LightSample;
static volatile LightSample adc_sample;
static volatile uint8_t control_due, button_event;
static volatile uint32_t button_at;
static PinName_t seqCHn[1] = {CDS_PIN};

typedef enum { LIGHT_MIDDLE, LIGHT_DARK_ZONE, LIGHT_BRIGHT_ZONE } LightZone;
static LightZone light_zone = LIGHT_MIDDLE;
static uint8_t light_started, light_ready;
static uint32_t light_start_at, light_on_at;
static float normal_duty;                     // 0.0=꺼짐, 1.0=최대 밝기

/* unsigned 시간 차이로 msTicks가 한 바퀴 돌아도 경과시간을 계산한다. */
static uint8_t elapsed(uint32_t now, uint32_t since, uint32_t period)
{
    return (uint32_t)(now - since) >= period;
}

/* 제어값이 클수록 밝도록 센서 방향을 맞춘다. */
static uint32_t light_value(uint32_t raw)
{
    return LIGHT_RAW_HIGH_IS_BRIGHT ? raw : 4095U - raw;
}

/* 여러 값을 같은 샘플로 복사할 때만 잠깐 인터럽트를 막는다. */
static LightSample read_light(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    LightSample sample = adc_sample;
    __set_PRIMASK(mask);
    return sample;
}

/* 히스테리시스: DARK는 1000 이하 진입/1100 이상 이탈,
 * BRIGHT는 3000 이상 진입/2900 이하 이탈. 중간은 비례식으로 계산.
 */
static float requested_light_duty(uint32_t value)
{
    if (light_zone == LIGHT_DARK_ZONE && value >= LIGHT_DARK + LIGHT_HYST)
        light_zone = LIGHT_MIDDLE;
    if (light_zone == LIGHT_BRIGHT_ZONE && value <= LIGHT_BRIGHT - LIGHT_HYST)
        light_zone = LIGHT_MIDDLE;
    if (light_zone == LIGHT_MIDDLE) {
        if (value <= LIGHT_DARK) light_zone = LIGHT_DARK_ZONE;
        else if (value >= LIGHT_BRIGHT) light_zone = LIGHT_BRIGHT_ZONE;
    }
    if (light_zone == LIGHT_DARK_ZONE) return 1.0f;
    if (light_zone == LIGHT_BRIGHT_ZONE) return 0.0f;

    // 예: value=2000이면 (3000-2000)/(3000-1000)=0.5, 즉 50%.
    return (float)(LIGHT_BRIGHT - value) / (float)(LIGHT_BRIGHT - LIGHT_DARK);
}

/* 버튼 후 5초 대기 및 켜진 뒤 최소 3초 유지. ADC 오류면 이전 출력 유지. */
static void update_light(uint32_t now, uint32_t light, uint8_t analog_valid)
{
    if (light_started && elapsed(now, light_start_at, LIGHT_WAIT_MS))
        light_ready = 1;
    if (!light_ready || !analog_valid) return;  // 대기/ADC 오류 중 이전값 유지

    float target = requested_light_duty(light);
    if (normal_duty == 0.0f && target > 0.0f) light_on_at = now;
    if (target == 0.0f && normal_duty > 0.0f &&
        !elapsed(now, light_on_at, LIGHT_MIN_ON_MS))
        return;                              // 최소 ON 시간 전에는 끄지 않음
    normal_duty = target;
}

/* 초기화: 센서 한 채널과 LED PWM, 버튼, 제어 타이머만 준비한다. */
void setup(void)
{
    RCC_PLL_init();                           // 시스템 클록 84MHz
    SysTick_init();                           // 1ms 시각 msTicks
    USART2_init();                            // USB 시리얼 printf
    USART2->CR1 &= ~USART_CR1_RXNEIE;         // 수신 IRQ는 사용하지 않음

    PWM_init(PANEL_PIN, 1);                   // 1ms 주기, 즉 1kHz
    PWM_duty(PANEL_PIN, 0.0f);                // 초기 목표 LED OFF
    // PWM_init이 기본 50%를 먼저 출력하는 driver이면 초기 순간 파형 확인 필요.

    JADC_init(CDS_PIN);
    JADC_sequence(seqCHn, 1);                 // JDR1=조도, 수분 채널 제외
    JADC_trigger_init(TIM5, 125, ADC_RISE);    // 125ms마다 자동 변환

    GPIO_init(START_BUTTON, INPUT);
    GPIO_pupd(START_BUTTON, 1);               // pull-up, 누르면 LOW
    EXTI_init(START_BUTTON, FALL, 2);
    clear_pending_EXTI(START_BUTTON);

    TIM_UI_init(TIM1, CONTROL_MS);            // 500ms마다 제어 요청
    clear_UIF(TIM1);
    control_due = 1;
}

int main(void)
{
    setup();
    uint32_t tPrint = msTicks;
    while (1) {
        // 요청 복사와 초기화를 함께 수행해 ISR 이벤트가 중간에 지워지지 않게 함.
        uint32_t mask = __get_PRIMASK();
        __disable_irq();
        uint8_t tick = control_due, press = button_event;
        uint32_t pressed_at = button_at;
        control_due = button_event = 0;
        __set_PRIMASK(mask);

        if (press && !light_started) {        // 첫 누름만 사용, 재입력은 무시
            light_started = 1;
            light_start_at = pressed_at;
        }
        if (tick || press) {
            LightSample sample = read_light();
            uint32_t now = msTicks;
            uint8_t valid = sample.valid && !elapsed(now, sample.sampled_at, ADC_STALE_MS);
            update_light(now, light_value(sample.light), valid);
            PWM_duty(PANEL_PIN, normal_duty); // 50%라면 0.5f를 전달
        }
        if (elapsed(msTicks, tPrint, 1000U)) {
            tPrint = msTicks;
            LightSample sample = read_light();
            uint8_t valid = sample.valid && !elapsed(msTicks, sample.sampled_at, ADC_STALE_MS);
            printf("LIGHT raw=%lu LED=%u%% ADC=%u START=%u READY=%u\r\n",
                   (unsigned long)sample.light, (unsigned)(normal_duty * 100.0f + 0.5f),
                   (unsigned)valid, (unsigned)light_started, (unsigned)light_ready);
        }
    }
}

/* ADC 변환 완료 ISR: 값/시각만 기록, 밝기 판단은 main에서 수행. */
void ADC_IRQHandler(void)
{
    if (is_ADC_JEOC()) {
        adc_sample.light = JADC_read(1);
        adc_sample.sampled_at = msTicks;
        adc_sample.valid = 1;
        clear_ADC_JEOC();
    }
}

/* 제어 타이머 ISR: main이 처리할 요청만 남긴다. */
void TIM1_UP_TIM10_IRQHandler(void)            // F411의 실제 TIM1 update vector
{
    if (is_UIF(TIM1)) {
        clear_UIF(TIM1);
        control_due = 1;                     // 밀린 횟수만큼 오래된 제어를 재실행하지 않음
    }
}

/* 버튼 ISR: 50ms 내 바운스를 걸러내고 누른 시각을 저장한다. */
void EXTI15_10_IRQHandler(void)
{
    static uint8_t seen;
    static uint32_t last_press;
    if (is_pending_EXTI(START_BUTTON)) {
        clear_pending_EXTI(START_BUTTON);
        uint32_t now = msTicks;
        if (!seen || elapsed(now, last_press, BUTTON_DEBOUNCE_MS)) {
            seen = 1;
            last_press = now;
            if (!button_event) { button_at = now; button_event = 1; }
        }
    }
}
