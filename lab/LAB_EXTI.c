/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University

Author           : DAEJINKIM
Created          : 05-03-2021
Modified         : 10-08-2026
Language/ver     : C in VS Code

Description      : LAB - EXTI & SysTick
                   7-Segment Counter with
                   SW1 Reset and SW2 Pause/Resume
/----------------------------------------------------------------*/

#include "ecRCC2.h"
#include "ecGPIO2.h"
#include "ecSysTick2.h"
#include "ecEXTI2.h"


// Pin Configuration

// SW1 : Reset counter to 0
#define SW1_PIN    PD_2

// SW2 : Pause / Resume
#define SW2_PIN    PA_4


// 7-Segment data pins

PinName_t pinFND[8] = {
    PB_0, // PB0 : a
    PB_1, // PB1 : b
    PB_2, // PB2 : c
    PB_3, // PB3 : d
    PB_4, // PB4 : e
    PB_5, // PB5 : f
    PB_6, // PB6 : g
    PB_7  // PB7 : dp
};


// 7-Segment selection pins
// FND0 : PA10
// FND1 : PA11
// FND2 : PC4
// FND3 : PC3

PinName_t selectFND[4] = {
    PA_10,
    PA_11,
    PC_4,
    PC_3
};


// Global Variables

// Current number displayed on the 7-Segment
volatile uint8_t count = 0;

// paused = 0 : counting
// paused = 1 : paused
volatile uint8_t paused = 0;

// reset_request = 1 : reset counter to 0
volatile uint8_t reset_request = 0;


// Function Prototype

void setup(void);


// Initialization

void setup(void)
{
    // System Clock = 84 MHz
    RCC_PLL_init();

    // Initialize SysTick Timer
    SysTick_init();


    // 7-Segment Initialization

    // Initialize PB0 ~ PB7
    FND_init(pinFND);

    // Initialize digit selection pins
    FND_select_init(selectFND);

    // Use FND0 only
    FND_select(0);


    // SW1 Initialization

    // PD2 input
    GPIO_init(SW1_PIN, INPUT);

    // Internal Pull-Up resistor
    GPIO_pupd(SW1_PIN, PU);

    // External Interrupt:
    // Falling Edge, Priority = 10
    EXTI_init(SW1_PIN, FALL, 10);


    // SW2 Initialization

    // PA4 input
    GPIO_init(SW2_PIN, INPUT);

    // Internal Pull-Up resistor
    GPIO_pupd(SW2_PIN, PU);

    // External Interrupt:
    // Falling Edge, Priority = 10
    EXTI_init(SW2_PIN, FALL, 10);
}


int main(void)
{
    setup();

    while (1)
    {
        // Display current count
        FND_display(count);

        // Keep current number for 1 second
        delay_ms(1000);


        // Reset counter when SW1 is pressed
        if (reset_request)
        {
            count = 0;
            reset_request = 0;
        }

        // Increase count only when not paused
        else if (!paused)
        {
            count++;

            // Repeat 0 ~ 9
            if (count > 9)
            {
                count = 0;
            }
        }
    }
}


// SW1 Interrupt Handler
// PD2 -> EXTI Line 2

void EXTI2_IRQHandler(void)
{
    // Check whether EXTI2 interrupt occurred
    if (is_pending_EXTI(SW1_PIN))
    {
        // Request counter reset
        reset_request = 1;

        // Clear EXTI pending flag
        clear_pending_EXTI(SW1_PIN);
    }
}


// SW2 Interrupt Handler
// PA4 -> EXTI Line 4

void EXTI4_IRQHandler(void)
{
    // Check whether EXTI4 interrupt occurred
    if (is_pending_EXTI(SW2_PIN))
    {
        // Toggle Pause / Resume state
        paused = !paused;

        // Clear EXTI pending flag
        clear_pending_EXTI(SW2_PIN);
    }
}