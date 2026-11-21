/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : DAEJINKIM
Created          : 09-23-2026
Modified         : 09-23-2026
Language/ver     : C in VS Code

Description      : LAB_GPIO_7segment_P2
                   7-Segment Counter with SW2 Button
/----------------------------------------------------------------*/

#include "stm32f4xx.h"
#include "ecGPIO2.h"
#include "ecRCC2.h"


// SW2 button on JKIT board
#define BUTTON_PIN PA_4


// 7-segment LED pins
// PB0 ~ PB7 correspond to a, b, c, d, e, f, g, dp
PinName_t pinFND[8] = {
    PB_0, PB_1, PB_2, PB_3,
    PB_4, PB_5, PB_6, PB_7
};


// 7-segment selection pins
// FND0, FND1, FND2, FND3
PinName_t selectFND[4] = {
    PA_10, PA_11, PC_4, PC_3
};


// Initialization
void setup(void)
{
    // Initialize system clock
    RCC_HSI_init();

    // Initialize PB0 ~ PB7 as 7-segment output pins
    FND_init(pinFND);

    // Initialize FND0 ~ FND3 selection pins
    FND_select_init(selectFND);

    // Initialize SW2 as input
    GPIO_init(BUTTON_PIN, INPUT);

    // SW2 uses Pull-Up resistor
    // Not pressed = HIGH
    // Pressed     = LOW
    GPIO_pupd(BUTTON_PIN, PU);
}


int main(void)
{
    // Initialize MCU and GPIO
    setup();


    // Counter variable
    // Initial displayed number = 0
    uint8_t cnt = 0;


    // Select FND0 only
    FND_select(0);


    while (1)
    {
        // Display current count value
        FND_display(cnt);


        // Check whether SW2 is pressed
        // Since SW2 uses Pull-Up:
        // LOW means button is pressed
        if (GPIO_read(BUTTON_PIN) == LOW)
        {
            // Wait for mechanical button bouncing to settle
            delay_ms_HSI(20);


            // Check button state again
            if (GPIO_read(BUTTON_PIN) == LOW)
            {
                // Increase counter by 1
                cnt++;


                // After 9, return to 0
                if (cnt > 9)
                {
                    cnt = 0;
                }


                // Update 7-segment immediately
                FND_display(cnt);


                // Wait until button is released
                // This prevents multiple counts from one button press
                while (GPIO_read(BUTTON_PIN) == LOW);
            }
        }
    }
}