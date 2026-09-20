#include "stm32f411xe.h"
#include "ecRCC2.h"

#define LED_PIN     PA_5        // LD2

int main(void) {

    /* Part 1. RCC GPIOA Register Setting */
    RCC_HSI_init();
    RCC_GPIOA_enable();

    /* Part 2. GPIO Register Setting */

    // GPIO Mode Register : MODE = OUTPUT
    GPIOA->MODER &= ~(3UL << (2 * LED_PIN));
    GPIOA->MODER |=  (1UL << (2 * LED_PIN));

    // GPIO Output Type Register
    GPIOA->OTYPER &= ~(1UL << LED_PIN);

    // GPIO Pull-Up/Pull-Down Register
    GPIOA->PUPDR &= ~(3UL << (2 * LED_PIN));

    // GPIO Output Speed Register
    GPIOA->OSPEEDR &= ~(3UL << (2 * LED_PIN));
    GPIOA->OSPEEDR |=  (2UL << (2 * LED_PIN));

    while (1) {
        GPIOA->ODR |= (1UL << LED_PIN);
    }
}