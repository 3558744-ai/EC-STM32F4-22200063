/*---------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author          : DAEJINKIM
Modified        : 30-09-2026
Language/ver    : C++ in VS Code
Description     : Tutorial - EXTI Button Interrupt and LED Toggle
/----------------------------------------------------------*/

#include "ecRCC2.h"
#include "ecGPIO2.h"
#include "ecSysTick2.h"
#include "ecEXTI2.h"

#define LED0_PIN   PB_12      // EVAL board JKIT LED0
#define LED7_PIN   PC_3       // EVAL board JKIT LED7
#define SW2_PIN    PA_4       // EVAL board JKIT SW2

void setup(void) {
    RCC_PLL_init();            // System Clock = 84 MHz
    SysTick_init();            // SysTick Timer Initialization

    GPIO_init(LED0_PIN, OUTPUT);
    GPIO_init(LED7_PIN, OUTPUT);

    GPIO_init(SW2_PIN, INPUT);
    GPIO_pupd(SW2_PIN, PU);

    EXTI_init(SW2_PIN, FALL, 10);
}

int main(void) {
    setup();

    while (1) {
        GPIO_write(LED7_PIN, HIGH);
        delay_ms(1000);
        GPIO_write(LED7_PIN, LOW);
        delay_ms(1000);
    }
}

void EXTI4_IRQHandler(void) {
    if (is_pending_EXTI(SW2_PIN)) {
        led_toggle(LED0_PIN);
        clear_pending_EXTI(SW2_PIN);
    }
}
