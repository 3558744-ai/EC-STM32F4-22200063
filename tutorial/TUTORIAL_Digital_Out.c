#include "stm32f411xe.h"
#include "ecRCC2.h"

#define LED_PIN     PA_5        // LD2

int main(void) {

    /* Part 1. RCC GPIOA Register Setting */
    RCC_HSI_init();
    RCC_GPIOA_enable();

    /* Part 2. GPIO Register Setting */

    // GPIO Mode Register : MODE = OUTPUT
    GPIOA->MODER &= ~(3UL << (2 * LED_PIN));   // Clear bits
    GPIOA->MODER |=  (1UL << (2 * LED_PIN));   // 01: Output

    // GPIO Output Type Register
    GPIOA->OTYPER &= ~(1UL << LED_PIN);        // 0: Push-Pull

    // GPIO Pull-Up/Pull-Down Register
    GPIOA->PUPDR &= ~(3UL << (2 * LED_PIN));   // 00: No Pull-Up/Down

    // GPIO Output Speed Register
    GPIOA->OSPEEDR &= ~(3UL << (2 * LED_PIN));
    GPIOA->OSPEEDR |=  (2UL << (2 * LED_PIN)); // 10: Fast Speed

    while (1) {
        GPIOA->ODR |= (1UL << LED_PIN);        // PA5 = HIGH → LED ON
    }
}