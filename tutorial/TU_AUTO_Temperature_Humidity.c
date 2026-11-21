/*
 * 2. 온습도 관련 코드 - AUTO 환기 단독 시험용
 * 원본: TU_DHT11_AUTO.c의 DHT11/환기 제어 부분을 분리.
 *
 * 연결: DHT11 DATA -> PA6(D12), 팬 드라이버 PWM -> PA15,
 *       서보 신호 -> PC7(D9), 모든 장치 GND -> 공통 GND.
 * 팬은 드라이버, 서보는 정격 별도 전원을 사용한다.
 * 흐름: 3초마다 DHT 측정 -> 온도 OR 습도 상한 초과 시 팬 ON/창문 OPEN.
 *       온도 AND 습도 모두 하한 미만일 때 팬 OFF/창문 CLOSED.
 *
 * 사용: 이 파일 하나만 기존 EC 프로젝트의 src/main.c로 사용한다.
 * 다른 main.c 또는 01_AUTO_Light.c와 동시에 빌드하지 않는다.
 * 완성된 EC 드라이버 및 제공 ecDHT11 드라이버가 필요하다.
 * 임계값/서보 펄스폭은 실물에 맞춰 조정할 예시값이다.
 * 범위: DHT11/팬/서보/USB 출력. 조명·토양 경보·LCD·BT는 포함하지 않는다.
 */
#include "ecSTM32F4v2.h"
#include "ecPWM2.h"
#include "ecTIM2.h"
#include <stdint.h>
#include <stdio.h>

#include "ecDHT11.h"

/* 과제 지정 핀 및 기존 코드의 측정/제어 간격. */
#define DHT11_PIN            PA_6
#define FAN_PIN              PA_15      // TIM2 CH1, 1 kHz
#define SERVO_PIN            PC_7       // TIM3 CH2, 50 Hz
#define CONTROL_MS           500U
#define DHT_PERIOD_MS        3000U       // 원본 유지; 최소 2초보다 길게 읽음

/* 실측 전 예시 설정. 상한 > 하한을 유지한다. */
#define TEMP_HIGH_C          28
#define TEMP_LOW_C           26
#define HUMID_HIGH_PCT       70
#define HUMID_LOW_PCT        60
#define DHT_FAIL_LIMIT       3U          // 연속 3회 실패하면 강제 환기
#define DHT_STALE_MS         10000U      // 정상값이 10초 이상 없을 때도 환기
#define FAN_ON_DUTY          1.0f        // ON=100%, OFF=0%인 드라이버 가정
#define SERVO_CLOSED_US      1000U       // 실제 창문 기구에 맞게 보정
#define SERVO_OPEN_US        2000U       // HIGH 펄스폭, PWM 주기는 별도 20ms

static volatile uint8_t control_due;          // TIM1 ISR -> main 제어 요청
static uint8_t vent_on, dht_valid, dht_seen, dht_failures;
static uint32_t last_good_dht;
static int temperature, humidity;
static int dht_error = DHT11_ERR_NO_REPLY;

/* unsigned 뺄셈으로 경과시간 판단. */
static uint8_t elapsed(uint32_t now, uint32_t since, uint32_t period)
{
    return (uint32_t)(now - since) >= period;
}

/* 상한 초과는 OR, 하한 미만은 AND. 사이 구간에서는 직전 환기 상태 유지.
 * 연속 3회 통신 실패 또는 정상값 없이 10초가 지나면 강제 환기한다.
 */
static void update_ventilation(uint32_t now)
{
    if (dht_failures >= DHT_FAIL_LIMIT || elapsed(now, last_good_dht, DHT_STALE_MS)) {
        vent_on = 1;
    } else if (dht_valid) {
        if (temperature > TEMP_HIGH_C || humidity > HUMID_HIGH_PCT)
            vent_on = 1;
        else if (temperature < TEMP_LOW_C && humidity < HUMID_LOW_PCT)
            vent_on = 0;
    }
    // 단발성 오류: 오래된 정상값으로 다시 판단하지 않고 현재 출력 유지.
    // 정상 복귀: 위 히스테리시스 규칙으로 돌아가므로 중간 영역에서는 유지.
}

/* DHT는 약 25ms 블로킹하므로 ISR 안에서 호출하지 않는다.
 * 실패하면 기존 T/H 숫자는 남아 있지만 dht_valid=0으로 구분한다.
 */
static void sample_dht(void)
{
    dht_error = DHT11_read();
    if (dht_error == DHT11_OK) {
        temperature = DHT11_temperature();
        humidity = DHT11_humidity();
        dht_valid = dht_seen = 1;
        dht_failures = 0;
        last_good_dht = msTicks;
    } else {
        dht_valid = 0;
        // TOOSOON은 스케줄 문제이므로 통신 실패 횟수에는 넣지 않는다.
        if (dht_error != DHT11_ERR_TOOSOON && dht_failures < 255U) ++dht_failures;
        printf("[DHT11] invalid err=%d failures=%u\r\n", dht_error, (unsigned)dht_failures);
    }
}

/* 온습도로 결정한 환기 상태를 팬과 창문에 함께 적용한다. */
static void apply_ventilation(void)
{
    PWM_duty(FAN_PIN, vent_on ? FAN_ON_DUTY : 0.0f);
    PWM_pulsewidth_us(SERVO_PIN, vent_on ? SERVO_OPEN_US : SERVO_CLOSED_US);
}

void setup(void)
{
    RCC_PLL_init();                           // 시스템 클록 84MHz
    SysTick_init();                           // msTicks 및 DHT us 시간 기준
    USART2_init();                            // USB 시리얼 printf
    USART2->CR1 &= ~USART_CR1_RXNEIE;

    PWM_init(FAN_PIN, 1);                     // 팬: 1ms 주기, 1kHz
    PWM_duty(FAN_PIN, 0.0f);                  // 초기 목표 OFF
    PWM_init(SERVO_PIN, 20);                  // 서보: 20ms 주기, 50Hz
    PWM_pulsewidth_us(SERVO_PIN, SERVO_CLOSED_US);
    // PWM_init의 기본 50% 출력이 잠깐 생길 수 있어 초기 파형은 driver 확인 필요.

    DHT11_init(DHT11_PIN);                    // DATA pull-up 필요(모듈 내장 여부 확인)
    TIM_UI_init(TIM1, CONTROL_MS);            // 500ms마다 환기 판단 요청
    clear_UIF(TIM1);
    last_good_dht = msTicks;                  // 최초 정상 측정을 기다리는 시간 기준
    control_due = 1;
}

int main(void)
{
    setup();
    uint32_t tDHT = msTicks, tPrint = msTicks;
    while (1) {
        uint32_t mask = __get_PRIMASK();
        __disable_irq();
        uint8_t tick = control_due;
        control_due = 0;
        __set_PRIMASK(mask);

        if (elapsed(msTicks, tDHT, DHT_PERIOD_MS)) {
            sample_dht();                    // main에서만 읽기
            tDHT = msTicks;                   // 읽기 완료 후 3초: 최소 간격 보장
        }
        if (tick) {
            update_ventilation(msTicks);
            apply_ventilation();
        }
        if (elapsed(msTicks, tPrint, 1000U)) {
            tPrint = msTicks;
            printf("DHT T=%dC H=%d%% SEEN=%u VALID=%u ERR=%d FAIL=%u VENT=%u\r\n",
                   temperature, humidity, (unsigned)dht_seen, (unsigned)dht_valid,
                   dht_error, (unsigned)dht_failures, (unsigned)vent_on);
            // SEEN=0: 정상 측정 전. VALID=0: T/H는 현재의 정상 측정값이 아님.
        }
    }
}

/* ISR에서는 요청만 남긴다. DHT/printf/PWM 제어는 main에서 실행한다.
 * DHT의 IRQ 차단 구간 때문에 msTicks 시간에 오차가 생길 수 있어 실측 확인.
 */
void TIM1_UP_TIM10_IRQHandler(void)            // F411의 실제 TIM1 update vector
{
    if (is_UIF(TIM1)) {
        clear_UIF(TIM1);
        control_due = 1;                     // 밀린 횟수만큼 오래된 제어를 재실행하지 않음
    }
}
