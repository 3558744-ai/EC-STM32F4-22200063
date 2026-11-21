/*
 * LED PWM 진단용: 센서 없이 0 -> 25 -> 50 -> 75 -> 100%를 3초씩 출력.
 * 이 파일 하나만 src/main.c로 사용한다. 다른 main 파일과 동시 빌드 금지.
 * 기존 밝기 제어 파일과 드라이버는 수정하지 않는다.
 * 연결: PB7 -> LED 패널/드라이버의 PWM 입력, GND 공통, 패널 정격 전원.
 * PB7은 패널 전원 공급 단자가 아니다. 버튼을 누를 필요는 없다.
 *
 * 목적: 계산된 %뿐 아니라 TIM4의 실제 설정 레지스터도 읽어 확인한다.
 * 현재 EC driver/84MHz 클록에서 예상 PSC=83, ARR=999,
 * CCR2는 0, 250, 500, 750, 1000으로 변해야 한다.
 * 레지스터가 맞아도 핀의 실제 파형까지 증명되지는 않는다.
 * 오실로스코프가 있으면 PB7-GND에서 1kHz와 각 듀티를 확인한다.
 */
#include "ecSTM32F4v2.h"
#include "ecPWM2.h"
#include <stdint.h>
#include <stdio.h>

#define PANEL_PIN PB_7
#define STEP_MS 3000U

static const unsigned duty_percent[] = {0U, 25U, 50U, 75U, 100U};

static void show_step(unsigned step)
{
    unsigned percent = duty_percent[step];
    PWM_duty(PANEL_PIN, (float)percent / 100.0f);
    // 위 명령 뒤 새 CCR 값이 다음 PWM 주기에서 적용된다(preload).
    printf("PWM=%u%% PSC=%lu ARR=%lu CCR2=%lu\r\n",
           percent, (unsigned long)TIM4->PSC,
           (unsigned long)TIM4->ARR, (unsigned long)TIM4->CCR2);
    printf("PB7_MODE=%lu AF=%lu TIM4_RUN=%lu CH2_EN=%lu OC2M=%lu POL=%lu\r\n",
           (unsigned long)((GPIOB->MODER >> 14U) & 3U),
           (unsigned long)((GPIOB->AFR[0] >> 28U) & 15U),
           (unsigned long)(TIM4->CR1 & 1U),
           (unsigned long)((TIM4->CCER >> 4U) & 1U),
           (unsigned long)((TIM4->CCMR1 >> 12U) & 7U),
           (unsigned long)((TIM4->CCER >> 5U) & 1U));
    // 정상 설정 예상: MODE=2(AF), AF=2(TIM4), RUN=1, CH2_EN=1,
    // OC2M=6(PWM mode 1), POL=0(active high).
}

int main(void)
{
    RCC_PLL_init();
    SysTick_init();
    USART2_init();
    USART2->CR1 &= ~USART_CR1_RXNEIE;
    PWM_init(PANEL_PIN, 1);                  // 주기 1ms = 1kHz
    // driver 기본 duty가 50%인 경우 초기화 순간 잠깐 출력될 수 있음.

    unsigned step = 0;
    uint32_t started_at = msTicks;
    show_step(step);
    while (1) {
        uint32_t now = msTicks;
        if ((uint32_t)(now - started_at) >= STEP_MS) {
            started_at = now;
            step = (step + 1U) % (sizeof(duty_percent) / sizeof(duty_percent[0]));
            show_step(step);
        }
    }
}
