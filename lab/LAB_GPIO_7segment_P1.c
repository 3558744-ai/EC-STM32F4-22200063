/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : DAEJINKIM
Created          : 09-23-2026
Modified         : 09-23-2026
Language/ver     : C in VS Code

Description      : LAB_GPIO_7segment_P1
                   7-Segment Display on JKIT Board
/----------------------------------------------------------------*/

#include "stm32f4xx.h"
#include "ecGPIO2.h"
#include "ecRCC2.h"


// 7-Segment LED pins
// PB0 ~ PB7 correspond to a, b, c, d, e, f, g, dp
PinName_t pinFND[8] = {
    PB_0, PB_1, PB_2, PB_3,
    PB_4, PB_5, PB_6, PB_7
};


// 7-Segment selection pins
// FND0, FND1, FND2, FND3
PinName_t selectFND[4] = {
    PA_10, PA_11, PC_4, PC_3
};


// Initialization
void setup(void)
{
    // Initialize system clock
    RCC_HSI_init();

    // Initialize segment pins PB0 ~ PB7
    FND_init(pinFND);

    // Initialize FND selection pins
    FND_select_init(selectFND);
}


int main(void)
{
    // Initialize MCU and GPIO
    setup();


    // Numbers to display
    uint8_t numDisplay1 = 0;
    uint8_t numDisplay2 = 5;


    // Select FND position
    // 0 = FND0
    // 1 = FND1
    uint8_t selectFND1 = 0;
    uint8_t selectFND2 = 1;


    while (1)
    {
        // Select FND0
        FND_select(selectFND1);

        // Display number 0
        FND_display(numDisplay1);

        // Keep FND0 ON for 1 ms
        delay_ms_HSI(1);


        // Select FND1
        FND_select(selectFND2);

        // Display number 5
        FND_display(numDisplay2);

        // Keep FND1 ON for 1 ms
        delay_ms_HSI(1);
    }
}